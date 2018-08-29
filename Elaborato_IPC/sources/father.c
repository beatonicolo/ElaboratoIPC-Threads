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
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/signal.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "son.h"
#include "mylib.h"
#include "father.h"

/** 
* @file 
* Contiene il codice del processo padre, ovvero inizializza le risorse e coordina l'esecuzione dei figli
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
* @brief pipe
*/
int *p;
/**
* @brief ordine della matrice
*/
int matrixOrder;
/**
* @brief numero di processi da creare
*/
int procsNumber;
/**
* @brief strutture per le operazioni sui semafori
*/
struct sembuf wait_b,signal_b;
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
	procsNumber=atoi(argv[5]); /*numero processi da creare*/
	
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

	createSons();
	sendOperations();

	/*attendo terminazione tutti i figli*/
	for(n=0;n<procsNumber;n++){
		int status;
		status=waitpid(-1,NULL,0);
		sprintf(temp, "[PADRE] figlio con pid=%d ha terminato\n",status);
		easywrite(temp);
	}
	
	printMatrix(fdC);

	/*stampa della somma elementi di C*/
	sem_wait(idSem,SEM_SUM);
	sum=*(shmSum);
	sem_signal(idSem,SEM_SUM);
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
		sem_wait(idSem,SEM_A);	
		*(shmA + calc_index(i,j,matrixOrder)) = atoi (col);
		sem_signal(idSem,SEM_A);
		for(j=1 ; j<matrixOrder ; j++) {
			col=strtok(NULL," ");
			sem_wait(idSem,SEM_A);
			*(shmA + calc_index(i,j,matrixOrder)) = atoi(col);
			sem_signal(idSem,SEM_A);
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
		sem_wait(idSem,SEM_B);	
		*(shmB + calc_index(i,j,matrixOrder)) = atoi (col);
		sem_signal(idSem,SEM_B);
		for(j=1 ; j<matrixOrder ; j++) {
			col=strtok(NULL," ");
			sem_wait(idSem,SEM_B);			
			*(shmB + calc_index(i,j,matrixOrder)) = atoi(col);
			sem_signal(idSem,SEM_B);	
		}
	}
	close(fdA);
	close(fdB);
	easywrite("[PADRE]matrici A e B salvate in memoria e file di testo chiusi\n");
}
/**
* @brief Funzione che inizializza le risorse quali aree di memoria condivisa,semafori,pipe,coda di messaggi 
*/
void initResources (){

	int sem_stato;
	
	/*creazione pipe*/
	p = malloc (sizeof(int [procsNumber])*2 );
	int i;	

	easywrite("[PADRE]inizializzando risorse\n");

	for (i=0;i<procsNumber;i++) {
		if (pipe(p + i*2) == -1) {
			syserr("errore apertura pipe");
			exit(1);
			}
	}

	/* creazione coda di messaggi*/
	if ((idMsg=msgget(MSGKEY, (0666|IPC_CREAT|IPC_EXCL))) == -1) {
		syserr("creazione coda msg fallita");
		exit(1);
		}
	
	/*creazione memoria condivisa*/
	if((idA=shmget(SHMKEY_A,sizeof(int [ (1<<matrixOrder) ]) ,0666|IPC_CREAT|IPC_EXCL)) == -1) {
		syserr("creazione shmem A fallita!");
		exit(1);
		}

	//attacco memoria condivisa
	if((shmA= shmat(idA,NULL,0666))== (int*) -1) {
		syserr("errore attaccamento area memoria condivisa A");
		exit(1);
		}
	
	if((idB=shmget(SHMKEY_B,sizeof(int [ (1<<matrixOrder) ]) ,0666|IPC_CREAT|IPC_EXCL)) == -1) {
		syserr("creazione shmem B fallita!");
		exit(1);
		}

	if((shmB= shmat(idB,NULL,0666))== (int*) -1) {
		syserr("errore attaccamento area memoria condivisa B");
		exit(1);
		}

	if((idC=shmget(SHMKEY_C,sizeof(int [ (1<<matrixOrder) ]) ,0666|IPC_CREAT|IPC_EXCL)) == -1) {
		syserr("creazione shmem C fallita!");
		exit(1);
		}

	if((shmC= shmat(idC,NULL,0666))== (int*) -1) {
		syserr("errore attaccamento area memoria condivisa C");
		exit(1);
		}

	if((idSum=shmget(SHMKEY_SUM,sizeof(int) ,0666|IPC_CREAT|IPC_EXCL)) == -1) {
		syserr("creazione shmem SUM fallita!");
		exit(1);
		}

	if((shmSum= shmat(idSum,NULL,0666))== (int*) -1) {
		syserr("errore attaccamento area memoria condivisa C");
		exit(1);
		}

	/*gestione controllo sui semafori*/
	union semun {
		int val;
		struct semid_ds *buf;
		ushort *array;
	} st_sem;

	/*inizializzazione struttura operazioni*/
	wait_b.sem_num=signal_b.sem_num=0;
	/*con la wait decremento sem */
	wait_b.sem_op=-1; 
	/*con la signal incremento sem*/
	signal_b.sem_op=1;
	
	wait_b.sem_flg=signal_b.sem_flg=SEM_UNDO;

	st_sem.val=1;
	/*creo semaforo*/
	if((idSem=semget(SEMKEY,5,0666|IPC_CREAT|IPC_EXCL)) == -1) {
		syserr("padre:crezione semafori fallita");
		exit(1);
		}
	
	sem_stato=semctl(idSem,SEM_A,SETVAL,st_sem);
	if (sem_stato==-1){
		syserr("padre:semaforo EMPTY non inizializzato");
		exit(1);
		}
	st_sem.val=1;
	sem_stato=semctl(idSem,SEM_B,SETVAL,st_sem);
	if(sem_stato==-1){
		syserr("padre: inizializzazione MUTEX fallita");
		exit(1);
		}
	st_sem.val=1;
	sem_stato=semctl(idSem,SEM_C,SETVAL,st_sem);
	if(sem_stato==-1){
		syserr("padre:inizializzazione semaforo FULL fallita");
		exit(1);
		}
	st_sem.val=1;
	sem_stato=semctl(idSem,SEM_SUM,SETVAL,st_sem);
	if(sem_stato==-1){
		syserr("padre:inizializzazione semaforo FULL fallita");
		exit(1);
		}
	/*questo semaforo mi serve per essere certo che i figli abbiano terminato le moltiplicazioni
	prima di passare a calcolare la somma delle righe*/
	st_sem.val=procsNumber;
	sem_stato=semctl(idSem,SEM_MUL,SETVAL,st_sem);
	if(sem_stato==-1){
		syserr("padre:inizializzazione semaforo MUL fallita");
		exit(1);
		}
	easywrite("[PADRE]risorse inizializzate\n");
}

/**
* @brief Funzione che crea i processi figli e gli fa eseguire la funzione 'sonSimulation' contenuta in @ref son.c
*/
void createSons(){
	int i;
	char temp[100];

	sprintf(temp, "[PADRE]creando %d figli\n",procsNumber);
	easywrite(temp);
	for (i = 0; i < procsNumber; i++){
		int sonpid;
		if((sonpid = fork()) == 0){
			sonSimulation(i,matrixOrder,p);
			}
		else if (sonpid == -1) {
			syserr("Generazione figli: errore durante la creazione di uno dei figli");
			exit(1);
			}
		sprintf(temp, "[PADRE]creato figlio con pid=%d\n",sonpid);
		easywrite(temp);
	}
}
/**
* @brief Funzione che si occupa di inviare ai figli le operazioni che essi devono eseguire
*/
void sendOperations(){
	int i,j,n,k,c=0;
	message *m; /*puntatore a messaggio*/
	int freeSon;

	m=(message *) malloc (sizeof(message));/* alloco memoria per messaggio*/

	easywrite("[PADRE]inviando operazioni\n");
	easywrite("[PADRE->OPERAZIONI]inviando moltiplicazioni\n");
	/*moltiplicazione*/
	for(i=0;i<matrixOrder;i++){
		for(j=0;j<matrixOrder;j++){
			sem_wait(idSem,SEM_MUL);
			if(msgrcv(idMsg, m, sizeof(message), MSGTYPE, 0) == -1) {
				syserr("padre: ricezione mesg fallita");
				exit(1);
				}
			freeSon=m->sender;
			char command [BUFFER_SIZE];
			sprintf(command,"M %d %d",i,j);
			if (write (p[1 + freeSon*2], command, BUFFER_SIZE) == -1) {
				syserr("padre: write su pipe fallito");
				exit(1);
				}
		}
	}	
	for(n=0;n<procsNumber;n++)/*controllo che tutti i figli abbiano terminato le moltiplicazioni*/
		sem_wait(idSem,SEM_MUL);
	
	easywrite("[PADRE->OPERAZIONI]moltiplicazioni effuate inviando somme\n");
/*somma*/
	for(k=0;k<matrixOrder;k++){
		if(msgrcv(idMsg, m, sizeof(message), MSGTYPE, 0) == -1) {
			syserr("server: ricezione mesg fallita");
			exit(1);
			}
		freeSon=m->sender;
		char command [BUFFER_SIZE];
		sprintf(command,"S %d",k);
		if (write (p[1 + freeSon*2], command, BUFFER_SIZE) == -1) {
			syserr("padre: write su pipe fallito");
			exit(1);
			}
	}

	easywrite("[PADRE->OPERAZIONI]somme inviate inviando comando di chiusura\n");
/*chiusura figli*/
	for(n=0;n<procsNumber;n++){
		if(msgrcv(idMsg, m, sizeof(message), MSGTYPE, 0) == -1) {
			syserr("server: ricezione mesg fallita");
			exit(1);
			}
		freeSon=m->sender;
		char command [BUFFER_SIZE];
		sprintf(command,"C");
		if (write (p[1 + freeSon*2], command, BUFFER_SIZE) == -1) {
			syserr("padre: write su pipe fallito");
			exit(1);
			}
	}
	easywrite("[PADRE]inviate tutte le operazioni\n");
}
/**
* @brief Funzione che si occupa di scrivere all'interno del file la matrice C risultante dalle operazioni eseguite
*/
void printMatrix(){
	int i,j,c;
	easywrite("[PADRE]scrivendo matrice C nel file di testo\n");
	for (i=0;i<matrixOrder;i++){
		for(j=0;j<matrixOrder;j++){
			sem_wait(idSem,SEM_C);
			c=*(shmC + calc_index(i,j,matrixOrder));
			sem_signal(idSem,SEM_C);
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
/*eliminazione coda messaggi*/
	easywrite("[PADRE]liberando risorse\n");
	if ((msgctl(idMsg, IPC_RMID, NULL)) == -1){
		syserr("errore chiusura coda di messaggi");
		exit(1);
		}
/*eliminazione semafori*/
	if ((semctl(idSem,0,IPC_RMID,0)) == -1){
		syserr("errore chiusura array di semafori");
		exit(1);
		}
/*chiusura ed eliminazione memoria condivisa A*/
	if ((shmdt(shmA))==-1){
		syserr("errore shmdt A");
		exit(1);
		}
	if((shmctl(idA,IPC_RMID,NULL)) == -1){
		syserr("errore eliminazione memoria condivisa A");
		exit(1);
		}
/*chiusura ed eliminazione memoria condivisa B*/
	if ((shmdt(shmB))==-1){
		syserr("errore shmdt B");
		exit(1);
		}
	if((shmctl(idB,IPC_RMID,NULL)) == -1){
		syserr("errore eliminazione memoria condivisa B");
		exit(1);
		}
/*chiusura ed eliminazione memoria condivisa C*/
	if ((shmdt(shmC))==-1){
		syserr("errore shmdt C");
		exit(1);
		}
	if((shmctl(idC,IPC_RMID,NULL)) == -1){
		syserr("errore eliminazione memoria condivisa C");
		exit(1);
		}
/*chiusura ed eliminazione memoria condivisa SUM*/
	if ((shmdt(shmSum))==-1){
		syserr("errore shmdt SUM");
		exit(1);
		}
	if((shmctl(idSum,IPC_RMID,NULL)) == -1){
		syserr("errore eliminazione memoria condivisa SUM");
		exit(1);
		}
	easywrite("[PADRE]risorse liberate\n");				
}


