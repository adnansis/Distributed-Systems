#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
/*
	Definiramo vrata (port) na katerem bo strežnik poslušal
	in velikost medponilnika za sprejemanje in pošiljanje podatkov
*/
#define PORT 10420
#define BUFFER_SIZE 256
#define MAX_CONNECTIONS 2

//Spremenjlivka za preverjane izhodnega statusa funkcij
int iResult;

//Definiramo nov vtič in medpomnilik
int clientSock;
char buff[BUFFER_SIZE];

//Število povezav
int num_of_connections = 0;

//Nova nit
pthread_t thread;

//Struktura za prenašanje argumentov
typedef struct Argument {
	int clientSock;
} args;

//Mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *new_conn(void *arg) {
	//Preberi argument in shrani v clientSocket
	int clientSock = ((args*)arg)->clientSock;

	//Postrezi povezanemu klientu
	do {
		//Sprejmi podatke
		iResult = recv(clientSock, buff, BUFFER_SIZE, 0);
		if(iResult > 0) {
			printf("Bytes received: %d\n", iResult);

			//Vrni prejete podatke pošiljatelju
			iResult = send(clientSock, buff, iResult, 0 );
			if(iResult == -1) {
				printf("send failed!\n");
				close(clientSock);
				break;
			}
			printf("Bytes sent: %d\n", iResult);
		}
		else if(iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed!\n");
			close(clientSock);
			break;
		}

	} while(iResult > 0);

	//Zmanjšaj število povezav
	pthread_mutex_lock(&lock);
	num_of_connections--;
	pthread_mutex_unlock(&lock);

	//Zapri povezavo
	close(clientSock);

	return NULL;
}

int main(int argc, char **argv) {
	/*
		Ustvarimo nov vtič, ki bo poslušal
		in sprejemal nove kliente preko TCP/IP protokola
	*/
	int listener = socket(AF_INET, SOCK_STREAM, 0);
	if(listener == -1) {
		printf("Error creating socket\n");
		return 1;
	}

	//Nastavimo vrata in mrežni naslov vtiča
	sockaddr_in listenerConf;
	listenerConf.sin_port = htons(PORT);
	listenerConf.sin_family = AF_INET;
	listenerConf.sin_addr.s_addr = INADDR_ANY;

	//Vtič povežemo z ustreznimi vrati
	iResult = bind(listener, (sockaddr *)&listenerConf, sizeof(listenerConf));
	if(iResult == -1) {
		printf("Bind failed\n");
		close(listener);
		return 1;
    }

	//Začnemo poslušati
	if(listen(listener, 5) == -1) {
		printf("Listen failed\n");
		close(listener);
		return 1;
	}
	
	/*
		V zanki sprejemamo nove povezave
		in jih strežemo (največ eno naenkrat)
	*/
	while(1){
		//Sprejmi povezavo in ustvari nov vtič
		clientSock = accept(listener, NULL, NULL);
		if (clientSock == -1) {
			printf("Accept failed\n");
			close(listener);
			return 1;
		}

		//Preveri število povezav
		if(num_of_connections >= MAX_CONNECTIONS) {
			printf("Someone tried to connect to the server but user limit was exceeded\n");
			close(clientSock);
		} else {
			//Za vsako novo povezavo naredi nov argument (clientSocket)
			args a;
			a.clientSock = clientSock;

			pthread_create(&thread, NULL, new_conn, (void*)(&a));
			pthread_detach(thread);

			//Povečaj število povezav
			pthread_mutex_lock(&lock);
			num_of_connections++;
			pthread_mutex_unlock(&lock);
		}
	}

	//Počistimo vse vtiče
	close(listener);

	return 0;
}