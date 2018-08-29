#include <string.h>
#include <unistd.h>
#include <sys/sem.h>
#include <stdlib.h>

/** 
* @file 
* Contiene le funzioni comuni a più file, come quelle di lettura e scrittura e le operazioni su semafori
*/
/**
* @brief costante che indica lo standard output

*/
#define STDOUT 1
/**
* @brief costante che indica lo standard error

*/
#define STDERR 2
/**
* @brief strutture per le operazioni sui semafori

*/
struct sembuf wait_b,signal_b;
/**
* @brief stampa una stringa
* @param buffer puntatore alla stringa da stampare
*/
void easywrite (char *buffer){
	write(STDOUT,buffer,strlen(buffer));	
}

/**
* @brief Inverte una stringa. Serve per il funzionamento di itoa. Non è incluso nell'header, quindi non è raggiungibile dall'esterno
* 
* @param s puntatore alla stringa da invertire
*/
void reverse(char *s) {
     int i, j;
     char c;
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }

/**
* @brief Converte un numero in una stringa
* 
* @param n il numero da invertire
* @param s puntatore alla stringa di destinazione
*/
void itoa(int n, char *s){
	//segno
    int sign;
    if ((sign = n) < 0)
        n = -n;

    //inversione
    char *t = s;
    do {
        *t= n % 10 + '0';
        t++;
    } while ((n /= 10) > 0); 

    //aggiunta del segno
    if (sign < 0){
        *t = '-';
        t++;
    }
    *t = '\0';
    reverse(s);
 }
/**
* @brief surrogato della funzine perror
*/
void syserr (char *msg){
	char temp[50] = "Errore: ";
	write(STDERR, strcat(temp, msg), strlen(msg) + 8);
	write(STDERR, "\n", 1);
}

/**
* @brief procedura per l'esecuzione di una wait sul semaforo
* @param semid id array di semafori su cui eseguire operazione
* @param sem_number semaforo su cui eseguire operazione
*/
void sem_wait (int semid, int sem_number) {
	wait_b.sem_num=sem_number;						/* setto il numero del semaforo */
	if (semop(semid,&wait_b,1)==-1) {	/* eseguo la wait */
		syserr("WAIT abortita. TERMINO.");
		exit(1);
	}
}

/**
* @brief procedura per l'esecuzione di una signal sul semaforo
* @param semid id array di semafori su cui eseguire operazione
* @param sem_number semaforo su cui eseguire operazione
*/
void sem_signal (int semid,int sem_number) {
	signal_b.sem_num=sem_number;						/* setto il numero del semaforo */
	if (semop(semid,&signal_b,1)==-1) {	/* eseguo la signal */
		syserr("SIGNAL abortita. TERMINO.");
		exit(1);
	}
}
/**
* @brief funzione per mappare gli indici di array 2D su array 1D
* @param i riga
* @param j colonna
* @param dim ordine della matrice
*/
int calc_index(int i, int j,int dim){
	int res;
	res=(i*dim) + j;
	return res;
}
/**
* @brief stampa un intero
* @param msg puntatore all'intero da stampare
*/
void syswriteint(int *msg){
	char temp[10];
	itoa(*msg, temp);
	write(STDOUT, temp, strlen(temp));
}
