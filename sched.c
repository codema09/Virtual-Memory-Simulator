// Name - K. Hrushikesh Reddy and Shivang Agrawal
// Roll no. - 21CS30028 and 21CS30048
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/msg.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <ctype.h>
#include <sys/shm.h>


#define MAX_BUFFER_SIZE 100
#define MAX_PAGES 1000
#define MAX_PROCESS 1000
#define FROMPROCESS 10
#define TOPROCESS 20  
#define FROMMMU 20

#define PAGEFAULT_HANDLED 5
#define TERMINATED 10

int k; //no. of processes
typedef struct _mmutosch {
	long    mtype;
	char mbuf[1];
} MM_SCH;

// int running[MAX_PROCESS];
// int runh,runt;

typedef struct mymsgbuf {
	long    mtype;
	int id;
} mymsgbuf;



int read_message( int qid, long type, struct mymsgbuf *qbuf )
{
	int  status, len;

	len = sizeof(struct mymsgbuf) - sizeof(long);

	if ((status = msgrcv( qid, qbuf, len, type,  0)) == -1)
	{
		perror("Error in getting message");
		exit(1);
	}

	return (status);
}

int send_message( int qid, struct mymsgbuf *qbuf )
{
	int     status, len;

	/* The len is essentially the size of the structure minus sizeof(mtype) */
	len = sizeof(struct mymsgbuf) - sizeof(long);

	if ((status = msgsnd( qid, qbuf, len, 0)) == -1)
	{
		perror("Error in sending message");
		exit(1);
	}

	return (status);
}

int read_message_mmu( int qid, long type,MM_SCH *qbuf )
{
	int status, len;

	/* The len is essentially the size of the structure minus sizeof(mtype) */
	len = sizeof(MM_SCH) - sizeof(long);

	if ((status = msgrcv(qid, qbuf, len, type,  0)) == -1)
	{
		perror("Error in receiving message");
		exit(1);
	}

	return (status);
}
int main(int argc , char * argv[])
{
	int key1_MQ, key2_MQ, master_pid;
	if (argc < 5) {
		printf("Scheduler rkey q2key k mpid\n");
		exit(EXIT_FAILURE);
	}
	key1_MQ = atoi(argv[1]);
	key2_MQ = atoi(argv[2]);
	k = atoi(argv[3]);
	master_pid = atoi(argv[4]);

	mymsgbuf sent_message, msg_recv;

	int mq1 = msgget(key1_MQ, 0666);
	int mq2 = msgget(key2_MQ, 0666);
	if (mq1 == -1)
	{
		perror("Failed in Message Queue 1 creation");
		exit(1);
	}
	if (mq2 == -1)
	{
		perror("Failed in Message Queue2 creation ");
		exit(1);
	}
	printf("Total No. of Process received = %d\n", k);

	//initialize the variables for running array
	int term_processes = 0; //to keep track of running process to exit at last
	// int terminate[MAX_PROCESS];
	while (1)
	{
		read_message(mq1, FROMPROCESS, &msg_recv);
		int curr_id = msg_recv.id;

		sent_message.mtype = TOPROCESS + curr_id;
		send_message(mq1, &sent_message);

		//recv messages from mmu
		MM_SCH mmu_recv;
		read_message_mmu(mq2, 0, &mmu_recv);
		//printf("received %ld\n", mmu_recv.mtype);
		if (mmu_recv.mtype == PAGEFAULT_HANDLED)
		{
			sent_message.mtype = FROMPROCESS;
			sent_message.id=curr_id;
			send_message(mq1, &sent_message);
		}
		else if (mmu_recv.mtype == TERMINATED)
		{
			term_processes++;
			//printf("Got terminate %d\n", curr_id);
		}
		else
		{
			perror("Wrong message from mmu\n");
			exit(1);
		}
		if (term_processes == k)
			break;
		//printf("====Term %d====\n", term_processes);
	}
	//printf("Sending sinal\n");
	kill(master_pid, SIGUSR1);
	pause();
	printf("Scheduler terminating ...\n") ;
	exit(1) ;
}