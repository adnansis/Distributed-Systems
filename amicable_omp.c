// Prevajanje programa: 
//				gcc -O2 -lm -fopenmp -w -o amicable_omp amicable_omp.c
// Zagon programa: 
//				srun -n1 --reservation=fri --cpus-per-task=32 ./amicable_omp

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <omp.h>

#define N 10000000
#define THREADS 32
#define PACK_SIZE 1000

int sum_all = 0;

int vsote[N+1];

double start, end;

void vsota_staticno_enakomerno() {

    #pragma omp parallel for schedule(static, N/THREADS)
	for(int i = 0; i < N; i++) {

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

		vsote[i] = sum;
	}

}

void vsota_staticno_krozno() {

    #pragma omp parallel for schedule(static, PACK_SIZE)
	for(int i = 0; i < N; i++) {

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

		vsote[i] = sum;
	}

}

void vsota_dinamicno() {

    #pragma omp parallel for schedule(dynamic, PACK_SIZE)
    for(int i = 0; i < N; i++) {

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

        vsote[i] = sum;
    }

}

void pari() {

    #pragma omg parallel for schedule(static, N/THREADS)
	for(int i = 0; i < N; i++) {

		int a = vsote[i];
		int b; 
		
		// omejitev, da ne gremo čez mejo niza (nekatera števila imajo vsoto deljiteljev > N)
		if(a <= N) {

			b = vsote[a];

			if(i == b && a != b) {
				//pthread_mutex_lock(&index_mutex);
                #pragma omp critical
                {
                    sum_all += (a + b);

				    // nastavimo na -1, da ne štejemo dvojnih vrednosti
				    vsote[a] = -1;
                }

				//pthread_mutex_unlock(&index_mutex);
			}
		}
	}

}

int main() {

	// vsota_staticno_enakomerno

	start = omp_get_wtime();

    vsota_staticno_enakomerno();

	end = omp_get_wtime(); 

	printf("Staticno enakomerno:  %f s\n", end - start);

	// vsota_staticno_krozno

	start = omp_get_wtime();

	vsota_staticno_krozno();

	end = omp_get_wtime(); 

	printf("Staticno krozno (Np=%d):      %f s\n", PACK_SIZE, end - start);

	// vsota_dinamicno

	start = omp_get_wtime();

	vsota_dinamicno();

	end = omp_get_wtime();

	printf("Dinamicno (Np=%d):  %f s\n", PACK_SIZE, end - start);

	// rezultat (končna vsota)

    pari();

	printf("Vsota = %d\n", sum_all);
	
    return 0;
}

/*
	Rezultat naloge:

	ŠTEVILO		STATIČNO		STATIČNO		STATIČNO			DINAMIČNO		DINAMIČNO		DINAMIČNO
	NITI		ENAKOMERNO		KROŽNO (Np=1)	KROŽNO (Np=1000)	(Np=10)			(Np=100)		(Np=1000)

	1			195.657474 s	198.337574 s	195.526645 s		198.321990 s	198.395531 s	198.326799 s

	2			127.962643 s	 99.671310 s	 97.969388 s 		 99.453069 s	 99.445326 s	 99.470047 s

	4		 	 69.997175 s	 50.587002 s	 49.142510 s		 49.932803 s	 49.887579 s	 49.923096 s

	8		 	 36.770868 s	 34.163064 s	 24.633058 s		 25.507659 s	 24.945291 s	 24.984201 s

	16		 	 18.739677 s	 12.653842 s	 12.358463 s		 12.569809 s	 12.555798 s	 12.540080 s

	32		 	 12.751069 s	 11.764138 s	  7.472528 s		  6.834785 s	  7.152081 s	  7.197113 s




	ŠTEVILO		STATIČNO	STATIČNO		STATIČNO			DINAMIČNO	DINAMIČNO	DINAMIČNO
	NITI		ENAKOMERNO	KROŽNO (Np=1)	KROŽNO (Np=1000)	(Np=10)		(Np=100)	(Np=1000)

	1		  	  1.00		  1.00		  	1.00				1.00		  1.00		  1.00

	2		  	  1.53		  1.98		  	2.00				1.99		  1.99		  2.02

	4		  	  2.79		  3.92		  	3.98				4.01		  3.97		  3.97

	8		  	  5.30		  5.80		  	7.94				7.77		  7.95		  7.94

	16		 	  10.44		 15.68		   15.82			   15.77		 15.80		 15.81

	32		 	  15.34		 16.99		   26.18			   29.04		 27.75		 27.58

*/