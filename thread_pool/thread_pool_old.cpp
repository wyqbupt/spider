#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <pthread.h>  
#include <assert.h>
/* the node of work_event list*/
typedef struct worker{

    void *(*process)(void*arg);
    void *arg;
    struct worker * next;

}Thread_worker;

/*thread pool*/
typedef struct{
    pthread_mutex_t queue_lock;
    pthread_cond_t ready;
	Thread_worker* work_head;
	int shutdown;// if or not shutdown the thread_pool
	pthread_t *threadid;
	int max_thread_num;
	int curr_work_size;
}Thread_pool;

/*ervent thread will use this fun to solve the event*/
void *thread_routine (void *arg); 

Thread_pool *pool = NULL;//thread pool

void init(int max_thread_size){
	pool = (Thread_pool*)malloc(sizeof(Thread_pool));
	pthread_mutex_init(&(pool->queue_lock),NULL);
	pthread_cond_init(&(pool->ready),NULL);
	pool->shutdown = 0;
	pool->work_head = NULL;
	pool->threadid = (pthread_t*)malloc(max_thread_size*sizeof(pthread_t));
	pool->curr_work_size = 0;
	pool->max_thread_num = max_thread_size;
	int i = 0;
	/*create max_thread_size thread*/
	for(i = 0;i < max_thread_size;i++){
		pthread_create(&(pool->threadid[i]),NULL,thread_routine,NULL);
	}
}

/*insert work to event list*/
void insert_work_queue(void *(*process) (void *arg), void *arg){
	Thread_worker * work = (Thread_worker*)malloc(sizeof(Thread_worker));
	work->process = process;
	work->arg = arg;
	work->next = NULL;
	/*insert this event to the list*/
	pthread_mutex_lock(&(pool->queue_lock));
	Thread_worker *member = pool->work_head;
	if(member != NULL){
		while(member->next != NULL)
			member = member->next;
		member->next = work;
	}
	else
		pool->work_head = work;
	assert(pool->work_head != NULL);
	pool->curr_work_size++;
	pthread_mutex_unlock(&(pool->queue_lock));
	pthread_cond_signal(&(pool->ready));
}
   
void *thread_routine (void *arg){
	printf ("thread is starting,the id is 0x%x\n", (unsigned)pthread_self());  
	while(1){
		pthread_mutex_lock (&(pool->queue_lock));
		while(pool->curr_work_size == 0 && pool->shutdown == 0){
			printf ("thread 0x%x is waiting\n", (unsigned)pthread_self());
			pthread_cond_wait(&(pool->ready),&(pool->queue_lock));
		}
		if(pool->shutdown){
			pthread_mutex_unlock(&(pool->queue_lock));
			printf ("thread 0x%x will exit\n", (unsigned)pthread_self());  
			pthread_exit(NULL);
		}
		assert(pool->curr_work_size != 0);
		assert(pool->work_head != NULL);
		Thread_worker * work = pool->work_head;
		pool->work_head = work->next;
		pool->curr_work_size--;
		pthread_mutex_unlock(&(pool->queue_lock));
		(*(work->process))(work->arg);
		free(work);
		work = NULL;
	}
}

/* destroy the thread_poo*/
void pool_destroy(){
	if(pool->shutdown)
		return;
	pool->shutdown = 1;
	
	pthread_cond_broadcast (&(pool->ready));//wake waitting thread*/
	int i = 0;

	for(i = 0;i < pool->max_thread_num;i++)
		pthread_join(pool->threadid[i],NULL);

	free(pool->threadid);
	Thread_worker * work = NULL;
	while(pool->work_head != NULL){//destroy event list
		work = pool->work_head;
		pool->work_head = work->next;
		free(work);
	}
	//destroy singal and mutex
	pthread_mutex_destroy(&(pool->queue_lock));
	pthread_cond_destroy(&(pool->ready));
	
	free(pool);
	pool = NULL;//make this func more safe
}
/*test code*/
void *myprocess (void *arg)  
{  
    printf ("threadid is 0x%x, working on task %d\n", (unsigned)pthread_self(),*(int *) arg);  
    sleep (1); 
    return NULL;  
}  
  
int  main(int argc, char **argv)  
{  
    init (3);  
      
    /*10 test events*/  
    int *workingnum = (int *) malloc (sizeof (int) * 10);  
    int i;  
    for (i = 0; i < 10; i++)  
    {  
        workingnum[i] = i;  
        insert_work_queue(myprocess, &workingnum[i]);  
    }  
    /*等待所有任务完成*/  
    sleep (5);  
    /*销毁线程池*/  
    pool_destroy ();  
  
    free(workingnum);  
    return 0;  
}  
