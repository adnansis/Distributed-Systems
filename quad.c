// Prevajanje programa: 
//				gcc -O2 quad.c -fopenmp -lm -o quad
// Zagon programa: 
//				srun -n1 --reservation=fri --cpus-per-task=32 ./quad

#include <stdio.h>
#include <math.h>
#include <omp.h>

#define TOL 1e-8

#define THREADS 32

double func(double x) {
    return sin(x*x);
}

// funkcija za integracijo, spodnja meja, zgornja meja, dovoljena napaka
double quad(double (*f)(double), double lower, double upper, double tol) {

    double quad_res;        // rezultat
    double h;               // dolzina intervala
    double middle;          // sredina intervala
    double quad_coarse;     // groba aproksimacija
    double quad_fine;       // fina aproksimacija (two trapezoids)
    double quad_lower;      // rezultat na spodnjem intervalu 
    double quad_upper;      // rezultat na zgornjem intervalu
    double eps;             // razlika

    h = upper - lower;
    middle = (lower + upper) / 2;

    // izracunaj integral z obema aproksimacijama trapezoidnega pravila 
    quad_coarse = h * (f(lower) + f(upper)) / 2.0; // na celem intervalu
    quad_fine = h/2 * (f(lower) + f(middle)) / 2.0 + h/2 * (f(middle) + f(upper)) / 2.0; // seštevek dveh polovic
    eps = fabs(quad_coarse - quad_fine);
    
    // ce se nismo dosegli zelene natancnosti, razdelimo interval na pol in ponovimo
    if (eps > tol) {
        #pragma omp task shared(quad_lower) final(h < 1.0)
        quad_lower = quad(f, lower, middle, tol / 2);
        quad_upper = quad(f, middle, upper, tol / 2);
        #pragma omp taskwait
        quad_res = quad_lower + quad_upper;
    } else {
        quad_res = quad_fine;
    }

    return quad_res;
}

int main(int argc, char* argv[]) {
    double quadrature;
    double dt = omp_get_wtime();

    omp_set_num_threads(THREADS);

    #pragma omp parallel
    #pragma omp master
    quadrature = quad(func, 0.0, 50.0, TOL);

    dt = omp_get_wtime() - dt;

    printf("Integral: %lf\nCas: %lf s\n", quadrature, dt);

    return 0;
}

/*

Rezultat naloge:

	ŠT. NITI		ČAS     	POHITRITEV				

	1			23.715206 s         1.00

	2			12.777247 s         1.86

	4		 	 6.903310 s         3.44

	8		 	 4.487764 s         5.29

	16		 	 2.441063 s         9.72

	32		 	 1.563141 s        15.20

*/