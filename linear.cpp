#include <omp.h>
#include <cstdlib>
#include <cmath>
#include <iostream>
using namespace std;
#define N 10

double MatrixA[N][N];
double MatrixB[N][N];
double MatrixC[N][N];

void MatrixAInit()
{
    for (int i=0;i<N;i++)
    {
        for (int j=0;j<N;j++)
        {
            MatrixA[i][j]= i + j;
        }
    }
}
void MatrixBInit()
{
    for (int i=0;i<N;i++)
    {
        for (int j=0;j<N;j++)
        {
            MatrixB[i][j]= i * j;
        }
    }
}

void PrintC()
{
    for (int i=0;i<N;i++)
    {
        for (int j=0;j<N;j++)
        {
            printf("%f ", MatrixC[i][j]);
        }
        printf("\n");
    }
}
int main(int argc, char* argv[]) {
    MatrixAInit();
    MatrixBInit();
    int i, j, k;
    double time_linear = omp_get_wtime();
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            MatrixC[i][j] = 0;
            for (k = 0; k < N; k++) {
                MatrixC[i][j] = MatrixC[i][j] + MatrixA[i][k] * MatrixB[k][j];
            }
        }
    }
    time_linear = omp_get_wtime() - time_linear;
    printf("time = %f\n", time_linear);
    PrintC();
    return 0;
}
