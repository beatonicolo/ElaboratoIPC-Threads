#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "father.h"
#include "mylib.h"
#include "son.h"

/** 
* @file 
* Contiene le funzioni che verranno usate dalle thread
*/
/**
* @brief mutex per la protezione di A
*/
pthread_mutex_t *mutexA;
/**
* @brief mutex per la protezione di B
*/
pthread_mutex_t *mutexB;
/**
* @brief mutex per la protezione di C
*/
pthread_mutex_t *mutexC;
/**
* @brief mutex per la protezione della somma delle righe di C
*/
pthread_mutex_t *mutexSum;
/**
* @brief puntator ad A
*/
int *shmA;
/**
* @brief puntatore a B
*/
int *shmB;
/**
* @brief puntatore a C
*/
int *shmC;
/**
* @brief puntatore alla somma delle righe di C
*/
int *shmSum;
/**
* @brief ordine delle matrici
*/
int matrixOrder;

/**
* @}
*/
/**
* @brief Funzione che contiene il corpo del codice delle thread
* 
* @param *comm stringa contenente comando da eseguire
*/
void *sonSimulation(void *comm){

	message *m;
	char command [BUFFER_SIZE];
	char temp[100],cmd;

	strcpy(command,(char*) comm);

	getSonResources();


	int i,j,k;
	easywrite("[THREAD] decodificando operazione ricevuta\n");

	/*DECIDE L'ORPERAZIONE DA ESEGUIRE*/
	cmd=command[0];
	strtok(command," ");
	switch (cmd) 
	{
		case 'M':
				i= atoi(strtok(NULL," "));
				j= atoi(strtok(NULL," "));
				doMultiplication(i,j);
				break;
			case 'S':
				k= atoi(strtok(NULL," "));
				doSum(k);
				break;
			default:
				syserr("figlio: ricevuto comando non corretto");
				exit(1);
		}
		closeMyself();

}

/**
* @brief Acquisisce le risorse necessarie, come matrici,ordine e mutex
*/
void getSonResources(){
	
	easywrite("[THREAD] Recuperando risorse\n");

	shmA = (int *) (intptr_t) getMemA();
	shmB = (int *) (intptr_t) getMemB();
	shmC = (int *) (intptr_t) getMemC();
	shmSum = (int *) (intptr_t) getMemSum();
	mutexA = (pthread_mutex_t *) (intptr_t) getMutexA();
	mutexB = (pthread_mutex_t *) (intptr_t) getMutexB();;
	mutexC = (pthread_mutex_t *) (intptr_t) getMutexC();
	mutexSum = (pthread_mutex_t *) (intptr_t) getMutexSum();

	matrixOrder=getOrder();

	easywrite("[THREAD] risorse recuperate\n");
}

/**
* @brief Esegue la moltiplicazione
* @param i i-esima riga di A
* @param j j-esima colonna di B
*/
void doMultiplication(int i,int j){

	char temp [100];
	int row[matrixOrder];
	int column[matrixOrder];
	int n,c;

	easywrite("[THREAD] eseguendo moltiplicazione\n");
/*leggo i-esima riga matrice A*/
	for(n=0;n<matrixOrder;n++){
		pthread_mutex_lock (mutexA);
		row[n]=*(shmA + calc_index(i,n,matrixOrder));
		pthread_mutex_unlock (mutexA);
	}

/*leggo j-esima colonna della matrice B*/
	for(n=0;n<matrixOrder;n++){
		pthread_mutex_lock (mutexB);
		column[n]=*(shmB + calc_index(n,j,matrixOrder));
		pthread_mutex_unlock (mutexB);
	}

	for(n=0;n<matrixOrder;n++)
		c+= row[n]*column[n];
/*scrivo c(i,j)*/
	pthread_mutex_lock (mutexC);
	*(shmC + calc_index(i,j,matrixOrder)) = c;
	pthread_mutex_unlock (mutexC);

	easywrite("[THREAD] moltiplicazione eseguita\n");
}
/**
* @brief Esegue la somma
* @param k k-esima riga di C
*/
void doSum(int k){

	int row[matrixOrder];
	int sumk = 0, n,t;
	char temp[100];

	easywrite("[THREAD] eseguendo somma\n");
/*leggo k-esima riga di C*/	
	for (n=0;n<matrixOrder;n++) {
		pthread_mutex_lock (mutexC);
		row[n] = *(shmC + calc_index(k,n,matrixOrder));
		pthread_mutex_unlock (mutexC);
	}
/*sommo i suoi elementi*/
	for (n=0;n<matrixOrder;n++)
		sumk += row[n];
/*aggiorno il valore*/		
	pthread_mutex_lock (mutexSum);
	*(shmSum)+=sumk;
	pthread_mutex_unlock (mutexSum);

	easywrite("[THREAD] somma eseguita\n");
}
/**
* @brief Funzione che termina la thread
*/
void closeMyself(){

	easywrite("[THREAD] risorse liberate. STO TERMINANDO... . . .  .  .   .    .\n");
	pthread_exit(NULL);

}




























