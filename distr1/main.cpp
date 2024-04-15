#include <iostream>
#include <vector>
#include <set>
#include "mpi.h"
#include <unistd.h>
#include <cstdio>
#include <thread>
#include <fstream>
using namespace std;

// максимальное время ожидания разрешения на вход в критическую секцию
static int MaxWait = 10000;
// время начала выполнения программы (нужно для получения временных меток)
static double start_time;
// номер текущего процесса
static int my_rank;
// количество процессов
static int world_size;
// множество номеров процессов с неотвеченными запросами 
static set<int> not_answered;
// вектора для запоминания полученных/отправленных запросов
static vector<MPI_Request> sent_requests, answered_requests, finalize_requests;


bool are_all_finished() {
    static int count_finished = 1;
    int answer;
    int finished = true;
    MPI_Status status;
    while (finished) {
        // проверка статуса других процессов
        MPI_Iprobe(MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, &finished, &status);
        // если не завершился, то finished станет false
        if (finished) {
            int proc_rank = status.MPI_SOURCE;
            MPI_Recv(&answer, 1, MPI_INT, proc_rank, 4, MPI_COMM_WORLD, &status);
            count_finished++;
        }
    }
    if (count_finished == world_size) {
        // чистка
        for (auto& request : finalize_requests)
            MPI_Wait(&request, MPI_STATUS_IGNORE);
        finalize_requests.clear();
        // все завершились
        return true;
    }
    // не все завершились
    return false;
}


void critical() {
    // временная метка процесса
    double current_time = MPI_Wtime() - start_time;
    // сообщение "OK"
    int ok_message = 0;
    // посылка всем процессам сообщение - запрос, содержащее имя критической секции, номер процесса и текущее время.
    for (int i = 0; i < world_size; i++) {
        if (i != my_rank) {
            sent_requests.push_back(MPI_REQUEST_NULL);
            MPI_Isend(&current_time, 1, MPI_DOUBLE, i, 2, MPI_COMM_WORLD, &(sent_requests.back()));
        }
    }
    // вектор информации о том, получено ли разрешение от других процессов
    vector<bool> permissions(world_size, false);
    int count_oks = 0;
    while (count_oks != world_size - 1) {
        MPI_Status status;
        int flag = true;
        // Ожидание разрешения на вход в критическую секцию
        while (flag) {
            // сообщение об ответе на запрос на вход
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
        // ответ на полученные запросы 
        flag = true;
        while (flag) {
            // сообщение о запросе на вход
            MPI_Iprobe(MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                // номер другого процесса 
                int proc_rank = status.MPI_SOURCE;
                double other_timestamp;
                MPI_Recv(&other_timestamp, 1, MPI_DOUBLE, proc_rank, 2, MPI_COMM_WORLD, &status);
                // сравнение временных меток своего запроса и чужого. Побеждает тот, чья метка меньше
                if (other_timestamp < current_time) {
                    // чужой процесс "победил" => отправляем "ок"
                    answered_requests.push_back(MPI_REQUEST_NULL);
                    MPI_Isend(&ok_message, 1, MPI_INT, proc_rank, 3, MPI_COMM_WORLD, &(answered_requests.back()));
                }
                else {
                    // запоминание номера процесса неотвеченного запроса
                    not_answered.insert(proc_rank);
                }
            }
        }
        // ожидание, пока все дадут разрешение
        sleep(rand() % MaxWait);
    }

    // вход в критическую секцию
    sleep(rand() % MaxWait);
    if (access("critical.txt", F_OK) != -1) {
        printf("Файл critical.txt уже существует!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    else {
        FILE* file = fopen("critical.txt", "w");
        fclose(file);
        // Случайное ожидание
        sleep(rand() % MaxWait);
        remove("critical.txt");
    }
    // конец критической секции

    // ответ тем, кому не ответили ранее
    for (int i : not_answered) {
        answered_requests.push_back(MPI_REQUEST_NULL);
        MPI_Isend(&ok_message, 1, MPI_INT, i, 3, MPI_COMM_WORLD, &(answered_requests.back()));
    }

    // чистка запомненных запросов 
    for (auto& request : answered_requests)
        MPI_Wait(&request, MPI_STATUS_IGNORE);
    answered_requests.clear();
    for (auto& request : sent_requests)
        MPI_Wait(&request, MPI_STATUS_IGNORE);
    sent_requests.clear();

    //  Если получатель не находится внутри критической секции и не запрашивал разрешение на вход в нее, то он посылает отправителю сообщение "OK"
    int flag = true;
    while (flag) {
        MPI_Status status;
        // сообщение о запросе на вход
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
        // сообщение о выходе
        MPI_Iprobe(MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, &flag, &status);
        if (flag) {
            int proc_rank = status.MPI_SOURCE;
            double other_timestamp;
            MPI_Recv(&other_timestamp, 1, MPI_DOUBLE, proc_rank, 2, MPI_COMM_WORLD, &status);
            answered_requests.push_back(MPI_REQUEST_NULL);
            MPI_Isend(&ok_message, 1, MPI_INT, proc_rank, 3, MPI_COMM_WORLD, &(answered_requests.back()));
        }
        // чистка
        for (auto& request : answered_requests)
            MPI_Wait(&request, MPI_STATUS_IGNORE);
        answered_requests.clear();
    }

    // посылка всем процессам с запомненными запросами
    int final_answer = 1;
    for (int i = 0; i < world_size; i++) {
        if (i != my_rank) {
            finalize_requests.push_back(MPI_REQUEST_NULL);
            MPI_Isend(&final_answer, 1, MPI_INT, i, 4, MPI_COMM_WORLD, &(finalize_requests.back()));
        }
    }

    // ожидание, пока все процессы закончат выполнение, чтобы не было случайного deadlock'a
    while (!are_all_finished()) {
        //  Если получатель не находится внутри критической секции и не запрашивал разрешение на вход в нее, то он посылает отправителю сообщение "OK"
        int flag = true;
        while (flag) {
            MPI_Status status;
            // сообщение о запросе на вход
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
            // сообщение о выходе
            MPI_Iprobe(MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                int proc_rank = status.MPI_SOURCE;
                double other_timestamp;
                MPI_Recv(&other_timestamp, 1, MPI_DOUBLE, proc_rank, 2, MPI_COMM_WORLD, &status);
                answered_requests.push_back(MPI_REQUEST_NULL);
                MPI_Isend(&ok_message, 1, MPI_INT, proc_rank, 3, MPI_COMM_WORLD, &(answered_requests.back()));
            }
            // чистка
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
