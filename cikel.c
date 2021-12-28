// Prevajanje programa: 
//              module load mpi
//				mpicc cikel.c -o cikel
// Zagon programa: 
//				srun --mpi=pmix -n10 --reservation=fri cikel

#include <stdio.h>
#include <string.h> 
#include <mpi.h>

int main(int argc, char *argv[]) {

	int my_rank; // rank (oznaka) procesa 
	int num_of_processes; // število procesov 
	int source; // rank pošiljatelja 
	int destination; // rank sprejemnika 
	int tag = 0; // zaznamek sporoèila 
	char message[100]; // rezerviran prostor za sporoèilo
    char buff[5];
	MPI_Status status; // status sprejema 

	MPI_Init(&argc, &argv); // inicializacija MPI okolja 
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); // poizvedba po ranku procesa 
	MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes); // poizvedba po številu procesov

    // procesi z rankom != 0 prejmejo sporocilo, dodajo svoj rank in posljejo naslednjem procesu
	if( my_rank != 0 ) {

        MPI_Recv(message, 100, MPI_CHAR, my_rank-1, tag, MPI_COMM_WORLD, &status); 

        // dodajanje my_rank v message
        sprintf(buff, "%d-\0", my_rank);
        strcat(message, buff);

        // dolocanje naslova za posiljanje sporocila
		if(my_rank < num_of_processes - 1)
            destination = my_rank + 1;
        else
            destination = 0;
        
		MPI_Send(message, (int)strlen(message)+1, MPI_CHAR, destination, tag, MPI_COMM_WORLD); 

	} else { 
        // proces z my_rank == 0 zacne verigo posiljanja sporocil
        // dodajanje my_rank v sporocilo
        sprintf(message, "%d-\0", my_rank);

        // prvo sporocilo procesa 0
        MPI_Send(message, (int)strlen(message)+1, MPI_CHAR, 1, tag, MPI_COMM_WORLD); 

        // zadnje sporocilo zadnjega procesa
		MPI_Recv(message, 100, MPI_CHAR, num_of_processes-1, tag, MPI_COMM_WORLD, &status);

        // dodajanje my_rank na konec sporocila
        sprintf(buff, "%d\0", my_rank);
        strcat(message, buff);

        // izpisemo celotno sporocilo
		printf("%s\n", message); 
		fflush(stdout);
	} 

	MPI_Finalize(); 

	return 0; 
}