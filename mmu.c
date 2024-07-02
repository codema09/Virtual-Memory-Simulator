// Name - K. Hrushikesh Reddy and Shivang Agrawal
// Roll no. - 21CS30028 and 21CS30048

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/msg.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <math.h>


#define MMU_TO_PROCESS 20
#define PAGE_FAULT -1
#define INVALID_PAGE_REFERENCE -2
#define PROCESS_SEND_TYPE 10
#define TERMINATED 10
#define HANDLED_PAGEFAULT 5
#define PROCESS_OVER -9

int sigusr2_flg = 1;
void handle_sigusr2(int sig)
{	
	sigusr2_flg = 0;
}

typedef struct {
	pid_t pid;
	int m;
	int frame_cnt;
	int frame_allo;
} PCB;

typedef struct {
	int frame_num;
	int valid_bit;
	int cnt;
} PTB_entry;


typedef struct
{
	int curr;
	int frame_free_list[];
} free_list;

struct msgbuf
{
	long mess_type;
	int id;
	int page_num;
};

struct mmu_to_process_buf
{
	long mess_type;
	int frame_num;
};

struct mmu_to_sched
{
	long mess_type;
	char mess_buf[1];
};

// Global declarations
int *page_fault_freq;
FILE *fptr;
int i;
int cnt = 0;

PCB *PCB_ptr;
PTB_entry *PTB_ptr;
free_list *free_ptr;

key_t freekey, pagetbkey;
key_t msgq2key, msgq3key;
key_t pcbkey;

int PTB_id, free_list_id;
int mess_2_q_key, mess_3_q_key;
int PCB_id;


int m,k;

int handle_receive_request(int* arg)
{
	struct msgbuf loc_mess_buf;
	int mess_len = sizeof(struct msgbuf) - sizeof(long);
	memset(&loc_mess_buf, 0, sizeof(loc_mess_buf));

	int rst = msgrcv(mess_3_q_key, &loc_mess_buf, mess_len, PROCESS_SEND_TYPE, 0);
	if (rst == -1)
	{
		if(errno == EINTR)
			return -1;
		perror("Error in msgrcv(mmu)");
		exit(1);
	}
	*arg = loc_mess_buf.id;
	return loc_mess_buf.page_num;
}

void handle_send(int arg, int frame_num)
{
	// printf("Frame number %d\n",frame_num);
	struct mmu_to_process_buf loc_mess_buf;
	loc_mess_buf.frame_num = frame_num;
	loc_mess_buf.mess_type = arg + MMU_TO_PROCESS;
	int mess_len = sizeof(struct msgbuf) - sizeof(long);
	int n = msgsnd(mess_3_q_key, &loc_mess_buf, mess_len, 0);
	if (n == -1)
	{
		perror("Error in msgsnd(mmu)");
		exit(1);
	}
}

void notify_scheduler(int mess_type)
{
	struct mmu_to_sched loc_mess_buf;
	loc_mess_buf.mess_type = mess_type;
	int mess_len = sizeof(struct msgbuf) - sizeof(long);
	int n = msgsnd(mess_2_q_key, &loc_mess_buf, mess_len, 0);
	if (n == -1)
	{
		perror("Error in msgsnd(mmu: notify_sched)");
		exit(1);
	}
}

int handle_page_fault(int id, int page_num)
{
	// int i;
	if (free_ptr->curr == -1 || PCB_ptr[i].frame_cnt <= PCB_ptr[i].frame_allo)
	{
		int min = INT_MAX, minj = -1;
		int vic_page = 0;
		for (int i = 0; i < PCB_ptr[i].m; i++)
		{
			int loc=id * m + i;
			if (PTB_ptr[loc].valid_bit == 1)
			{
				if (PTB_ptr[loc].cnt < min)
				{
					min = PTB_ptr[loc].cnt;
					vic_page = PTB_ptr[loc].frame_num;
					minj = i;
				}
			}
		}
		PTB_ptr[id * m + minj].valid_bit = 0;
		return vic_page;
	}
	else
	{
		int free_pg = free_ptr->frame_free_list[free_ptr->curr];
		free_ptr->curr -= 1;
		return free_pg;
	}
}

void set_free_pages(int i)
{
	for (int j = 0; j < PCB_ptr[i].m; i++)
	{
		int loc=i * m + j;
		if (PTB_ptr[loc].valid_bit == 1)
		{
			free_ptr->frame_free_list[free_ptr->curr + 1] = PTB_ptr[loc].frame_num;
			free_ptr->curr+=1;
		}
	}	
}

int sevice_memory_req()
{
	free_ptr = (free_list*)(shmat(free_list_id, NULL, 0));
	PTB_ptr = (PTB_entry*)(shmat(PTB_id, NULL, 0));
	PCB_ptr = (PCB*)(shmat(PCB_id, NULL, 0));

	int id = -1, page_num;
	page_num = handle_receive_request(&id);
	if(page_num == -1 && id == -1)
	{
		return 0;
	}
	i = id;
	if (page_num == PROCESS_OVER)
	{
		set_free_pages(id);
		notify_scheduler(TERMINATED);
		return 0;
	}
	
	cnt ++;
	printf("Page reference : (%d,%d,%d)\n",cnt,id,page_num);
	fprintf(fptr,"Page reference : (%d,%d,%d)\n",cnt,id,page_num);
	if (PCB_ptr[id].m < page_num || page_num < 0)
	{
		printf("Invalid Page Reference : (%d %d)\n",id,page_num);
		fprintf(fptr,"Invalid Page Reference : (%d %d)\n",id,page_num);
		handle_send(id, INVALID_PAGE_REFERENCE);
		printf("Process %d: Trying to access invalid page ref %d\n", id, page_num);
		set_free_pages(id);
		notify_scheduler(TERMINATED);
	}
	else
	{
		if (PTB_ptr[i * m + page_num].valid_bit == 0)
		{
			
			fprintf(fptr,"Page Fault : (%d, %d)\n",id,page_num);
			printf("Page Fault : (%d, %d)\n",id,page_num);
			page_fault_freq[id] += 1;
			handle_send(id, -1);
			int frame_no = handle_page_fault(id, page_num);
			int loc=i * m + page_num;
			PTB_ptr[loc].cnt = cnt;
			PTB_ptr[loc].valid_bit = 1;
			PTB_ptr[loc].frame_num = frame_no;
			
			notify_scheduler(HANDLED_PAGEFAULT);
		}
		else
		{
			handle_send(id, PTB_ptr[i * m + page_num].frame_num);
			PTB_ptr[i * m + page_num].cnt = cnt;			
		}
	}
	shmdt(PCB_ptr);
	shmdt(PTB_ptr);
	shmdt(free_ptr);
}

int main(int argc, char const *argv[])
{
	if (argc < 8)
	{
		printf("Error: Provide following arguments\n");
		printf("1)mess_2key\n");
		printf("2)mess_3key\n");
		printf("3)ptb_key\n");
		printf("4)f_key\n");
		printf("5)pcb_key\n");
		printf("6)m\n");
		printf("7)k\n");
		exit(1);
	}

	signal(SIGUSR2, handle_sigusr2);
	fptr = fopen("result.txt","w");
	
	mess_2_q_key = atoi(argv[1]);
	mess_3_q_key = atoi(argv[2]);
	PTB_id = atoi(argv[3]);
	free_list_id = atoi(argv[4]);
	PCB_id = atoi(argv[5]);
	m = atoi(argv[6]);
	k = atoi(argv[7]);

	page_fault_freq = (int *)malloc(k*sizeof(int));
	for(i=0;i<k;i++)
	{
		page_fault_freq[i] = 0;
	} 

	while(1)
	{
		if(sigusr2_flg==0)break;
		sevice_memory_req();
	}
	printf("\nPage fault Count for each Process:\n");	
	fprintf(fptr,"\nPage fault Count for each Process:\n");
	printf("\n%10s\t| Freq\n","Process ID");
	printf("_________________________________\n");
	fprintf(fptr,"Process Id\tFreq\n");
	for(i=0;i<k;i++)
	{
		printf("%10d\t|%5d\n",i,page_fault_freq[i]);
		fprintf(fptr,"%10d\t|%5d\n",i,page_fault_freq[i]);
	}
	fclose(fptr);
	// sleep(100);
	return 0;
}
