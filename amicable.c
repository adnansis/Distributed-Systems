// Prevajanje programa: 
//				gcc -O2 -lm -lpthread -fopenmp -w -o amicable amicable.c
// Zagon programa: 
//				srun -n1 --reservation=fri --cpus-per-task=32 ./amicable

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <omp.h>


#define N 10000000
#define THREADS 32
#define PACK_SIZE 1000

int sum_all = 0;

int vsote[N+1];

pthread_mutex_t index_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_t thread[THREADS];

int paket_start = 0;
 
double start; 
double end;

void *vsota_staticno_enakomerno(void *arg) {

	int rank = (int)arg;
	int start = (int)(N / THREADS * rank);
	int stop = (int)(N / THREADS * (rank + 1));

	for(int i = start; i < stop; i++) {

		int sum = 1;
		int koren = sqrt(i);

		for(int j = 2; j <= koren; j++) {

			if(i % j == 0){
				// if both divisors are same then
				// add it only once else add both
				if(j == (i / j)) {
					sum += j;
				} else {
					sum += (j + i / j);
				}
			}
		}

		pthread_mutex_lock(&index_mutex);
		vsote[i] = sum;
		pthread_mutex_unlock(&index_mutex);
	}

	return NULL;
}

void *vsota_staticno_krozno(void *arg) {

	int rank = (int)arg;

	for(int i = rank; i < N; i=i+THREADS) {

		int sum = 1;
		int koren = sqrt(i);

		for(int j = 2; j <= koren; j++) {

			if(i % j == 0){
				// if both divisors are same then
				// add it only once else add both
				if(j == (i / j)) {
					sum += j;
				} else {
					sum += (j + i / j);
				}
			}
		}

		pthread_mutex_lock(&index_mutex);
		vsote[i] = sum;
		pthread_mutex_unlock(&index_mutex);
	}

	return NULL;
}

void *vsota_dinamicno(void *arg) {

	int rank = (int)arg;

	while(paket_start < N) {

		pthread_mutex_lock(&index_mutex);
		int tmp = paket_start;
		paket_start += PACK_SIZE;
		pthread_mutex_unlock(&index_mutex);

		for(int i = tmp; i < tmp + PACK_SIZE; i++) {

			int sum = 1;
			int koren = sqrt(i);

			for(int j = 2; j <= koren; j++) {

				if(i % j == 0){
					// if both divisors are same then
					// add it only once else add both
					if(j == (i / j)) {
						sum += j;
					} else {
						sum += (j + i / j);
					}
				}
			}

			pthread_mutex_lock(&index_mutex);
			vsote[i] = sum;
			pthread_mutex_unlock(&index_mutex);
		}
	}

	return NULL;
}

void *pari(void *arg) {

	int rank = (int)arg;
	int start = (int)(N / THREADS * rank);
	int stop = (int)(N / THREADS * (rank + 1));

	for(int i = start; i < stop; i++) {

		int a = vsote[i];
		int b; 
		
		// omejitev, da ne gremo čez mejo niza (nekatera števila imajo vsoto deljiteljev > N)
		if(a <= N) {

			b = vsote[a];

			if(i == b && a != b) {
				pthread_mutex_lock(&index_mutex);

				sum_all += (a + b);

				// nastavimo na -1, da ne štejemo dvojnih vrednosti
				vsote[a] = -1;

				pthread_mutex_unlock(&index_mutex);
			}
		}
	}

	return NULL;
}

int main() {

	// vsota_staticno_enakomerno

	start = omp_get_wtime();

	for(int i = 0; i < THREADS; i++) {
		pthread_create(&thread[i], NULL, vsota_staticno_enakomerno, (void *)i);
	}

	for(int i = 0; i < THREADS; i++) {
		pthread_join(thread[i], NULL);
	}

	end = omp_get_wtime(); 

	printf("Staticno enakomerno:  %f seconds\n", end - start);

	// vsota_staticno_krozno

	start = omp_get_wtime();

	for(int i = 0; i < THREADS; i++) {
		pthread_create(&thread[i], NULL, vsota_staticno_krozno, (void *)i);
	}

	for(int i = 0; i < THREADS; i++) {
		pthread_join(thread[i], NULL);
	}

	end = omp_get_wtime(); 

	printf("Staticno krozno:      %f seconds\n", end - start);

	// vsota_dinamicno

	start = omp_get_wtime();

	for(int i = 0; i < THREADS; i++) {
		pthread_create(&thread[i], NULL, vsota_dinamicno, (void *)i);
	}

	for(int i = 0; i < THREADS; i++) {
		pthread_join(thread[i], NULL);
	}

	end = omp_get_wtime();

	printf("Dinamicno (Np=%d):  %f seconds\n", PACK_SIZE, end - start);

	// rezultat (končna vsota)

	for(int i = 0; i < THREADS; i++) {
		pthread_create(&thread[i], NULL, pari, (void *)i);
	}

	for(int i = 0; i < THREADS; i++) {
		pthread_join(thread[i], NULL);
	}

	printf("Vsota = %d\n", sum_all);
	
    return 0;
}

/*
	Rezultat naloge:

	ŠTEVILO		STATIČNO	STATIČNO	DINAMIČNO	DINAMIČNO	DINAMIČNO
	NITI		ENAKOMERNO	KROŽNO (Np=1)	(Np=10)		(Np=100)	(Np=1000)

	1		196.573822 s	196.630018 s	196.565986 s	196.135025 s	207.194485 s

	2		128.373098 s	 99.222416 s	 98.592155 s	 98.540943 s	102.382426 s

	4		 70.560009 s	 50.320708 s	 49.882212 s	 49.600952 s	 52.115071 s

	8		 37.082304 s	 25.470679 s	 25.083503 s	 25.217189 s	 25.999786 s

	16		 19.059653 s	 13.419039 s	 13.290676 s	 13.201291 s	 13.545761 s

	32		 12.518750 s	 10.291471 s	  9.262277 s	  8.410783 s	  9.509691 s

	Pohitritev:

	ŠTEVILO		STATIČNO	STATIČNO	DINAMIČNO	DINAMIČNO	DINAMIČNO
	NITI		ENAKOMERNO	KROŽNO (Np=1)	(Np=10)		(Np=100)	(Np=1000)

	1		  1.00		  1.00		  1.00		  1.00		  1.00

	2		  1.53		  1.98		  1.99		  1.99		  2.02

	4		  2.79		  3.90		  3.94		  3.95		  3.98

	8		  5.30		  7.72		  7.83		  7.78		  7.97

	16		 10.31		 14.65		 17.79		 14.86		 15.29

	32		 15.70		 19.11		 21.23		 23.32		 21.79

*/
