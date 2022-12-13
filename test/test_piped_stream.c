#include<piped_stream.h>

#include<executor.h>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

stream pyp_strm;
#define MAX_CAPACITY 12

#define PRODUCERS_COUNT 1
#define CONSUMERS_COUNT 1

char* input[PRODUCERS_COUNT][7] = {
	{
		"Hello, Rohan !!",
		"How are you?",
		"How are you doing?",
		"Get well soon, son.",
		"Thank you so much, Ms. Rupa.",
		"You are always welcomed here.",
		"I know that Ms. Rupa."
	}
};

void* producer(void* param)
{
	int id = (int) param;

	for(int i = 0; i < sizeof(input[id])/sizeof(input[id][0]); i++)
	{
		unsigned int data_size = strlen(input[id][i]);

		unsigned int bytes_written = write_to_stream(&pyp_strm, input[id][i], data_size);
		if(pyp_strm.error)
			break;

		printf("producer(%d) : <%s>\n", id, input[id][i]);
	}

	// close stream once all the data has been written
	close_stream(&pyp_strm);

	printf("producer(%d) : EXIT\n", id);

	return NULL;
}

#define CONSUMER_READ_BUFFER_SIZE 8

void* consumer(void* param)
{
	int id = (int) param;

	char read_buffer[CONSUMER_READ_BUFFER_SIZE];

	while(1)
	{
		unsigned int bytes_read = read_from_stream(&pyp_strm, read_buffer, CONSUMER_READ_BUFFER_SIZE);
		if(bytes_read == 0 || pyp_strm.error)
			break;
		printf("consumer(%d) : <%.*s>\n", id, bytes_read, read_buffer);
	}

	printf("consumer(%d) : EXIT\n", id);

	return NULL;
}

int main()
{
	initialize_piped_stream(&pyp_strm, MAX_CAPACITY);

	// initialize job executor for executing transactions with the server
	executor* transaction_executor = new_executor(FIXED_THREAD_COUNT_EXECUTOR, PRODUCERS_COUNT + CONSUMERS_COUNT, 10, 0, NULL, NULL, NULL);

	// submit consumer jobs
	for(int i = 0; i < CONSUMERS_COUNT; i++)
		submit_job(transaction_executor, consumer, ((void*)i), NULL, 0);

	// submit producer jobs
	for(int i = 0; i < PRODUCERS_COUNT; i++)
		submit_job(transaction_executor, producer, ((void*)i), NULL, 0);

	// shutdown, wait for shutdown and delete the job executor
	shutdown_executor(transaction_executor, 0);
	wait_for_all_threads_to_complete(transaction_executor);
	delete_executor(transaction_executor);

	// destroy stream
	deinitialize_stream(&pyp_strm);
}