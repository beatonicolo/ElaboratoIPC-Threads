#define BUFFER_SIZE 512
#define SEM_A 0
#define SEM_B 1
#define SEM_C 2
#define SEM_SUM 3
#define SEM_MUL 4
#define MSGKEY 90
#define SEMKEY 91
#define SHMKEY_A 92
#define SHMKEY_B 93
#define SHMKEY_C 94
#define SHMKEY_SUM 95
#define MSGTYPE 1


void itoa(int n, char *s);
void easywrite(char *buffer);
void syserr (char *msg);
void sem_wait (int semid, int sem_number);
void sem_signal (int semid,int sem_number);
int calc_index(int i, int j,int dim);
void syswriteint(int *msg);
/**
* @brief struttura che rappresenta il messaggio

*/
typedef struct msg4queue {
/**
* @brief indica tipo del messaggio

*/
	long mtype;
/**
* @brief indica il figlio che ha inviato il messaggio

*/
	int sender;
} message;


