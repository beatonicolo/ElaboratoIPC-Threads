#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/signal.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "father.h"
#include "mylib.h"
#include "son.h"

/** 
* @file 
* Contiene le funzioni che verranno usate dai processi figli
*/

/**
* @name Variabili del processo
* @{
*/
/**
* @brief identificatori segmenti memoria condivisa
*/
int idA,idB,idC,idSum;
/**
* @brief identificatore array semafori
*/
int idSem;
/**
* @brief identificatore coda messaggi	
*/
int idMsg;
/**
* @brief puntatori alle zone di mem condivisa
*/
int *shmA, *shmB, *shmC, *shmSum;
/**
* @brief ordine matrice
*/
int matrixOrder;
/**
* @brief strutture per le operazioni sui semafori
*/
struct sembuf wait_b,signal_b;
/**
* @brief identificatore figlio
*/
int sonid;
/**
* @brief pipe
*/
int *p;
/**
* @}
*/
/**
* @brief Funzione che contiene il corpo del codice del figlio
* 
* @param id indice del figlio (es: se ind=1 l'etichetta del figlio sarà [NODO 1])
* @param order ordine della matrice
* @param pipe puntatore alla pipe
*/
void sonSimulation(int id, int order, int *pipe){

	message *m;
	char command [BUFFER_SIZE],temp[100],cmd;

	sonid=id;
	matrixOrder=order;

	m= (message *) malloc (sizeof(message));

	getSonResources(pipe);

	while(1) {

		int i,j,k;

	/*INVIO TRAMITE CODA AL PADRE UN MESSAGGIO PER AVVISARLO CHE SONO LIBERO */
		m->mtype=MSGTYPE;
		m->sender=sonid;
		if(msgsnd(idMsg, m, sizeof(message), 0) == -1){
			perror("invio msg fallito");
			exit(1);
			}

	/*LEGGE IL COMANDO INVIATO DAL PADRE*/
		if (read (p[0 + sonid*2], command, BUFFER_SIZE) == -1) {
					perror("figlio: read fallita");
					exit(1);
					}
	sprintf(temp, "[FIGLIO %i] decodificando operazione ricevuta\n",sonid);
	easywrite(temp);
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

			case 'C':
				closeMyself();
			default:
				syserr("figlio: ricevuto comando non corretto");
				exit(1);
		}

	}
}

/**
* @brief Acquisisce le risorse del figlio: semafori, mememoria condivisa,pipe,coda di messaggi
*/
void getSonResources(int *pipe){
	
	char temp [100];
	sprintf(temp, "[FIGLIO %i] Recuperando risorse\n",sonid);
	easywrite(temp);

	/*recupero pipe*/
	p=pipe;

	/*recupero coda di messaggi*/
	if ((idMsg=msgget(MSGKEY,0666)) == -1) {
		syserr("recupero coda msg fallita");
		exit(1);
		}
	
	/*recupero memoria condivisa*/
	if((idA=shmget(SHMKEY_A,sizeof(int [ (1<<matrixOrder) ]) ,0666)) == -1) {
		syserr("recupero shmem A fallita!");
		exit(1);
		}

	/*attacco memoria condivisa*/
	if((shmA= shmat(idA,NULL,0666))== (int*) -1) {
		syserr("figlio: errore attaccamento area memoria condivisa A");
		exit(1);
		}
	
	/*recupero memoria condivisa*/
	if((idA=shmget(SHMKEY_B,sizeof(int [ (1<<matrixOrder) ]) ,0666)) == -1) {
		syserr("recupero shmem B fallita!");
		exit(1);
		}

	/*attacco memoria condivisa*/
	if((shmB= shmat(idB,NULL,0666))== (int*) -1) {
		syserr("figlio: errore attaccamento area memoria condivisa B");
		exit(1);
		}
		
	/*recupero memoria condivisa*/
	if((idC=shmget(SHMKEY_C,sizeof(int [ (1<<matrixOrder) ]) ,0666)) == -1) {
		syserr("recupero shmem C fallita!");
		exit(1);
		}

	/*attacco memoria condivisa*/
	if((shmC= shmat(idC,NULL,0666))== (int*) -1) {
		syserr("figlio: errore attaccamento area memoria condivisa A");
		exit(1);
		}

	/*mem condivisa somma*/
	if((idSum=shmget(SHMKEY_SUM,sizeof(int) ,0666)) == -1) {
		syserr("recupero shmem SUM fallita!");
		exit(1);
		}

	/*attacco memoria condivisa*/
	if((shmSum= shmat(idSum,NULL,0666))== (int*) -1) {
		syserr("figlio: errore attaccamento area memoria condivisa C");
		exit(1);
		}

	/*recupero array semafori*/
	if((idSem=semget(SEMKEY,5,0666)) == -1) {
		syserr("recupero semafori fallito");
		exit(1);
		}

	sprintf(temp, "[FIGLIO %i] risorse recuperate\n",sonid);
	easywrite(temp);
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

	sprintf(temp, "[FIGLIO %i] eseguendo moltiplicazione\n",sonid);
	easywrite(temp);
/*leggo i-esima riga matrice A*/
	for(n=0;n<matrixOrder;n++){
		sem_wait(idSem,SEM_A);
		row[n]=*(shmA + calc_index(i,n,matrixOrder));
		sem_signal(idSem,SEM_A);
	}

/*leggo j-esima colonna della matrice B*/
	for(n=0;n<matrixOrder;n++){
		sem_wait(idSem,SEM_B);
		column[n]=*(shmB + calc_index(n,j,matrixOrder));
		sem_signal(idSem,SEM_B);
	}

	for(n=0;n<matrixOrder;n++)
		c+= row[n]*column[n];
/*scrivo c(i,j)*/
	sem_wait(idSem,SEM_C);
	*(shmC + calc_index(i,j,matrixOrder)) = c;
	sem_signal(idSem,SEM_C);

	sem_signal(idSem,SEM_MUL);/*incremento semaforo che conta moltiplicazioni*/

	sprintf(temp, "[FIGLIO %i] moltiplicazione eseguita\n",sonid);
	easywrite(temp);
}
/**
* @brief Esegue la somma
* @param k k-esima riga di C
*/
void doSum(int k){

	int row[matrixOrder];
	int sumk = 0, n,t;
	char temp[100];

	sprintf(temp, "[FIGLIO %i] eseguendo moltiplicazione\n",sonid);
	easywrite(temp);
/*leggo k-esima riga di C*/	
	for (n=0;n<matrixOrder;n++) {
		sem_wait(idSem,SEM_C);
		row[n] = *(shmC + calc_index(k,n,matrixOrder));
		sem_signal(idSem,SEM_C);
	}
/*sommo i suoi elementi*/
	for (n=0;n<matrixOrder;n++)
		sumk += row[n];
/*aggiorno il valore*/		
	sem_wait(idSem,SEM_SUM);
	*(shmSum)+=sumk;
	sem_signal(idSem,SEM_SUM);

	sprintf(temp, "[FIGLIO %i] somma eseguita\n",sonid);
	easywrite(temp);
}
/**
* @brief Funzione che libera le risorse acquisite dal processo figlio e alla fine lo fa terminare
*/
void closeMyself(){
	
	char temp [100];

	sprintf(temp, "[FIGLIO %i] liberando risorse\n",sonid);
	easywrite(temp);
/* chiudo shmA*/
	if(shmdt(shmA) == -1){
		sprintf(temp, "[FIGLIO %i] Memoria condivisa: impossibile disconnettersi dalla zona di memoria condivisa A", sonid);
		syserr(temp);
		}
/* chiudo shmB*/
	if(shmdt(shmB) == -1){
		sprintf(temp, "[FIGLIO %i] Memoria condivisa: impossibile disconnettersi dalla zona di memoria condivisa B", sonid);
		syserr(temp);
		}
/* chiudo shmC*/
	if(shmdt(shmC) == -1){
		sprintf(temp, "[FIGLIO %i] Memoria condivisa: impossibile disconnettersi dalla zona di memoria condivisa C", sonid);
		syserr(temp);
		}
/* chiudo shmSUM*/
	if(shmdt(shmSum) == -1){
		sprintf(temp, "[FIGLIO %i] Memoria condivisa: impossibile disconnettersi dalla zona di memoria condivisa SUM", sonid);
		syserr(temp);
		}

	sprintf(temp, "[FIGLIO %i] risorse liberate. STO TERMINANDO... . . .  .  .   .    .\n",sonid);
	easywrite(temp);

	exit(0);
}




























