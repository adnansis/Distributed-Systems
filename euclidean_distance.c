// Prevajanje programa: 
//				gcc -O2 -lm -l pthread -lgomp -w -o euclidean_distance euclidean_distance.c
// Zagon programa: 
//              srun -n1 --reservation=fri --cpus-per-task=8 ./euclidean_distance

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <omp.h>
#include <stdlib.h>

#define THREADS 8
#define D 100000000

pthread_t thread[THREADS];

pthread_barrier_t barrier;

int level = THREADS;
int modul = 2;

double *a;

double *b;

double result[THREADS];

void *kvadrati(void *arg) {

    int rank = (int)arg;
	int start = (int)(D / THREADS * rank);
	int stop = (int)(D / THREADS * (rank + 1));

    double d = 0;

	for(int i = start; i < stop; i++) {

        d += (double)pow((a[i] - b[i]), 2);
    }

    result[rank] = d;

    // navadno seštejemo vse d po drevesni strukturi (barrier!!!)

    pthread_barrier_wait(&barrier);

    while(level > 2) {

        if (rank % modul == 0 && (rank + modul / 2) < THREADS){

            result[rank] = result[rank] + result[rank + (modul/2)];

            if((rank + modul / 2) == THREADS - 1) {
                result[(rank + modul / 2)] = 0;
            }
        }

        pthread_barrier_wait(&barrier);
        
        if(rank == 0) {
            level /= 2;
            modul *= 2;
        }

        pthread_barrier_wait(&barrier);
    }

    if (rank == 0) {

        if(result[THREADS - 1] == 0) {

            if(ceil(log2(THREADS)) == floor(log2(THREADS))){

                result[rank] = result[rank] + result[(THREADS / 2)];

            } else {

                result[rank] = result[rank] + result[(THREADS / 2) + 1];

            }
        } else {

            result[rank] = result[rank] + result[(THREADS / 2)] + result[THREADS - 1];

        }
    }
    return NULL;
}

int main() {

    // generiranje vetorjev

    a = malloc(D * sizeof(double));
    b = malloc(D * sizeof(double));

    for (int i = 0; i < D; i++) {
        a[i] = rand() / ((double)RAND_MAX);
        b[i] = rand() / ((double)RAND_MAX);
    }

    double start = omp_get_wtime();

    pthread_barrier_init(&barrier, NULL, THREADS);

    for(int i = 0; i < THREADS; i++) {
		pthread_create(&thread[i], NULL, kvadrati, (void *)i);
	}

	for(int i = 0; i < THREADS; i++) {
		pthread_join(thread[i], NULL);
	}

    double end = omp_get_wtime();

    printf("Rezultat = %f\n", result[0]);
    printf("Čas =  %f seconds | Velikost = %d | Niti = %d\n", end - start, D, THREADS);
    
    free(a);
    free(b);
    
    return 0;
}

/*

Rezultat naloge:
	
ŠTEVILO	    Čas	
NITI		

1	        0.228282 seconds

2	        0.238046 seconds

4	        0.134414 seconds

8		    0.108033 seconds

*/