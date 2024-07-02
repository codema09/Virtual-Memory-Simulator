// Name - K. Hrushikesh Reddy and Shivang Agrawal
// Roll no. - 21CS30028 and 21CS30048

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/msg.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/shm.h>
#include <ctype.h>

#define NUM_MAX_PAGES 1000

#define TO_MMU 10
#define FROM_MMU 20 

#define TO_SCHED 10
#define FROM_SCHED 20  

int page_num[NUM_MAX_PAGES] ;
int num_pages;

typedef struct msgbuf {
	long mess_type;
	int id;
} msgbuf;

typedef struct mmu_msg_buf_send {
	long mess_type;        
	int id;
	int page_num;
} mmu_msg_buf_send;

typedef struct mmu_msg_buf_receive {
	long mess_type;      
	int frame_num;
} mmu_msg_buf_receive;

int message_send( int mess_qid, struct msgbuf *mess_qbuf )
{
	int res, mess_len;

	
	mess_len = sizeof(struct msgbuf) - sizeof(long);

	if ((res = msgsnd( mess_qid, mess_qbuf, mess_len, 0)) == -1)
	{
		perror("Error in sending message");
		exit(1);
	}

	return (res);
}

int message_receive( int mess_qid, long type, struct msgbuf *mess_qbuf )
{
	int     res, mess_len;

	
	mess_len = sizeof(struct msgbuf) - sizeof(long);

	if ((res = msgrcv( mess_qid, mess_qbuf, mess_len, type,  0)) == -1)
	{
		perror("Error in receiving message");
		exit(1);
	}

	return (res);
}

int mmu_message_send( int mess_qid, struct mmu_msg_buf_send *mess_qbuf )
{
	int mess_len;

	
	mess_len = sizeof(struct mmu_msg_buf_send) - sizeof(long);

	int res;
	if ((res = msgsnd( mess_qid, mess_qbuf, mess_len, 0)) == -1)
	{
		perror("Error in sending message");
		exit(1);
	}

	return res;
}

int mmu_message_receive( int mess_qid, long type, struct mmu_msg_buf_receive *mess_qbuf )
{
	int mess_len;

	
	mess_len = sizeof(struct mmu_msg_buf_receive) - sizeof(long);

	int res;
	if ((res = msgrcv( mess_qid, mess_qbuf, mess_len, type,  0)) == -1)
	{
		perror("Error in receiving message");
		exit(1);
	}

	return res;
}

void get_page_nums(char arg[])
{
	const char s[2] = "|";
	char *token;
	token = strtok(arg, s);
	while ( token != NULL )
	{
		page_num[num_pages] = atoi(token);
		token = strtok(NULL, s);
		num_pages++;
	}
}

int main(int argc, char *argv[]) 
{
	if (argc < 5)
	{
		printf("Error in process:\n");
		printf("\t4 Command line arguments: \n");
		printf("\t\t1)id\n");
		printf("\t\t2)mess_queue1\n");
		printf("\t\t3)mess_queue3\n");
		printf("\t\t4)reference string\n");
		exit(1);
	}
	int id;
	id = atoi(argv[1]);
	printf("\nProcess with id %d started\n", id);

	int mess_queue_1_key, mess_queue_3_key;
	mess_queue_1_key = atoi(argv[2]);
	mess_queue_3_key = atoi(argv[3]);

	num_pages = 0;

	int message_queue_1, message_queue_3;

	message_queue_1 = msgget(mess_queue_1_key, 0666);
	if (message_queue_1 == -1)
	{
		perror("Message Queue 1 creation failed");
		exit(1);
	}

	message_queue_3 = msgget(mess_queue_3_key, 0666);
	if (message_queue_3 == -1)
	{
		perror("Message Queue 3 creation failed");
		exit(1);
	}

	get_page_nums(argv[4]);

	
	msgbuf send_mess;
	send_mess.mess_type = TO_SCHED;
	send_mess.id = id;
	message_send(message_queue_1, &send_mess);
	

	
	msgbuf my_receive_mess;
	message_receive(message_queue_1, FROM_SCHED + id, &my_receive_mess);
	

	mmu_msg_buf_receive receive_mmu;
	mmu_msg_buf_send snd_mmu;
	int curr_page = 0; 
	while (curr_page < num_pages)
	{
		
		printf("Process %d: Sent request for %d page number\n",id, page_num[curr_page]);
		snd_mmu.mess_type = TO_MMU;
		snd_mmu.id = id;
		snd_mmu.page_num = page_num[curr_page];
		mmu_message_send(message_queue_3, &snd_mmu);

		mmu_message_receive(message_queue_3, FROM_MMU + id, &receive_mmu);
		if (receive_mmu.frame_num == -2)
		{
			printf("Process %d: Invalid page reference, terminating\n", id) ;
			exit(1);
		}
		else if (receive_mmu.frame_num == -1) 
		{
			printf("Process %d: Page fault occured\n", id);
			
		}
		else if (receive_mmu.frame_num >= 0)
		{
			printf("Process %d: Frame number from MMU received: %d\n" , id, receive_mmu.frame_num);
			curr_page++;
		}
	}
	printf("Process %d Terminated successfly\n", id);
	snd_mmu.page_num = -9;
	snd_mmu.id = id;
	snd_mmu.mess_type = TO_MMU;
	mmu_message_send(message_queue_3, &snd_mmu);

	return 0;
}


