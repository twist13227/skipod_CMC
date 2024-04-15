#include <iostream>
#include <vector>
#include <set>
#include "mpi.h"
#include <unistd.h>
#include <cstdio>
#include <thread>
#include <fstream>
using namespace std;

// ������������ ����� �������� ���������� �� ���� � ����������� ������
static int MaxWait = 10000;
// ����� ������ ���������� ��������� (����� ��� ��������� ��������� �����)
static double start_time;
// ����� �������� ��������
static int my_rank;
// ���������� ���������
static int world_size;
// ��������� ������� ��������� � ������������� ��������� 
static set<int> not_answered;
// ������� ��� ����������� ����������/������������ ��������
static vector<MPI_Request> sent_requests, answered_requests, finalize_requests;


bool are_all_finished() {
    static int count_finished = 1;
    int answer;
    int finished = true;
    MPI_Status status;
    while (finished) {
        // �������� ������� ������ ���������
        MPI_Iprobe(MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, &finished, &status);
        // ���� �� ����������, �� finished ������ false
        if (finished) {
            int proc_rank = status.MPI_SOURCE;
            MPI_Recv(&answer, 1, MPI_INT, proc_rank, 4, MPI_COMM_WORLD, &status);
            count_finished++;
        }
    }
    if (count_finished == world_size) {
        // ������
        for (auto& request : finalize_requests)
            MPI_Wait(&request, MPI_STATUS_IGNORE);
        finalize_requests.clear();
        // ��� �����������
        return true;
    }
    // �� ��� �����������
    return false;
}


void critical() {
    // ��������� ����� ��������
    double current_time = MPI_Wtime() - start_time;
    // ��������� "OK"
    int ok_message = 0;
    // ������� ���� ��������� ��������� - ������, ���������� ��� ����������� ������, ����� �������� � ������� �����.
    for (int i = 0; i < world_size; i++) {
        if (i != my_rank) {
            sent_requests.push_back(MPI_REQUEST_NULL);
            MPI_Isend(&current_time, 1, MPI_DOUBLE, i, 2, MPI_COMM_WORLD, &(sent_requests.back()));
        }
    }
    // ������ ���������� � ���, �������� �� ���������� �� ������ ���������
    vector<bool> permissions(world_size, false);
    int count_oks = 0;
    while (count_oks != world_size - 1) {
        MPI_Status status;
        int flag = true;
        // �������� ���������� �� ���� � ����������� ������
        while (flag) {
            // ��������� �� ������ �� ������ �� ����
            MPI_Iprobe(MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                int answer;
                int proc_rank = status.MPI_SOURCE;
                MPI_Recv(&answer, 1, MPI_INT, proc_rank, 3, MPI_COMM_WORLD, &status);
                if (proc_rank != my_rank && !permissions[proc_rank]) {
                    if (answer != 0)
                        MPI_Abort(MPI_COMM_WORLD, 1);
                    permissions[proc_rank] = true;
                    count_oks++;
                }
            }
        }
        // ����� �� ���������� ������� 
        flag = true;
        while (flag) {
            // ��������� � ������� �� ����
            MPI_Iprobe(MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                // ����� ������� �������� 
                int proc_rank = status.MPI_SOURCE;
                double other_timestamp;
                MPI_Recv(&other_timestamp, 1, MPI_DOUBLE, proc_rank, 2, MPI_COMM_WORLD, &status);
                // ��������� ��������� ����� ������ ������� � ������. ��������� ���, ��� ����� ������
                if (other_timestamp < current_time) {
                    // ����� ������� "�������" => ���������� "��"
                    answered_requests.push_back(MPI_REQUEST_NULL);
                    MPI_Isend(&ok_message, 1, MPI_INT, proc_rank, 3, MPI_COMM_WORLD, &(answered_requests.back()));
                }
                else {
                    // ����������� ������ �������� ������������� �������
                    not_answered.insert(proc_rank);
                }
            }
        }
        // ��������, ���� ��� ����� ����������
        sleep(rand() % MaxWait);
    }

    // ���� � ����������� ������
    sleep(rand() % MaxWait);
    if (access("critical.txt", F_OK) != -1) {
        printf("���� critical.txt ��� ����������!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    else {
        FILE* file = fopen("critical.txt", "w");
        fclose(file);
        // ��������� ��������
        sleep(rand() % MaxWait);
        remove("critical.txt");
    }
    // ����� ����������� ������

    // ����� ���, ���� �� �������� �����
    for (int i : not_answered) {
        answered_requests.push_back(MPI_REQUEST_NULL);
        MPI_Isend(&ok_message, 1, MPI_INT, i, 3, MPI_COMM_WORLD, &(answered_requests.back()));
    }

    // ������ ����������� �������� 
    for (auto& request : answered_requests)
        MPI_Wait(&request, MPI_STATUS_IGNORE);
    answered_requests.clear();
    for (auto& request : sent_requests)
        MPI_Wait(&request, MPI_STATUS_IGNORE);
    sent_requests.clear();

    //  ���� ���������� �� ��������� ������ ����������� ������ � �� ���������� ���������� �� ���� � ���, �� �� �������� ����������� ��������� "OK"
    int flag = true;
    while (flag) {
        MPI_Status status;
        // ��������� � ������� �� ����
        MPI_Iprobe(MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &flag, &status);
        if (flag) {
            int proc_rank = status.MPI_SOURCE;
            double other_timestamp;
            MPI_Recv(&other_timestamp, 1, MPI_DOUBLE, proc_rank, 2, MPI_COMM_WORLD, &status);
            answered_requests.push_back(MPI_REQUEST_NULL);
            MPI_Isend(&ok_message, 1, MPI_INT, proc_rank, 3, MPI_COMM_WORLD, &(answered_requests.back()));
        }
    }
    while (flag) {
        MPI_Status status;
        // ��������� � ������
        MPI_Iprobe(MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, &flag, &status);
        if (flag) {
            int proc_rank = status.MPI_SOURCE;
            double other_timestamp;
            MPI_Recv(&other_timestamp, 1, MPI_DOUBLE, proc_rank, 2, MPI_COMM_WORLD, &status);
            answered_requests.push_back(MPI_REQUEST_NULL);
            MPI_Isend(&ok_message, 1, MPI_INT, proc_rank, 3, MPI_COMM_WORLD, &(answered_requests.back()));
        }
        // ������
        for (auto& request : answered_requests)
            MPI_Wait(&request, MPI_STATUS_IGNORE);
        answered_requests.clear();
    }

    // ������� ���� ��������� � ������������ ���������
    int final_answer = 1;
    for (int i = 0; i < world_size; i++) {
        if (i != my_rank) {
            finalize_requests.push_back(MPI_REQUEST_NULL);
            MPI_Isend(&final_answer, 1, MPI_INT, i, 4, MPI_COMM_WORLD, &(finalize_requests.back()));
        }
    }

    // ��������, ���� ��� �������� �������� ����������, ����� �� ���� ���������� deadlock'a
    while (!are_all_finished()) {
        //  ���� ���������� �� ��������� ������ ����������� ������ � �� ���������� ���������� �� ���� � ���, �� �� �������� ����������� ��������� "OK"
        int flag = true;
        while (flag) {
            MPI_Status status;
            // ��������� � ������� �� ����
            MPI_Iprobe(MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                int proc_rank = status.MPI_SOURCE;
                double other_timestamp;
                MPI_Recv(&other_timestamp, 1, MPI_DOUBLE, proc_rank, 2, MPI_COMM_WORLD, &status);
                answered_requests.push_back(MPI_REQUEST_NULL);
                MPI_Isend(&ok_message, 1, MPI_INT, proc_rank, 3, MPI_COMM_WORLD, &(answered_requests.back()));
            }
        }
        while (flag) {
            MPI_Status status;
            // ��������� � ������
            MPI_Iprobe(MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                int proc_rank = status.MPI_SOURCE;
                double other_timestamp;
                MPI_Recv(&other_timestamp, 1, MPI_DOUBLE, proc_rank, 2, MPI_COMM_WORLD, &status);
                answered_requests.push_back(MPI_REQUEST_NULL);
                MPI_Isend(&ok_message, 1, MPI_INT, proc_rank, 3, MPI_COMM_WORLD, &(answered_requests.back()));
            }
            // ������
            for (auto& request : answered_requests)
                MPI_Wait(&request, MPI_STATUS_IGNORE);
            answered_requests.clear();
        }
        sleep(rand() % MaxWait);
    }
}


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    critical();
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
