/** 
* @page makedoc Documentazione Makefile
* Possibili parametri per l'esecuzione del comando make all'interno della cartella del progetto
* @section sec Opzioni di lancio
* <b>make</b>: esegue make all<br>
* <b>make all</b>: complia i file sorgenti e crel l'eseguibile 'simulation'<br>
* <b>make clean</b>: elimina tutti i file presenti nella cartella del progetto, esclusi i file sorgenti<br>
* <b>make run</b>: compila i sorgenti ed esegue il programma<br>
* <b>make doc</b>: genera la documentazione nella cartella 'documentation'<br>
* <b>make cleandoc</b>: elimina tutta la documentazione dalla cartella 'documentation'<br>
*/

/**
* @mainpage Moltiplicazione di due matrici quadrate
* <div style="border-bottom: 1px solid #C4CFE5;"><br>Elaborato di Sistemi Operativi<br>
* Anno accademico 2016/2017<br><br></div><br>
* @section files File che compongono il progetto
* @subsection srcs File sorgenti
* <ul>
*   <li>@ref father.c</li>
*   <li>@ref son.c</li>
*   <li>@ref mylib.c</li>
* </ul>
* @subsection headers Headers
* <ul>
*   <li>@ref father.h</li>
*   <li>@ref son.h</li>
*   <li>@ref mylib.h</li>
* </ul>
* @subsection make Makefile
* <ul>
*   <li>@ref makedoc </li>
* </ul>
*/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "son.h"
#include "mylib.h"
#include "father.h"

/** 
* @file 
* Contiene il codice del processo padre, ovvero inizializza le risorse e coordina l'esecuzione delle thread
* @author Nicolò Beato
*/

/**
* @name Variabili del processo
* @{
*/
/**
* @brief file descriptor del file contenente la matrice A
*/
int fdA;
/**
* @brief file descriptor del file contenente la matrice B
*/
int fdB;
/**
* @brief file descriptor del file contenente la matrice C
*/
int fdC;

/**
* @brief mutex per la protezione delle risorse condivise
*/
pthread_mutex_t *mutexA,*mutexB,*mutexC,*mutexSum;

/**
* @brief puntatori alle zone di mem condivisa
*/
int *shmA, *shmB, *shmC, *shmSum;

/**
* @brief ordine della matrice
*/
int matrixOrder;

/**
* @}
*/
/**
* @brief Funzione principale del programma. Si occupa di allocare le risorse IPC, far partire i processi figli ed eseguire il codice del processo padre
*/
int main( int argc, char *argv[] ) {
	int n,sum;
	char temp[100];

	matrixOrder=atoi(argv[4]); /*ordine matrice*/
	
	easywrite("[PADRE]aprendo file\n");
	if ((fdA=open(argv[1],O_RDONLY|O_EXCL,S_IRUSR)) == -1) { /*argv[1]=matA*/
		syserr("errore nell'apertura di matA");
		exit(1);
		}
	if ((fdB=open(argv[2],O_RDONLY|O_EXCL,S_IRUSR)) == -1) { /*argv[2]=matB*/
		syserr("errore nell'apertura di matB");
		exit(1);
		}
	/*elimino contenuto matC*/
	if((truncate(argv[3], 0)) == -1){
		syserr("errore nell'eliminazione contenuto di matC");
		exit(1);
		}
	if ((fdC=open(argv[3],O_WRONLY|O_APPEND|O_EXCL,S_IWUSR)) == -1) { /*argv[3]=matC*/
		syserr("errore nell'apertura di matC");
		exit(1);
		}

	initResources ();
	importMatrix();

	/*inizializzo a 0 shmSum siccome devo ancora creare figli non uso semafori*/
	*(shmSum)=0;

	sendOperations();
	
	printMatrix(fdC);

	/*stampa della somma elementi di C*/
	pthread_mutex_lock (mutexSum);
	sum=*(shmSum);
	pthread_mutex_unlock (mutexSum);
	easywrite("la somma degli elementi della matrice C risulta= ");
	syswriteint(&sum);
	easywrite("\n");
	
	freeResources();
		
	exit(0);
}
/**
* @brief Funzione che copia nelle rispettive aree di memoria condivisa le matrici A e B
*/
void importMatrix () {

	char buffer [BUFFER_SIZE];
	int i, j;
	char* rows [matrixOrder];
	char* col;

	easywrite("[PADRE]salvando in memoria le matrici A e B\n");

/*matrice A*/
	if((read(fdA,buffer,BUFFER_SIZE)) == -1){
		syserr("errore nella lettura file");
		exit(1);
		}

	//leggo il buffer e spezzo una riga alla volta
	i=0;
	rows [i] = strtok(buffer, "\n");
	for (i=1 ; i<matrixOrder ; i++)
		rows [i] = strtok (NULL,"\n");

	//spezzo ogni riga nei token a(i,j)
	for(i=0 ; i<matrixOrder ; i++) {
		col=strtok(rows[i]," ");
		j=0;
		pthread_mutex_lock (mutexA);
		*(shmA + calc_index(i,j,matrixOrder)) = atoi (col);
		pthread_mutex_unlock (mutexA);
		for(j=1 ; j<matrixOrder ; j++) {
			col=strtok(NULL," ");
			pthread_mutex_lock (mutexA);
			*(shmA + calc_index(i,j,matrixOrder)) = atoi(col);
			pthread_mutex_unlock (mutexA);
		}
	}
/*matriceB*/
	if((read(fdB,buffer,BUFFER_SIZE)) == -1){
		syserr("errore nella lettura file");
		exit(1);
		}

	/*leggo il buffer e spezzo una riga alla volta*/
	i=0;
	rows [i] = strtok(buffer, "\n");
	for (i=1 ; i<matrixOrder ; i++)
		rows [i] = strtok (NULL,"\n");

	/*spezzo ogni riga nei token a(i,j)*/
	for(i=0 ; i<matrixOrder ; i++) {
		col=strtok(rows[i]," ");
		j=0;
		pthread_mutex_lock (mutexB);	
		*(shmB + calc_index(i,j,matrixOrder)) = atoi (col);
		pthread_mutex_unlock (mutexB);
		for(j=1 ; j<matrixOrder ; j++) {
			col=strtok(NULL," ");
			pthread_mutex_lock (mutexB);			
			*(shmB + calc_index(i,j,matrixOrder)) = atoi(col);
			pthread_mutex_unlock (mutexB);	
		}
	}
	close(fdA);
	close(fdB);
	easywrite("[PADRE]matrici A e B salvate in memoria e file di testo chiusi\n");
}
/**
* @brief Funzione che inizializza le risorse quali aree di memoria comune e mutex
*/
void initResources (){

	int i;	
	easywrite("[PADRE]inizializzando risorse\n");
	

	//creo zona memoria condivisa
	shmA= (int*) malloc (sizeof(int [(1<<matrixOrder)])); 
	shmB= (int*) malloc (sizeof(int [(1<<matrixOrder)])); 
	shmC= (int*) malloc (sizeof(int [(1<<matrixOrder)])); 
	shmSum= (int*) malloc (sizeof(int));

	/*inizializzazione mutex*/
	mutexA = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	if((pthread_mutex_init(mutexA, NULL)) == -1){
		syserr("Impossibile creare mutexA, terminazione...\n\n");
		//closeResources(1, sons, NULL);
		exit(1);
		}
	mutexB = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	if((pthread_mutex_init(mutexB, NULL)) == -1){
		syserr("Impossibile creare mutexB, terminazione...\n\n");
		//closeResources(1, sons, NULL);
		exit(1);
		}
	mutexC = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	if((pthread_mutex_init(mutexC, NULL)) == -1){
		syserr("Impossibile creare mutexC, terminazione...\n\n");
		//closeResources(1, sons, NULL);
		exit(1);
		}
	mutexSum = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	if((pthread_mutex_init(mutexSum, NULL)) == -1){
		syserr("Impossibile creare mutexSum, terminazione...\n\n");
		//closeResources(1, sons, NULL);
		exit(1);
		}
	

	easywrite("[PADRE]risorse inizializzate\n");
}
/**
* @brief Funzione che si occupa di creare le thread e inviare loro le operazioni da eseguire
*/
void sendOperations(){
	int i,j,n,k,rc,t=0;
	pthread_attr_t attr;

	pthread_t threadsMulti[(1<<matrixOrder)];
	pthread_t threadsSum[matrixOrder];
	char command[BUFFER_SIZE];
	char commandMulti[(1<<matrixOrder)] [BUFFER_SIZE];
	char commandSum [matrixOrder] [BUFFER_SIZE];

	pthread_attr_init(&attr);
   	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	easywrite("[PADRE]inviando operazioni\n");
	easywrite("[PADRE->OPERAZIONI]inviando moltiplicazioni\n");

	//creazione array moltiplicazioni
	for(i=0;i<matrixOrder;i++){
		for(j=0;j<matrixOrder;j++){
			sprintf(command,"M %d %d",i,j);
			strcpy(commandMulti[t++],command);
		}
	}

	/*moltiplicazione*/
	t=0;
	for(i=0;i<matrixOrder;i++){
		for(j=0;j<matrixOrder;j++){
			if((rc=pthread_create(&threadsMulti[t], &attr, sonSimulation,&commandMulti[t++])) == -1) {
				syserr("Generazione threads: errore durante la creazione di una thread");
				exit(1);
				}	
		}}

	easywrite("[PADRE->OPERAZIONI]moltiplicazioni effuate\n");	

	for(n=0;n<=t;n++) {/*controllo che tutti i figli abbiano terminato le moltiplicazioni*/
		void *status;
		pthread_join (threadsMulti[n], &status);
	}
		
	easywrite("[PADRE->OPERAZIONI]inviando somme\n");
/*somma*/
	for(k=0;k<matrixOrder;k++){
		sprintf(command,"S %d",k);
		strcpy(commandSum[k],command);
		}
	
	for(k=0;k<matrixOrder;k++){
			if((rc=pthread_create( &threadsSum[k], &attr, sonSimulation,&commandSum[k])) == -1) {
				syserr("Generazione threads: errore durante la creazione di una thread");
				exit(1);
				} 
	}

	easywrite("[PADRE->OPERAZIONI]somme inviate\n");
	easywrite("[PADRE]inviate tutte le operazioni\n");

	for(n=0;n<matrixOrder;n++){/*controllo che tutti i figli abbiano terminato le somme*/
		void *status;
		pthread_join (threadsSum[n],&status);
		}
}
/**
* @brief Funzione che si occupa di scrivere all'interno del file la matrice C risultante dalle operazioni eseguite
*/
void printMatrix(){
	int i,j,c;
	easywrite("[PADRE]scrivendo matrice C nel file di testo\n");
	for (i=0;i<matrixOrder;i++){
		for(j=0;j<matrixOrder;j++){
			pthread_mutex_lock (mutexC);
			c=*(shmC + calc_index(i,j,matrixOrder));
			pthread_mutex_unlock (mutexC);
			char temp[10];//stringa in cui viene copiata la stringa contenente c(i,j)
			itoa(c, temp);
			if ((write(fdC, temp, strlen(temp))) == -1) {
				syserr("padre: write su file C fallito");
				exit(1);
				}
			if ((write(fdC, " ", 1)) == -1) {
				syserr("padre: write su file C fallito");
				exit(1);
				}
		}
		if ((write(fdC, "\n", 1)) == -1) {
				syserr("padre: write su file C fallito");
				exit(1);
				}
	}
	close(fdC);
	easywrite("[PADRE]matrice scritta e file chiuso\n");	
}
/**
* @brief Funzione che libera le risorse allocate
*/
void freeResources(){
	free (shmA);
   	free (shmB);
	free (shmC);
	free (shmSum);

	pthread_mutex_destroy(mutexA);
	pthread_mutex_destroy(mutexB);
	pthread_mutex_destroy(mutexC);
	pthread_mutex_destroy(mutexSum);
	easywrite("[PADRE]risorse liberate\n");				
}
/**
* @brief Funzione restituisce il mutex per A
*/
pthread_mutex_t *getMutexA(){
	return mutexA;
}
/**
* @brief Funzione restituisce il mutex per B
*/
pthread_mutex_t *getMutexB(){
	return mutexB;
}
/**
* @brief Funzione restituisce il mutex per C
*/
pthread_mutex_t *getMutexC(){
	return mutexC;
}
/**
* @brief Funzione restituisce il mutex per la somma delle righe di C
*/
pthread_mutex_t *getMutexSum(){
	return mutexSum;
}
/**
* @brief Funzione restituisce la matrice A
*/
int* getMemA(){
	return shmA;
}
/**
* @brief Funzione restituisce la matrice B
*/
int* getMemB(){
	return shmB;
}
/**
* @brief Funzione restituisce la matrice C
*/
int* getMemC(){
	return shmC;
}
/**
* @brief Funzione restituisce la somma delle righe di C
*/
int* getMemSum(){
	return shmSum;
}
/**
* @brief Funzione restituisce l'ordine delle matrici
*/
int getOrder(){
	return matrixOrder;
}	


