#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "math.h"
#include "mpi.h"

void MatrixAInit(int N, double* MatrixA)
{
    for (int i=0;i<N;i++)
    {
        for (int j=0;j<N;j++)
        {
            MatrixA[i*N + j] = i + j;
        }
    }
}
void MatrixBInit(int N, double* MatrixB)
{
    for (int i=0;i<N;i++)
    {
        for (int j=0;j<N;j++)
        {
            MatrixB[i*N + j]= i * j;
        }
    }
}

void PrintC(int N, double* MatrixC)
{
    for (int i=0;i<N;i++)
    {
        for (int j=0;j<N;j++)
        {
            printf ("%f  ", MatrixC[i*N+j] ) ;
        }
        printf("\n");
    }
}

void initProcess(double** pMatrixA, double** pMatrixB, double** pMatrixC, int size, double** pProcARows, double** pProcBCols, double** pProcCRows,
                 int** pColNums, int** pColInds, int procNum, int procRank)
{
    double* matrixA = NULL;
    double* matrixB = NULL;
    double* matrixC = NULL;
    double* procARows = NULL;
    double* procBCols = NULL;
    double* procCRows = NULL;
    int* colNums = NULL;
    int* colInds = NULL;
    int i = 0, j = 0, temp = 0;

    colNums = (int*) malloc(procNum * sizeof(int));
    colInds = (int*) malloc(procNum * sizeof(int));
    temp = size % procNum;
    colNums[0] = size / procNum;
    if (temp > 0)
        ++colNums[0];
    else
        temp = 1;
    colInds[0] = 0;
    for (i = 1; i < temp; ++i)
    {
        colNums[i] = size / procNum + 1;
        colInds[i] = colInds[i - 1] + colNums[i - 1];
    }
    for (i = temp; i < procNum; ++i)

    {
        colNums[i] = size / procNum;
        colInds[i] = colInds[i - 1] + colNums[i - 1];
    }

    procARows = (double*) malloc(colNums[0] * size * sizeof(double));
    procBCols = (double*) malloc(colNums[0] * size * sizeof(double));
    procCRows = (double*) malloc(colNums[0] * size * sizeof(double));

    for (i = 0; i < colNums[0]; ++i)
    {
        for (j = 0; j < size; ++j)
        {
            procCRows[i * size + j] = 0.0;
        }
    }

    if (procRank == 0)
    {
        matrixA = (double*) malloc(size * size * sizeof(double));
        matrixB = (double*) malloc(size * size * sizeof(double));
        matrixC = (double*) malloc(size * size * sizeof(double));
        MatrixAInit(size, matrixA);
        MatrixBInit(size, matrixB);

        for (i = 0; i < size; ++i)
        {
            for (j = 0; j < size; ++j)
            {
                matrixC[i * size + j] = 0.0;
            }
        }
    }

    *pMatrixA = matrixA;
    *pMatrixB = matrixB;
    *pMatrixC = matrixC;
    *pProcARows = procARows;
    *pProcBCols = procBCols;
    *pProcCRows = procCRows;
    *pColNums = colNums;
    *pColInds = colInds;
}

void deInitProcess(double* matrixA, double* matrixB, double* matrixC, double* procARows, double* procBCols, double* procCRows, int* colNums, int* colInds, int procRank)
{
    if (procRank == 0)
    {
        free(matrixA);
        free(matrixB);
        free(matrixC);
    }
    free(colNums);
    free(colInds);
    free(procARows);
    free(procBCols);
    free(procCRows);
}

void distributeData(double* matrixA, double* matrixB, int size, double* procARows, double* procBCols, int* colNums, int procNum, int procRank)
{
    int* sendNums = NULL;
    int* sendInds = NULL;
    int i = 0;

    sendNums = (int*) malloc(procNum * sizeof(int));
    sendInds = (int*) malloc(procNum * sizeof(int));
    sendNums[0] = colNums[0] * size;
    sendInds[0] = 0;
    for (i = 1; i < procNum; ++i)
    {
        sendNums[i] = colNums[i] * size;
        sendInds[i] = sendInds[i - 1] + sendNums[i - 1];
    }

    MPI_Scatterv(matrixA, sendNums, sendInds, MPI_DOUBLE, procARows, colNums[procRank] * size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(matrixB, sendNums, sendInds, MPI_DOUBLE, procBCols, colNums[procRank] * size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    free(sendNums);
    free(sendInds);
}
void parallelMult(double* procARows, double* procBCols, double* procCRows, int size, int* colNums, int* colInds, int procNum, int procRank)
{
    int i = 0, j = 0, k = 0, p = 0, t = 0;MPI_Status status;

    for (p = 0; p < procNum - 1; ++p)
    {
        t = (procRank + p) % procNum;
        for (i = 0; i < colNums[procRank]; ++i)
        {
            for (j = 0; j < colNums[t]; ++j)
            {
                for (k = 0; k < size; ++k)
                {
                    procCRows[i * size + colInds[t] + j] += procARows[i * size + k] * procBCols[k + j * size];
                }
            }
        }
        MPI_Sendrecv_replace(procBCols, colNums[0] * size, MPI_DOUBLE, ((procRank == 0) ? (procNum - 1) : (procRank - 1)), 0, (procRank + 1) % procNum, 0, MPI_COMM_WORLD, &status);
    }
    t = (procRank + procNum - 1) % procNum;
    for (i = 0; i < colNums[procRank]; ++i)
    {
        for (j = 0; j < colNums[t]; ++j)
        {
            for (k = 0; k < size; ++k)
            {
                procCRows[i * size + colInds[t] + j] += procARows[i * size + k] * procBCols[k + j * size];
            }
        }
    }
}

void replicateData(double* matrixC, int size, double* procCRows, int* colNums, int procNum, int procRank)
{
    int* recvNums = NULL;
    int* recvInds = NULL;
    int i = 0;

    recvNums = (int*) malloc(procNum * sizeof(int));
    recvInds = (int*) malloc(procNum * sizeof(int));
    recvNums[0] = colNums[0] * size;
    recvInds[0] = 0;
    for (i = 1; i < procNum; ++i)
    {
        recvNums[i] = colNums[i] * size;
        recvInds[i] = recvInds[i - 1] + recvNums[i - 1];
    }

    MPI_Gatherv(procCRows, colNums[procRank] * size, MPI_DOUBLE, matrixC, recvNums, recvInds, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    free(recvNums);
    free(recvInds);
}

int main(int argc, char *argv[])
{
    int procNum = 0, procRank = 0;
    int size = 3000;
    double* matrixA = NULL;
    double* matrixB = NULL;
    double* matrixC = NULL;
    double* testC = NULL;
    double* procARows = NULL;
    double* procBCols = NULL;
    double* procCRows = NULL;
    int* colNums = NULL;
    int* colInds = NULL;
    double t1 = 0.0, t2 = 0.0, serialDur = 0.0, parallelDur = 0.0;
    int i = 0, j = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &procNum);
    MPI_Comm_rank(MPI_COMM_WORLD, &procRank);
    initProcess(&matrixA, &matrixB, &matrixC, size, &procARows, &procBCols, &procCRows, &colNums, &colInds, procNum, procRank);
    distributeData(matrixA, matrixB, size, procARows, procBCols, colNums, procNum, procRank);
    MPI_Barrier(MPI_COMM_WORLD);
    t1 = MPI_Wtime();
    parallelMult(procARows, procBCols, procCRows, size, colNums, colInds, procNum, procRank);
    MPI_Barrier(MPI_COMM_WORLD);
    t2 = MPI_Wtime();
    parallelDur = t2 - t1;
    printf (" ********** mpi_time = % f \n" , parallelDur);
    replicateData(matrixC, size, procCRows, colNums, procNum, procRank);
    //PrintC(size, matrixC);
    deInitProcess(matrixA, matrixB, matrixC, procARows, procBCols, procCRows, colNums, colInds, procRank);
    MPI_Finalize();
    return 0;
}
