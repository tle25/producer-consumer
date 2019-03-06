#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

//global variables
sem_t full, empty;
pthread_mutex_t mutex;
pthread_attr_t attr;
int Ptime;
int Ctime;
int max_buf;
int *buffer, *PArray, *CArray;
int items_produced;
int items_consumed;
int buffer_index = 0, PArray_index = 0, CArray_index = 0;
int global_items = 1;
int over_consume_amount;
int num_consumer_thread;
int consume_more = 0;
/* 
 * Function to remove item.
 * Item removed is returned
 */
int dequeue_item()
{
	int val = buffer[0];
if(buffer_index > 1){
	int *temp = malloc(sizeof(int)*buffer_index);
	for(int i = 0; i < buffer_index - 1; i++){
		temp[i] = buffer[i+1];
	}

	for(int i = 0; i < buffer_index; i++){
		buffer[i] = temp[i];
	}
	buffer_index--;
	free(temp);
}
else{
buffer_index--;
}
   return val;

}

/* 
 * Function to add item.
 * Item added is returned.
 * It is up to you to determine
 * how to use the ruturn value.
 * If you decide to not use it, then ignore
 * the return value, do not change the
 * return type to void. 
 */
int enqueue_item(int item)
{
   buffer[buffer_index++] = item;
}

struct ThreadArgs
{
 	pthread_t tid;
    int id;
    int count;
};



void* producer(void * arg)
{
    struct ThreadArgs* args = (struct ThreadArgs*)arg;
   int item_produced;

while((args->count)-- > 0){
    	// Produce an item in next_produced
    	sem_wait(&empty);
    	pthread_mutex_lock(&mutex);

// display and put the item being produced to producer array	

item_produced = global_items++;
enqueue_item(item_produced);
PArray[PArray_index++] = item_produced;
printf("%d was produced by producer-> %d\n", item_produced, args->id);

	sleep(Ptime);	
   		pthread_mutex_unlock(&mutex);
		sem_post(&full);
    
    //	printf("%d was produced by producer-> %d\n", count, args->id);


    }
    pthread_exit(0);
	//return NULL;
}



void* consumer(void * arg)
{
    struct ThreadArgs* args = (struct ThreadArgs*)arg;
    int item_consumed;

//The last thread consumes more items than other thread
// if there P*X/C is not evenly divisible

    while((args->count)-- > 0){

    	sem_wait(&full);
    	pthread_mutex_lock(&mutex);

    // Remove an first item from buffer to next consumed
    // display and put the item being consumed to consumer array	
    item_consumed = dequeue_item();
    CArray[CArray_index++] = item_consumed;
	printf("%d was consumed by consumer-> %d\n", item_consumed, args->id);
	sleep(Ctime);
		pthread_mutex_unlock(&mutex);
    	sem_post(&empty);

    };
    pthread_exit(0);
	//return NULL;
}
int main(int argc, char** argv) {
// time when program starts
time_t begin = time(NULL);
printf("Current time: %s\n", asctime(localtime(&begin)));



int over_consume = 0;
	if(argc != 7){
		printf("Error command arguments\n");
		exit(1);
	}

	// Get value args
	max_buf = atoi(argv[1]); //numbers of threads
	int num_producer_thread = atoi(argv[2]);
	num_consumer_thread = atoi(argv[3]);
	items_produced = atoi(argv[4]);

	
	int val = num_producer_thread*items_produced;
	if((val % num_consumer_thread) != 0){
		over_consume = 1;
		items_consumed = val / num_consumer_thread;
		over_consume_amount = items_consumed + val % items_consumed;
	}

	else{
		items_consumed = val / num_consumer_thread;
		over_consume_amount = items_consumed;
	}

	Ptime = atoi(argv[5]);
	Ctime = atoi(argv[6]);


// Print messages from command args	
	printf("Number of Buffers: %d\n", max_buf);
	printf("Number of Producers: %d\n", num_producer_thread);
	printf("Number of Consumers: %d\n", num_consumer_thread);
	printf("Number of items Produced by each producer: %d\n", items_produced);
	printf("Number of items Consumed by each consumer: %d\n", items_consumed);
	printf("Over consume on?: %d\n", over_consume);
	printf("Over consume amount: %d\n", over_consume_amount);
	printf("Time each Producer Sleeps (second) : %d\n", Ptime);
	printf("Time each Consumer Sleeps (second) : %d\n", Ctime);



    // Initialize semophores and mutex
  	if(sem_init(&full,0,0) < 0){
  		perror("error at full init");
  	}

  	if(sem_init(&empty, 0, max_buf) < 0){
  		perror("error at empty init");
  	}
	if(pthread_mutex_init(&mutex, NULL) < 0){
		perror("error at mutex init");
	}

	//default attributes
	pthread_attr_init(&attr);

	int ret_val;

	//Initialize buffer, producer array and consumer array
	buffer = malloc(sizeof(int) * max_buf);
	PArray = malloc(sizeof(int) * num_producer_thread * items_produced);
	CArray = malloc(sizeof(int) * num_consumer_thread * 
		(items_consumed + over_consume));

	// Create producer and consumer threads;
	struct ThreadArgs producer_thread[num_producer_thread];
	struct ThreadArgs consumer_thread[num_consumer_thread];
	
	//spawn producer threads and consumer threads
	//Creating producer threads
	for(int i=0; i < num_producer_thread;i++)
    {
    	//producer_thread[i].id = i+1;
    	producer_thread[i].id = i+1;
        producer_thread[i].count = items_produced;

        ret_val = pthread_create(&producer_thread[i].tid, &attr, producer, (void *)&producer_thread[i]);

        if(ret_val < 0)
        {
            perror("Error creating producer thread..");
            return -2;
        }
    }


    // Creating consumer threads
    for(int i=0; i < num_consumer_thread ;i++)
    {
    	//consumer_thread[i].id = i+1;
    	consumer_thread[i].id = i+1;
    	//The last thread consumes more items than other thread
		// if there P*X/C is not evenly divisible
    	if(i+1 == num_consumer_thread && over_consume == 1){
    	consumer_thread[i].count = over_consume_amount;
    	}

    	else{
    		consumer_thread[i].count = items_consumed;
    	}

    	//create threads
        ret_val = pthread_create(&consumer_thread[i].tid, &attr, consumer, (void *)&consumer_thread[i]);

        if(ret_val < 0)
        {
            perror("Error creating consumer thread..");
            return -2;
        }
    }



	for(int i = 0; i < num_producer_thread; i++)
    {
        pthread_join(producer_thread[i].tid, NULL);      

        printf("Producer thread joined: %d\n", i+1);


	}


	for(int i = 0; i < num_consumer_thread; i++)
    {
        pthread_join(consumer_thread[i].tid, NULL);      

        printf("Consumer thread joined: %d\n", i+1);


	}


time_t end = time(NULL);
printf("Current time: %s\n", asctime(localtime(&end)));

//float time = ((end - begin)/CLOCKS_PER_SEC); //time duration



//Test if producer array and consumer array are identical
printf(" Producer Array | Consumer Array\n");
int true = 1;
for(int i = 0; i < PArray_index; i++){
		int num1 = PArray[i], num2;

		// Number of items produced is always greater than or equal numbers of items consumed
		// Check if number consume is not equal numbers of produce
		if(i < CArray_index){
		num2 = CArray[i];
		}
		


		if(num1 == num2 && true == 1){
		printf("\t%d\t| \t%d\n", num1, num2);
		}

		else{
			printf("\t%d\t| \n", num1);
			true = 0;
		}
	}

	if(true == 1){
		printf("Consume and Produce Arrays match!\n");
	}
	else{
		printf("Consume and Produce Arrays no match!\n");
	}






int time = end - begin;
printf("Total Runtime: %d sec\n", time);

	pthread_mutex_destroy(&mutex);
	sem_destroy(&full);
	sem_destroy(&empty);
	free(buffer);
	free(PArray);
	free(CArray);

return 0;

}
