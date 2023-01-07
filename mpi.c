include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

void MatrixAInit(int N, int* MatrixA)
{
    for (int i=0;i<N;i++)
    {
        for (int j=0;j<N;j++)
        {
            MatrixA[i*N + j] = i + j;
        }
    }
}
void MatrixBInit(int N, int* MatrixB)
{
    for (int i=0;i<N;i++)
    {
        for (int j=0;j<N;j++)
        {
            MatrixB[i*N + j]= i * j;
        }
    }
}

void PrintC(int N, int* MatrixC)
{
    for (int i=0;i<N;i++)
    {
        for (int j=0;j<N;j++)
        {
            printf ("%d  ", MatrixC[i*N+j] ) ;
        }
        printf("\n");
    }
}
int main(int argc, char *argv[])
{
    int n , pid, p ;
    n = 10;
    int *sendA, *sendB, *finalC , *recA , *recC;
    int i, j, k;
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    if (pid ==0 ) {
        sendA = (int*) malloc( n *n * sizeof( int* ));
        sendB = (int*) malloc( n *n *sizeof( int* ));
        finalC = (int*) malloc( n *n* sizeof( int* ));
        MatrixAInit(n, sendA);
        MatrixBInit(n, sendB);
    }
    double start = MPI_Wtime();
    MPI_Bcast (&n, 1 , MPI_INT , 0 , MPI_COMM_WORLD) ;
    printf ("  n / p is %d \n " , n/p  ) ;
    recA = (int*) malloc( n/p * n * sizeof( int* ));
    if (sendB == NULL) {sendB = (int*) malloc( n * n * sizeof( int* ));}
    recC = (int*) malloc( n /p *n * sizeof( int* ));
    MPI_Scatter(sendA , n * n/p , MPI_INT , recA , n * n/p , MPI_INT , 0, MPI_COMM_WORLD) ;
    MPI_Bcast (sendB, n*n , MPI_INT , 0 , MPI_COMM_WORLD) ;
    for (i=0 ; i< (n/p) ; i++ )
        for (j = 0 ; j<n ; j++ )
        {
            recC[i*n+j] = 0 ;
            for (k = 0 ; k<n ; k++ )
                recC[i*n+j] += recA[i*n+k]*sendB[k*n+j];
        }
    MPI_Gather(recC , n*n/p , MPI_INT , finalC , n*n/p , MPI_INT , 0, MPI_COMM_WORLD) ;
    double end = MPI_Wtime();
    double time = end - start ;
    printf (" ********** mpi_time = % f \n" , time ) ;
    if (pid == 0) {
        PrintC(n, finalC);
    }
    MPI_Finalize();
    return 0;
}
