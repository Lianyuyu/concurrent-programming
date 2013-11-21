#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#define RAND_DIVISOR 100000000
#define TRUE 1

int numProd;
int numCons;
int numMaxInpipe;
int numRecordPerProd;
pthread_t tid;       //Thread ID
int file_pipes[2];
int flag_end_counter_consumer_read; //record how many "end" records have been read from pipe
int flag_end_counter_producer_write;// record how many "end" records have been written to pipe
int flag_start_counter_producer;//record how many producers run the first line of code in threads
int flag_start_counter_consumer;//record how many consumers run the first line of code in threads
int flag_producer_write;//record how many records were written to pipe by all producer;
int flag_consumer_read;//reocrd how many rocords were read from pipe by all consumer;
int **consumer_read_records;//record how many message read in each consumer
int flag_kill_all_threads;/* when threads can not exit normally*/

/* the semaphores */
sem_t full, empty, sem_mutex;
sem_t sem_mutex_producer_write, sem_mutex_consumer_read, sem_mutex_end_counter_consumer_read, sem_mutex_end_counter_producer_write;

pthread_attr_t attr; //Set of thread attributes

void *producer(void *param); /* the producer thread */
void *consumer(void *param); /* the consumer thread */

void initializeData()
{

    /* Create the full semaphore and initialize to 0 */
    sem_init(&full, 0, 0);

    /* Create the empty semaphore and initialize to BUFFER_SIZE */
    sem_init(&empty, 0, numMaxInpipe);

    sem_init(&sem_mutex, 0, 1);

    sem_init(&sem_mutex_producer_write, 0 ,1);
    sem_init(&sem_mutex_consumer_read, 0 ,1);
    sem_init(&sem_mutex_end_counter_consumer_read, 0 ,1);
    sem_init(&sem_mutex_end_counter_producer_write, 0 ,1);

    /* Get the default attributes */
    //pthread_attr_init(&attr);

    flag_end_counter_consumer_read = 0;
    flag_end_counter_producer_write = 0;
    flag_start_counter_consumer = 0;
    flag_start_counter_producer = 0;
    flag_producer_write = 0;
    flag_consumer_read = 0;
    flag_kill_all_threads = 0;
}

void cancel_all_threads(pthread_t* tidsCons, pthread_t monitor_all_infor_tid, pthread_t monitor_deadlock_tid)
{
    int i,rc;


    for(i = 0; i< numCons; i++)
    {
        //rc = pthread_kill(tidsCons[i],SIGALRM);
        rc = pthread_cancel(tidsCons[i]);
        if(rc != 0)
        {
            printf("can not kill consumer %d \n",i);
        }
    }
    //rc = pthread_kill(monitor_all_infor_tid,SIGALRM);
    rc = pthread_cancel(monitor_all_infor_tid);
    if(rc != 0)
    {
        printf("can not kill monitor_all_infor_tid %d \n",rc);

    }

    rc = pthread_cancel(monitor_deadlock_tid);
    if(rc != 0)
    {
        printf("can not kill monitor_deadlock_tid %d \n",rc);
    }

    printf("All threads are canceled. \n");
}

//check all information about producers and consumers
void *monitor_all_infor(void *param)
{
    int full_value, empty_value;
    char input[20];

    while(1)
    {
        usleep(500000);
        sem_getvalue(&full, &full_value);
        sem_getvalue(&empty, &empty_value);
        printf("\nmonitor information\n");
        printf("sem full %d sem empty %d\n", full_value,empty_value);
        printf("flag_end_counter_producer_write %d \n",flag_end_counter_producer_write );
        printf("flag_end_counter_consumer_read %d \n",flag_end_counter_consumer_read );
        printf("flag_start_counter_producer %d \n", flag_start_counter_producer);
        printf("flag_start_counter_consumer %d \n", flag_start_counter_consumer);
        printf("flag_producer_write %d \n", flag_producer_write);
        printf("flag_consumer_read %d \n", flag_consumer_read);
        printf("monitor information\n\n");
    }
}

//check whether the program run into deadlock
void *monitor_deadlock(void *param)
{

    int counter, full_value, empty_value;
    counter = 0;
    while(1)
    {
        usleep(500000);
        sem_getvalue(&full, &full_value);
        sem_getvalue(&empty, &empty_value);
        if(full_value == 0 && empty_value == numMaxInpipe)
        {
            counter++;
        }
        if(counter == 5)
        {
            flag_kill_all_threads = 1;
        }
    }
}

/* Producer Thread */
void *producer(void *param)
{
    flag_start_counter_producer++;

    int i, record, producer_id;
    char recordString[512];

    producer_id = (int)param;

    for(i=1; i<numRecordPerProd+1; i++)
    {
        sprintf(recordString, "producer %d in turn %d write %d", producer_id, i,i);

        /* acquire the empty lock */
        sem_wait(&empty);

        /* acquire the mutex lock */
        //sem_wait(&sem_mutex);

        write(file_pipes[1],recordString,512);

        /* release the mutex lock */
        //sem_post(&sem_mutex);

        /* signal full */
        sem_post(&full);

        sem_wait(&sem_mutex_producer_write);
        flag_producer_write++;
        sem_post(&sem_mutex_producer_write);

        //printf("%s \n",recordString);
    }

    strcpy(recordString,"end");

    sem_wait(&empty);

    //sem_wait(&sem_mutex);
    write(file_pipes[1],recordString,512);

    //sem_post(&sem_mutex);

    sem_post(&full);

    sem_wait(&sem_mutex_producer_write);
    flag_producer_write++;
    sem_post(&sem_mutex_producer_write);

    sem_wait(&sem_mutex_end_counter_producer_write);
    flag_end_counter_producer_write++;
    sem_post(&sem_mutex_end_counter_producer_write);

    //sprintf(recordString, "producer %d in turn %d write end", producer_id, i);
    //printf("%s \n",recordString);

    pthread_exit(NULL);
}

/* Consumer Thread */
void *consumer(void *param)
{
    flag_start_counter_consumer++;
    int record, consumer_id;
    char recordString[512];
    int i = 1;
    char consumer_file_name[100],file_name[20];
    char consumer_output[50];
    FILE *outputfile;

    consumer_id = (int)param;
    consumer_read_records[consumer_id][0] = 0;
    strcpy(file_name,"consumer");

    sprintf(consumer_file_name, "%s-%d",file_name, consumer_id);
    outputfile = fopen(consumer_file_name,"wt");
    if( outputfile == NULL)
    {
        printf("Cannot create the output file: %s !\n",consumer_file_name);
        return 0;
    }

    while(TRUE)
    {
        /* aquire the full lock */
        sem_wait(&full);

        /* aquire the mutex lock */
        //sem_wait(&sem_mutex);

        read(file_pipes[0], recordString, 512);

        /* release the mutex lock */
        //sem_post(&sem_mutex);

        /* signal empty */
        sem_post(&empty);

        consumer_read_records[consumer_id][0]++;

        sem_wait(&sem_mutex_consumer_read);
        flag_consumer_read++;
        sem_post(&sem_mutex_consumer_read);

        if(strcmp(recordString,"end") == 0)
        {
            sem_wait(&sem_mutex_end_counter_consumer_read);
            flag_end_counter_consumer_read++;
            sem_post(&sem_mutex_end_counter_consumer_read);
        }

        sprintf(consumer_output, "consumer %d in turn %d read - %s - \n",consumer_id , i ,recordString);
        //printf("%s \n",consumer_output);

        fprintf(outputfile,"%s",consumer_output);
        i = i + 1;
    }
}

int main(int argc, char *argv[])
{
    /* Loop counter */
    int i;
    int numTurn;
    pthread_t monitor_all_infor_tid, monitor_deadlock_tid;
    int err,rc;
    char check_file_name[20];
    int flags_read;
    struct sigaction actions;
    int all_records_read, all_records_write;

    /* Verify the correct number of arguments were passed in */
    if(argc != 5)
    {
        fprintf(stderr, "USAGE:./sempipetest <numproducers> <numconsumers> <itemsperproducer> <maxinpipe> \n");
        exit(0);
    }

    numProd = atoi(argv[1]); /* Number of producer threads */
    numCons = atoi(argv[2]); /* Number of consumer threads */
    numRecordPerProd = atoi(argv[3]);
    numMaxInpipe = atoi(argv[4]);


    flags_read |= O_NONBLOCK;
    fcntl(file_pipes[0], F_SETFL, flags_read);

    pthread_t tidsProd[numProd];
    pthread_t tidsCons[numCons];
    consumer_read_records = (int** )malloc(numCons*sizeof(int*));
    for(i =0; i<numCons; i++)
    {
        consumer_read_records[i] = (int*) malloc(2*sizeof(int));
    }

    /* Initialize the app */
    initializeData();

    /* create pipe*/
    if(pipe(file_pipes) == -1)
    {
        printf("pipe error\n");
        exit(1);
    }

    /* monitor thread*/

    /* Create the producer threads */
    for(i = 0; i < numProd; i++)
    {
        /* Create the thread */
        numTurn = i;
        rc = pthread_create(&tid,NULL,producer,(void*)numTurn);
        if(rc != 0)
        {
            printf(" producer %d ERROR: return code from pthread_create is %d \n", i, rc);
            exit(1);
        }
        tidsProd[i] = tid;
    }

    /* Create the consumer threads */
    for(i = 0; i < numCons; i++)
    {
        /* Create the thread */
        numTurn = i;
        rc = pthread_create(&tid,NULL,consumer,(void*)numTurn);
        if( rc != 0)
        {
            printf("consumer %d ERROR: return code from pthread_create is %d \n",i, rc);
            exit(1);
        }
        tidsCons[i] = tid;
    }
    printf("All producer and consumer threads are created.\n");

    //create monitor_all_information thread
    rc = pthread_create(&monitor_all_infor_tid, NULL, monitor_all_infor,NULL);
    if(rc != 0)
    {
        printf("~ ERROR: return code from pthread_create is %d \n",  rc);
        exit(1);
    }

    //create monitor_deadlock thread
    rc = pthread_create(&monitor_deadlock_tid, NULL, monitor_deadlock,NULL);
    if(rc != 0)
    {
        printf("~ ERROR: return code from pthread_create is %d \n", rc);
        exit(1);
    }

    //wait for all producers exit
    for(i = 0; i < numProd; i++)
    {
        pthread_join(tidsProd[i], NULL);
    }
    printf("All producer threads exited.\n");

    //monitor when all records have been read from pipe
    while(1)
    {
        //usleep(500000);
        if(flag_end_counter_consumer_read == numProd)
        {
            break;
        }
        //if run into dead lock, force to kill all threads
        if(flag_kill_all_threads == 1)
        {
            cancel_all_threads(tidsCons, monitor_all_infor_tid, monitor_deadlock_tid);
            printf("can not successfully read all records!\n");
            printf("The program is forcedly terminated !\n");
            exit(1);
        }
    }

    printf("flag_end_counter_consumer_read %d end, consumers received all 'end' records\n",flag_end_counter_consumer_read);

    all_records_read = 0;
    for(i =0; i<numCons; i++)
    {
        printf("consumer %d read_records %d lines. \n",i,consumer_read_records[i][0]);
        all_records_read = all_records_read + consumer_read_records[i][0];
    }

    all_records_write = (numProd * numRecordPerProd) + numProd;

    if(all_records_read == all_records_write)
    {
        printf("The number of all consumer_read_records %d is equal to (numProd * numRecordPerProd + numProd) %d \n",all_records_read,all_records_write);
    }

    cancel_all_threads(tidsCons, monitor_all_infor_tid, monitor_deadlock_tid);

    printf("Exit the program\n");
    exit(0);
}

