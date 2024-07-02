// Name - K. Hrushikesh Reddy and Shivang Agrawal
// Roll no. - 21CS30028 and 21CS30048

//number in queue vs p_id vs page_number accessed
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <limits.h>
#include <math.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>


typedef struct {
    int frameno;
    int isvalid;
    int count;
}pagetable_entry;

typedef struct {
    pid_t pid;
    int m;
    int frame_count;
    int f_allo;
}PCB;

typedef struct 
{
    int current;
    int flist[];
}free_list;

int k,m,f;
int flag = 0;
key_t readykey;
key_t msgq2key, msgq3key;
key_t share_key;
key_t free_key,pagetbkey;

int ptbid, freelid;
int readyid, msgq2id, msgq3id;
int PCBid;

int max(int p, int q)
{
    return (p>q)?p:q;
}

int min(int a,int b)
{
    return (a<b)?a:b;   
}
void print_PCB(PCB p)
{
    printf("PID = %d num_pages = %d num_frames = %d\n",p.pid,p.m,p.frame_count);

}

void myexit(int status);

void createfree_list()
{
    int i;
    free_key = ftok("master.c",56);
    if(free_key == -1)
    {   
        perror("free_key");
        myexit(EXIT_FAILURE);
    }
    freelid = shmget(free_key, sizeof(free_list)+f*sizeof(int), 0666 | IPC_CREAT | IPC_EXCL);
    if(freelid == -1)
    {   
        perror("free-shmget");
        myexit(EXIT_FAILURE);
    }

    free_list *it = (free_list*)(shmat(freelid, NULL, 0));
    if(*((int *)it) == -1)
    {
        perror("freel-shmat");
        myexit(EXIT_FAILURE);
    }
    for(i=0;i<f;i++)
    {
        it->flist[i] = i;
    }
    it->current = f-1;

    if(shmdt(it) == -1)
    {
        perror("freel-shmdt");
        myexit(EXIT_FAILURE);
    }
}

void createPageTables()
{
    int i;
    pagetbkey = ftok("master.c",100);
    if(pagetbkey == -1)
    {   
        perror("pagetbkey");
        myexit(EXIT_FAILURE);
    }
    ptbid = shmget(pagetbkey, m*sizeof(pagetable_entry)*k, 0666 | IPC_CREAT);
    if(ptbid == -1)
    {   
        perror("PCB-shmget");
        myexit(EXIT_FAILURE);
    }

    pagetable_entry *it = (pagetable_entry*)(shmat(ptbid, NULL, 0));
    if(*(int *)it == -1)
    {
        perror("PCB-shmat");
        myexit(EXIT_FAILURE);
    }

    for(i=0;i<k*m;i++)
    {
        it[i].frameno = -1;
        it[i].isvalid = 0;
    }

    if(shmdt(it) == -1)
    {
        perror("PCB-shmdt");
        myexit(EXIT_FAILURE);
    }
}

void createPCBs()
{
    int i;
    share_key = ftok("master.c",500);
    if(share_key == -1)
    {   
        perror("share_key");
        myexit(EXIT_FAILURE);
    }
    PCBid = shmget(share_key, sizeof(PCB)*k, 0666 | IPC_CREAT | IPC_EXCL );
    if(PCBid == -1)
    {   
        perror("PCB-shmget");
        myexit(EXIT_FAILURE);
    }

    PCB *it = (PCB*)(shmat(PCBid, NULL, 0));
    if(*(int *)it == -1)
    {
        perror("PCB-shmat");
        myexit(EXIT_FAILURE);
    }

    int totpages = 0;
    for(i=0;i<k;i++)
    {
        it[i].pid = i;
        it[i].m = rand()%m + 1;
        it[i].f_allo = 0;
        totpages +=  it[i].m;
    }
    int alc_frame = 0;
    printf("tot = %d, k = %d, f=  %d\n",totpages,k,f);
    int max = 0,maxi = 0;
    for(i=0;i<k;i++)
    {
        it[i].pid = -1;
        int allo = (int)round(it[i].m*(f-k)/(float)totpages) + 1;
        if(it[i].m > max)
        {
            max = it[i].m;
            maxi = i;
        }
        alc_frame = alc_frame + allo;
        //printf("%d\n",allo);
        it[i].frame_count = allo;
        
    }
    it[maxi].frame_count += f - alc_frame; 

    for(i=0;i<k;i++)
    {
        print_PCB(it[i]);
    }

    if(shmdt(it) == -1)
    {
        perror("freel-shmdt");
        myexit(EXIT_FAILURE);
    }

}

void create_MSGQ()
{
    readykey = ftok("master.c",200);
    if(readykey == -1)
    {   
        perror("readykey");
        myexit(EXIT_FAILURE);
    }
    readyid = msgget(readykey, 0666 | IPC_CREAT| IPC_EXCL);
    if(readyid == -1)
    {
        perror("ready-msgget");
        myexit(EXIT_FAILURE);
    }

    msgq2key = ftok("master.c",300);
    if(msgq2key == -1)
    {   
        perror("msgq2key");
        myexit(EXIT_FAILURE);
    }
    msgq2id = msgget(msgq2key, 0666 | IPC_CREAT| IPC_EXCL );
    if(msgq2id == -1)
    {
        perror("msgq2-msgget");
        myexit(EXIT_FAILURE);
    } 

    msgq3key = ftok("master.c",400);
    if(msgq3key == -1)
    {   
        perror("msgq3key");
        myexit(EXIT_FAILURE);
    }
    msgq3id = msgget(msgq3key, 0666 | IPC_CREAT| IPC_EXCL);
    if(msgq3id == -1)
    {
        perror("msgq3-msgget");
        myexit(EXIT_FAILURE);
    } 
}



void init_processes()
{
    PCB *it = (PCB*)(shmat(PCBid, NULL, 0));
    /*if(*(int *)it == -1)
    {
        perror("PCB-shmat");
        myexit(EXIT_FAILURE);
    }*/

    int i,j;
    for(i=0;i<k;i++)
    {
        int rlength = rand()%(8*it[i].m) + 2*it[i].m + 1;
        char rstring[m*20*40];
        printf("rlength = %d\n",rlength);
        int l = 0;
        for(j=0;j<rlength;j++)
        {
            int r;
            r = rand()%it[i].m;
            float p = (rand()%100)/100.0;
            if(p < 0.2)
            {
                r = rand()%(1000*m) + it[i].m;
            }
            l += sprintf(rstring+l,"%d|",r);
        }
        printf("Access String = %s\n",rstring);
        if(fork() == 0)
        {
            char buf1[20],buf2[20],buf3[20];
            sprintf(buf1,"%d",i);
            sprintf(buf2,"%d",readykey);
            sprintf(buf3,"%d",msgq3key);
            execlp("./process","./process",buf1,buf2,buf3,rstring,(char *)(NULL));
            exit(0);

        }
        usleep(250*1000);   //wait to create next process
    }

}

void clr_resources()
{
    if(msgctl(readyid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl-ready");
    }
    if(msgctl(msgq2id, IPC_RMID, NULL) == -1)
    {
        perror("msgctl-msgq2");
    }
    if(shmctl(ptbid,IPC_RMID, NULL) == -1)
    {
        perror("shmctl-ptb");
    }
    if(shmctl(freelid,IPC_RMID, NULL) == -1)
    {
        perror("shmctl-freel");
    }
    if(shmctl(PCBid,IPC_RMID, NULL) == -1)
    {
        perror("shmctl-PCB");
    }
    if(msgctl(msgq3id, IPC_RMID, NULL) == -1)
    {
        perror("msgctl-msgq3");
    }
}

void myexit(int status)
{
    clr_resources();
    exit(status);
}



int pid,spid,mpid;

void timetoend(int sig)
{
    //printf("Mater: gi=o the signal\n");
    sleep(1);
    kill(spid, SIGTERM);
    kill(mpid, SIGUSR2);
    sleep(2);
    flag = 1;

}
int main(int argc, char const *argv[])
{
    srand(time(NULL));
    signal(SIGUSR1, timetoend);
    signal(SIGINT, myexit);
    if(argc < 4)
    {
        printf("Provide:master k m f\n");
        myexit(EXIT_FAILURE);
    }
    k = atoi(argv[1]);
    m = atoi(argv[2]);
    f = atoi(argv[3]);
    pid = getpid();
    if(k <= 0 || m <= 0 || f <=0 || f < k)
    {
        printf("Invalid input\n");
        myexit(EXIT_FAILURE);
    }

    createPageTables();
    createfree_list();
    createPCBs();
    create_MSGQ();

    if((spid = fork()) == 0)
    {
        char arg1[20],arg2[20],arg3[20],arg4[20];
        sprintf(arg1,"%d",readykey);
        sprintf(arg2,"%d",msgq2key);
        sprintf(arg3,"%d",k);
        sprintf(arg4,"%d",pid);
        execlp("./scheduler","./scheduler",arg1,arg2,arg3,arg4,(char *)(NULL));
        exit(0);
    }


    if((mpid = fork()) == 0)
    {
        char buf1[20],buf2[20],buf3[20],buf4[20],buf5[20],buf6[20],buf7[20];
        sprintf(buf1,"%d",msgq2id);
        sprintf(buf2,"%d",msgq3id);
        sprintf(buf3,"%d",ptbid);
        sprintf(buf4,"%d",freelid);
        sprintf(buf5,"%d",PCBid);
        sprintf(buf6,"%d",m);
        sprintf(buf7,"%d",k);
                execlp("./mmu","./mmu",buf1,buf2,buf3,buf4,buf5,buf6,buf7,(char *)(NULL));

        exit(0);
    }
    printf("generating processed\n");
    init_processes();
    if(flag == 0)
        pause();
    clr_resources();

    return 0;
}