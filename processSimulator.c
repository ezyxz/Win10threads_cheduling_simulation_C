#include"linkedlist.h"
#include"coursework.h"
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
int process_runed;
int pid[128];
pthread_t tconsumer1 ,tconsumer2,tgenerate,tready,tterminated;
struct process * process_pointer;
struct element ** free_pid_head;
struct element ** free_pid_tail;
struct element ** ready_pid_head;
struct element ** ready_pid_tail;
struct element ** Terminated_pid_head;
struct element ** Terminated_pid_tail;
struct element ** p_pid_head[MAX_PRIORITY];
struct element ** p_pid_tail[MAX_PRIORITY];
struct process process_table[SIZE_OF_PROCESS_TABLE];
sem_t s_free_pid,s_new_pid,s_ready_pid,s_terminated_pid;
pthread_mutex_t mymutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t boostmutex=PTHREAD_MUTEX_INITIALIZER;
struct timeval * oStartTime;
struct timeval * oEndTime;
struct timeval oBaseTime1;
struct timeval oBaseTime2;
int Time_difference_1;
int Time_difference_2;
int Response_Time1[SIZE_OF_PROCESS_TABLE];
int Turnaround_Time1[SIZE_OF_PROCESS_TABLE];
int Response_Time2[SIZE_OF_PROCESS_TABLE];
int Turnaround_Time2[SIZE_OF_PROCESS_TABLE];
 pthread_t tvice_short_trem;
///////////////////////FUNCTION////////////////////////////////////////////
void printHeadersSVG()
{
	printf("SVG: <!DOCTYPE html>\n");
	printf("SVG: <html>\n");
	printf("SVG: <body>\n");
	printf("SVG: <svg width=\"10000\" height=\"1100\">\n");
}

//In this case, Consumer1 and Consumer2 have echo own basetime to calculate Response time and Turnaround time
//Because when Consumer1 is runing, the basetime in Consumer2 The passage of time is still calculating
//The solution is calculate the time difference when one of the cpu is running and  accumulate the previous time difference
void printProcessSVG(int iCPUId, struct process * pProcess,struct timeval oBaseTime ,struct timeval oStartTime, struct timeval oEndTime)
{
    int iXOffset;
    if (iCPUId==2)
    {
        iXOffset = getDifferenceInMilliSeconds(oBaseTime, oStartTime) + 31 - Time_difference_2;
    }else
    {
        iXOffset = getDifferenceInMilliSeconds(oBaseTime, oStartTime) + 30 - Time_difference_1;
    }
    int iYOffsetPriority = (pProcess->iPriority + 1) * 16 - 12;
	int iYOffsetCPU = (iCPUId - 1 ) * (480 + 50);
	int iWidth = getDifferenceInMilliSeconds(oStartTime, oEndTime);
	printf("SVG: <rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"8\" style=\"fill:rgb(%d,0,%d);stroke-width:1;stroke:rgb(255,255,255)\"/>\n", iXOffset /* x */, iYOffsetCPU + iYOffsetPriority /* y */, iWidth, *(pProcess->pPID) - 1 /* rgb */, *(pProcess->pPID) - 1 /* rgb */);
}

void printPrioritiesSVG()
{
	for(int iCPU = 1; iCPU <= NUMBER_OF_CPUS;iCPU++)
	{
		for(int iPriority = 0; iPriority < MAX_PRIORITY; iPriority++)
		{
			int iYOffsetPriority = (iPriority + 1) * 16 - 4;
			int iYOffsetCPU = (iCPU - 1) * (480 + 50);
			printf("SVG: <text x=\"0\" y=\"%d\" fill=\"black\">%d</text>\n", iYOffsetCPU + iYOffsetPriority, iPriority);
		}
	}
}
void printRasterSVG()
{
	for(int iCPU = 1; iCPU <= NUMBER_OF_CPUS;iCPU++)
	{
		for(int iPriority = 0; iPriority < MAX_PRIORITY; iPriority++)
		{
			int iYOffsetPriority = (iPriority + 1) * 16 - 8;
			int iYOffsetCPU = (iCPU - 1) * (480 + 50);
			printf("SVG: <line x1=\"%d\" y1=\"%d\" x2=\"10000\" y2=\"%d\" style=\"stroke:rgb(125,125,125);stroke-width:1\" />\n", 16, iYOffsetCPU + iYOffsetPriority, iYOffsetCPU + iYOffsetPriority);
		}
	}
}

void printFootersSVG()
{
	printf("SVG: Sorry, your browser does not support inline SVG.\n");
	printf("SVG: </svg>\n");
	printf("SVG: </body>\n");
	printf("SVG: </html>\n");
}

// A pop function to remove the first node in the linked list and return it
int* linkedlist_pop(struct element ** queue)
{
    int * temp_int;
    struct element* temp_ptr;
    if(!queue || !(*queue))
    return NULL;
    if ((*queue)&&(*queue)->pData)
    {
        temp_int=(int *) (*queue)->pData;
        temp_ptr=(*queue);
        (*queue)=(*queue)->pNext;
        free(temp_ptr);
        
        return temp_int;
    }else
    {
       
        return NULL;
    }
}
// A pop function to return the first node in the linked list
int* linkedlist_peek(struct element ** queue)
{
    int * temp_int;
    struct element* temp_ptr;
    if(!queue || !(*queue))
    return NULL;   

    if ((*queue)&&(*queue)->pData)
    {
        temp_int=(int *) (*queue)->pData;
        
        return temp_int;
    }else
    {
       
        return NULL;
    }
}

///////////////////////////THREAD/////////////////////////////////////
//////////////////////////////////////////////////////////////////////


//Process generator
//The generate thread 
//Wait if there exists free pid (s_free_pid)
//Find free process from free process queue
//Once find it, pop it from  free process queue (free_pid_head) and push to new process queue(new_pid_head new_pid_tail)
//Finally the amount of new pid +1 [sem_post(&s_new_pid)]
void *thread_tgenerate(void *arg) 
{   
    int * temp_int;
    struct process* temp_ptr;
    while(process_runed<=NUMBER_OF_PROCESSES)
    {
        sem_wait(&s_free_pid);

        temp_int=linkedlist_pop(free_pid_head);
        
        if (temp_int)
        {
            temp_ptr=generateProcess(temp_int);
            process_table[*temp_int]=*temp_ptr;
            free(temp_ptr);
            printf("TXT: Generated: Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime,process_table[*temp_int].iRemainingBurstTime);
            addLast((void*)temp_int,ready_pid_head,ready_pid_tail);

        }
        
        sem_post(&s_new_pid);
       
    }
    return NULL;
}


//Booster Daemon
//Schedule the processes in priority 16-31 ( p_pid_head[i] )to the queue of priority 16 at intervals
//
void* thread_tvice_short_trem()
{
    int * temp_int;

    sleep(2);
    while (process_runed<=NUMBER_OF_PROCESSES)
    {
        sleep(1);
        
        for (int i = 16; i < 32; i++)
        {
            if (linkedlist_peek(p_pid_head[i]))
            {
                
                temp_int=linkedlist_pop(p_pid_head[i]);
                if (temp_int)
                {
                    sem_wait(&s_ready_pid);
                    addLast((void*)temp_int,p_pid_head[16],p_pid_tail[16]);
                    printf("TXT: Boost: Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime,process_table[*temp_int].iRemainingBurstTime);
                    sem_post(&s_ready_pid);

                }
                
            }

         }
        
    }
    

        
    
    return NULL;
}

// Short term
//Schedule the processes to push their own priority
//Wait if there exists new pid ----> sem_wait(&s_new_pid);
//Find new process from new process queue
//Once find it, pop it from  new process queue (new_pid_head) and push to their own priority short term queue(p_pid_head[temp_Priority],p_pid_tail[temp_Priority)
//Finally the amount of ready pid +1---> sem_post(&s_ready_pid);
void *thread_tready(void *arg)
{
    int * temp_int;
    int temp_Priority;
    struct timeval local_basetime;
   
    sleep(0.05);

    gettimeofday(&local_basetime,NULL);
      
    //turn on boost thread
    
    if ( pthread_create( &tvice_short_trem, NULL, thread_tvice_short_trem, NULL ) )
	{
		printf("error creating thread.");
		abort();
    }
  
    while(process_runed<=NUMBER_OF_PROCESSES)
    {
        
        sem_wait(&s_new_pid);

        temp_int=linkedlist_pop(ready_pid_head);
        
        if (temp_int)
        {
            temp_Priority=process_table[*temp_int].iPriority;
            addLast((void*)temp_int,p_pid_head[temp_Priority],p_pid_tail[temp_Priority]);
            printf("TXT: Admitted: Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime,process_table[*temp_int].iRemainingBurstTime);
        }
        
        
         sem_post(&s_ready_pid);

         
     }    
       

    if ( pthread_join ( tvice_short_trem, NULL ) ) {
		printf("error joining thread.");
		abort();
     }

   
         
        
    
    return NULL;
} 


//Consumer1 thread
//Every time find if there exists ready pid in Priority 0-15 and RUN FCFS
//If there is no ready pid in Priority in 0-15  then  RUN RR in Priority 16 queue
//If process is finished push to terminated queue and the terminated of ready pid +1 --->sem_post(&s_terminated_pid)
//Else push to priority short term queue16 again --->addLast((void*)temp_int,p_pid_head[16],p_pid_tail[16])
void *thread_consumer1(void *arg)
{
   int * temp_int;
    sleep(1);
    gettimeofday(&oBaseTime1, NULL);
    while(process_runed<=NUMBER_OF_PROCESSES)
    {  
        sleep(0.01); 
        
        int rr_set=0;// rr_set is to judge if every process is Priority 0 - 15 has finfished
                     // by in  for (int j = 0; j< 16; j++) if it always break,the rr_set is always 0
                     
       for (int j = 0; j< 16; j++)  //Run FCFS in Priority 0 -15        
       {
           int set=0;
           while (1)
            {
                if (linkedlist_peek(p_pid_head[j])==NULL){
                    break;
                }
                
                temp_int=linkedlist_pop(p_pid_head[j]);
                
                if(temp_int)
                {
                    sem_wait(&s_ready_pid);
                    pthread_mutex_lock(&mymutex); // This is to lock Consumer 2 when Consumer 1 is running
                    runNonPreemptiveJob(&(process_table[*temp_int]),oStartTime,oEndTime);
                    addLast((void*)temp_int,Terminated_pid_head,Terminated_pid_tail);
                    int time_difference=getDifferenceInMilliSeconds(*oStartTime,*oEndTime);
                    int res_time= getDifferenceInMilliSeconds(oBaseTime1,*oStartTime)-Time_difference_1;
                    int ron_time = getDifferenceInMilliSeconds(oBaseTime1,*oEndTime)-Time_difference_1;

                    Time_difference_2+=time_difference;

                    printf("TXT: Consumer 1, Process Id = %d (FCFS), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %d, Turnaround Time = %d\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime,process_table[*temp_int].iRemainingBurstTime,res_time,ron_time);
                    printProcessSVG(1,&process_table[*temp_int],oBaseTime1,*oStartTime,*oEndTime); 
                    process_runed++;
                    pthread_mutex_unlock(&mymutex);
                    sem_post(&s_terminated_pid);
                    rr_set=1;
                    set=1;
                    break;
                }
            }
            if (set==1)
            {
            break;
            }
       }

       if (linkedlist_peek(p_pid_head[16])&& rr_set==0 ) //Run RR at Priority 16 queue
       {
           temp_int=linkedlist_pop(p_pid_head[16]);
           if (temp_int)
           {
                pthread_mutex_lock(&mymutex);
                runPreemptiveJob(&(process_table[*temp_int]),oStartTime,oEndTime);
                int time_difference=getDifferenceInMilliSeconds(*oStartTime,*oEndTime);
                int res_time= getDifferenceInMilliSeconds(oBaseTime1,*oStartTime)-Time_difference_1;
                int ron_time = getDifferenceInMilliSeconds(oBaseTime1,*oEndTime)-Time_difference_1;
                Time_difference_2+=time_difference;
                if (process_table[*temp_int].iRemainingBurstTime!=0)
                {
                    printf("TXT: Consumer 1, Process Id = %d (RR), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %d\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime,process_table[*temp_int].iRemainingBurstTime,res_time);
                    printProcessSVG(1,&process_table[*temp_int],oBaseTime1,*oStartTime,*oEndTime); 
                    addLast((void*)temp_int,p_pid_head[16],p_pid_tail[16]);
                    sem_post(&s_ready_pid);
                }else if(process_table[*temp_int].iRemainingBurstTime!=process_table[*temp_int].iInitialBurstTime&&process_table[*temp_int].iRemainingBurstTime!=0)
                {
                    printf("TXT: Consumer 1, Process Id = %d (RR), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime,process_table[*temp_int].iRemainingBurstTime);
                    printProcessSVG(1,&process_table[*temp_int],oBaseTime1,*oStartTime,*oEndTime); 
                    addLast((void*)temp_int,p_pid_head[16],p_pid_tail[16]); 
                    sem_post(&s_ready_pid);
                }else if(process_table[*temp_int].iRemainingBurstTime==0)
                {
                    printf("TXT: Consumer 1, Process Id = %d (RR), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Turnaround Time = %d\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime,process_table[*temp_int].iRemainingBurstTime,ron_time);
                    printProcessSVG(1,&process_table[*temp_int],oBaseTime1,*oStartTime,*oEndTime); 
                    addLast((void*)temp_int,Terminated_pid_head,Terminated_pid_tail); 
                    sem_post(&s_terminated_pid);
                    process_runed++;
                }
                
                
                pthread_mutex_unlock(&mymutex);
            
           }
           
        }
       
     }
    return NULL;
    

}

void *thread_consumer2(void *arg)
{
    
    int * temp_int;
    int Response_Time_t,Turnaround_Time_t;
    sleep(1.1);

    
    gettimeofday(&oBaseTime2, NULL);
    while(process_runed<=NUMBER_OF_PROCESSES)
    {
        sleep(0.01);
        
        int rr_set=0;
        for (int j = 0; j < 16; j++)
        {
            int set=0;
            while (1)
            {
                if (linkedlist_peek(p_pid_head[j])==NULL){
                    
                    break;
                }
                 
                 
                 temp_int=linkedlist_pop(p_pid_head[j]);
                
                if(temp_int)
                {
                    sem_wait(&s_ready_pid);
                    pthread_mutex_lock(&mymutex);
                    runNonPreemptiveJob(&(process_table[*temp_int]),oStartTime,oEndTime);
                    int time_difference=getDifferenceInMilliSeconds(*oStartTime,*oEndTime);
                    
                    int res_time= getDifferenceInMilliSeconds(oBaseTime2,*oStartTime)+1-Time_difference_2;
                    int ron_time = getDifferenceInMilliSeconds(oBaseTime2,*oEndTime)+1-Time_difference_2;
                    Time_difference_1+=time_difference;
                    
                    addLast((void*)temp_int,Terminated_pid_head,Terminated_pid_tail);

                    printf("TXT: Consumer 2, Process Id = %d (FCFS), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %d, Turnaround Time = %d\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime,process_table[*temp_int].iRemainingBurstTime,res_time,ron_time);
                    printProcessSVG(2,&process_table[*temp_int],oBaseTime2,*oStartTime,*oEndTime); 
                    process_runed++;
                    sem_post(&s_terminated_pid);
                    pthread_mutex_unlock(&mymutex);
                    rr_set=1;
                    set=1;
                    break;
                }
            }
            
            if (set==1)
            {
            break;
            }
        }
        ////RR
       if (linkedlist_peek(p_pid_head[16])&&rr_set==0 )
       {
           temp_int=linkedlist_pop(p_pid_head[16]);
           if (temp_int)
           {
                pthread_mutex_lock(&mymutex);
                runPreemptiveJob(&(process_table[*temp_int]),oStartTime,oEndTime);
                int time_difference=getDifferenceInMilliSeconds(*oStartTime,*oEndTime);
                int res_time= getDifferenceInMilliSeconds(oBaseTime2,*oStartTime)-Time_difference_2;
                int ron_time = getDifferenceInMilliSeconds(oBaseTime2,*oEndTime)-Time_difference_2;
                Time_difference_1+=time_difference;
                if (process_table[*temp_int].iRemainingBurstTime!=0)
                {
                    printf("TXT: Consumer 2, Process Id = %d (RR), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Response Time = %d\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime,process_table[*temp_int].iRemainingBurstTime,res_time);
                    printProcessSVG(2,&process_table[*temp_int],oBaseTime2,*oStartTime,*oEndTime); 
                    addLast((void*)temp_int,p_pid_head[16],p_pid_tail[16]);
                    sem_post(&s_ready_pid);
                }else if(process_table[*temp_int].iRemainingBurstTime!=process_table[*temp_int].iInitialBurstTime&&process_table[*temp_int].iRemainingBurstTime!=0)
                {
                    printf("TXT: Consumer 2, Process Id = %d (RR), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime,process_table[*temp_int].iRemainingBurstTime);
                    printProcessSVG(2,&process_table[*temp_int],oBaseTime2,*oStartTime,*oEndTime); 
                    addLast((void*)temp_int,p_pid_head[16],p_pid_tail[16]); 
                    sem_post(&s_ready_pid);
                }else if(process_table[*temp_int].iRemainingBurstTime==0)
                {
                    printf("TXT: Consumer 2, Process Id = %d (RR), Priority = %d, Previous Burst Time = %d, Remaining Burst Time = %d, Turnaround Time = %d\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime,process_table[*temp_int].iRemainingBurstTime,ron_time);
                    printProcessSVG(2,&process_table[*temp_int],oBaseTime2,*oStartTime,*oEndTime); 
                    addLast((void*)temp_int,Terminated_pid_head,Terminated_pid_tail); 
                    sem_post(&s_terminated_pid);
                    process_runed++;
                }
                
                
                pthread_mutex_unlock(&mymutex);
            
           }
           
        }       
        

    }
    return NULL;

}
//Termination Daemon
//The Termination thread 
//Wait if there exists terminated pid --->sem_wait(&s_terminated_pid);
//Find free process from Terminated process queue
//Once find it, pop it from  Terminated process queue (Terminated_pid_head) and push to free process queue(free_pid_head,free_pid_tail)
//Finally the amount of new pid +1 [sem_post(&s_new_pid)]
void *thread_tterminated(void *arg)
{
    
    int * temp_int;
     while(process_runed<=NUMBER_OF_PROCESSES)
    {
        
        sem_wait(&s_terminated_pid);
        temp_int=linkedlist_pop(Terminated_pid_head);
        
        if (temp_int){
            
            printf("TXT: Terminated: Process Id = %d, Priority = %d, Previous Burst Time = %d, Remaining Burst Time = 0\n",*temp_int,process_table[*temp_int].iPriority,process_table[*temp_int].iPreviousBurstTime);
            addLast((void*)temp_int,free_pid_head,free_pid_tail);
        }
        sleep(0.05);
        sem_post(&s_free_pid);
    }
    return NULL;

}






int main(int argc,char**argv)
{
    printHeadersSVG();
    printPrioritiesSVG();
    printRasterSVG();
	
	free_pid_head=(struct element **)malloc(sizeof(struct element *));
    free_pid_tail=(struct element **)malloc(sizeof(struct element *));
    ready_pid_head=(struct element **)malloc(sizeof(struct element *));
    ready_pid_tail=(struct element **)malloc(sizeof(struct element *));
    Terminated_pid_head=(struct element **)malloc(sizeof(struct element *));
    Terminated_pid_tail=(struct element **)malloc(sizeof(struct element *));
    oEndTime=(struct timeval *)malloc(sizeof(struct timeval ));
    oStartTime=(struct timeval *)malloc(sizeof(struct timeval ));
    
    for (int i = 0; i < MAX_PRIORITY; i++)
    {
        p_pid_head[i]=(struct element **)malloc(sizeof(struct element *));
        p_pid_tail[i]=(struct element **)malloc(sizeof(struct element *));
       

    }
    //First simulate that there are 128 processes to run
    
    for (int i = 0; i < SIZE_OF_PROCESS_TABLE; i++)
	{
		pid[i]=i;
		process_pointer=generateProcess(&pid[i]);
		process_table[i]=*process_pointer;
        free(process_pointer);
    }
    for (int i = 0; i < SIZE_OF_PROCESS_TABLE; i++)
    {
        
        addLast((void*)&pid[i],free_pid_head,free_pid_tail);
        
    }

    sem_init(&s_free_pid,0,128);
    sem_init(&s_new_pid,0,0);
    sem_init(&s_ready_pid,0,0);
    sem_init(&s_terminated_pid,0,0);
 

    if ( pthread_create( &tgenerate, NULL, thread_tgenerate, NULL ) )
	{
		printf("error creating thread.");
		abort();
    }

     if ( pthread_create( &tready, NULL, thread_tready, NULL ) )
	{
		printf("error creating thread.");
		abort();
    }
   	if ( pthread_create( &tconsumer1, NULL, thread_consumer1, NULL ) )
	{
		printf("error creating thread.");
		abort();
    }
   	if ( pthread_create( &tconsumer2, NULL, thread_consumer2, NULL ) )
	{
		printf("error creating thread.");
		abort();
    }
    
    if ( pthread_create( &tterminated, NULL, thread_tterminated, NULL ) )
	{
		printf("error creating thread.");
		abort();
    }

    

    
    if ( pthread_join ( tterminated, NULL ) ) {
		printf("error joining thread.");
		abort();
     }

	if ( pthread_create( &tconsumer2, NULL, thread_consumer2, NULL ) )
	{
		printf("error creating thread.");
		abort();
    }
    if ( pthread_join ( tconsumer1, NULL ) ) {
		printf("error joining thread.");
		abort();
     }
    if ( pthread_join ( tgenerate, NULL ) ) {
		printf("error joining thread.");
		abort();
     }
    if ( pthread_join ( tready, NULL ) ) {
		printf("error joining thread.");
		abort();
     }
     free(free_pid_head);
     free(free_pid_tail);
     free(ready_pid_head);
     free(ready_pid_tail);

     free(Terminated_pid_head);
     free(Terminated_pid_tail);
     for (int i = 0; i < MAX_PRIORITY; i++)
     {
        free(p_pid_head[i]);
        free(p_pid_tail[i]);
     }
     
     printFootersSVG();
    return 0;

}