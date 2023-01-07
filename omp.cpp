#include <omp.h>
#include <cstdlib>
#include <cmath>
#include <iostream>
using namespace std;
#define N 3000

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
            cout << MatrixC[i][j]<<" ";
        }
        cout<<endl;
    }
}
void MatrixCInit()
{
    for (int i=0;i<N;i++)
    {
        for (int j=0;j<N;j++)
        {
            MatrixC[i][j]= 0;
        }
    }
}
int main(int argc, char* argv[]) {
    MatrixAInit();
    MatrixBInit();
    MatrixCInit();
    int i, j, k;
    omp_set_nested(true);
    double time_omp = omp_get_wtime();
#pragma omp parallel for private (j, k) shared(MatrixA, MatrixB)
    for (i = 0; i < N; i++) {
#pragma omp parallel for private (k) shared(MatrixA, MatrixB)
        for (j = 0; j < N; j++) {
            for (k = 0; k < N; k++) {
                MatrixC[i][j] = MatrixC[i][j] + MatrixA[i][k] * MatrixB[k][j];
            }
        }
    }
    time_omp = omp_get_wtime() - time_omp;
    cout<<"time_omp = "<<time_omp<<endl;
    //PrintC();
    return 0;
}
