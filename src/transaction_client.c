#include<transaction_client.h>

typedef struct transaction_handler_params transaction_handler_params;
struct transaction_handler_params
{
	// the additional_params, to be given to the transaction_function
	// the transaction is responsible to delete the additional params
	void* additional_params;

	// this is the transaction, that the transaction_handler has to execute
	// set close_connection_requested to 1, if the connection is corrupted/closed by server, so the connection will be closed, and restarted on any other transaction
	// your transaction must return a result, that you receive after talking with the server
	void* (*transaction)(int fd, int* close_connection_requested, void* additional_params);

	// this is the transaction_client, that this transaction belongs to
	// tclient->manager->connection_mapping where you need to explicitly specify, on what connection file discriptor you are working with
	// tclient->connection_group, which you will have to use to open new connection
	transaction_client* tclient;
};

static void* transaction_handler(transaction_handler_params* params);
static void transaction_worker_finish(void* args);

// Thread specific data, every thread of the transaction client, will reference it'c own connection file discriptor using this key
static pthread_key_t connection_file_discriptor_key;

// Control once to create this pthread key (above) only once
static pthread_once_t connection_file_discriptor_key_once_control = PTHREAD_ONCE_INIT;

static void connection_file_discriptor_key_create()
{
	pthread_key_create(&connection_file_discriptor_key, free);
}

transaction_client* get_transaction_client(connection_group conn_group, unsigned long long int connection_count)
{
	// key create once call => may or may not be called, based on previous invocation
	pthread_once(&connection_file_discriptor_key_once_control, connection_file_discriptor_key_create);
	
	transaction_client* tclient = (transaction_client*) malloc(sizeof(transaction_client));
	tclient->conn_group = conn_group;
	tclient->transaction_executor = get_executor(FIXED_THREAD_COUNT_EXECUTOR, connection_count, 0, NULL, transaction_worker_finish, NULL);
	return tclient;
}

static void* transaction_handler(transaction_handler_params* params)
{
	// find fd or create fd, that can be used by the current thread
	int* fd_p = pthread_getspecific(connection_file_discriptor_key);
	if(fd_p == NULL)
	{
		fd_p = malloc(sizeof(int));
		pthread_setspecific(connection_file_discriptor_key, fd_p);
		*fd_p = -1;
	}

	// if not found make_connection, and insert the entry
	if(*fd_p == -1)
	{
		// try to make a connection
		*fd_p = make_connection(&(params->tclient->conn_group));
	}

	// to confirm, if the connection, we tried to make was successfull
	if(*fd_p == -1)
	{
		printf("making connection failed\n");
		return NULL;
	}

	int close_connection_requested = 0;

	// execute the transaction
	void* result = params->transaction(*fd_p, &close_connection_requested, params->additional_params);

	if(close_connection_requested)
	{
		// close the connection
		close(*fd_p);
	}

	return result;
}

static void transaction_worker_finish(void* args)
{
	// find fd or create fd, that can be used by the current thread
	int* fd_p = pthread_getspecific(connection_file_discriptor_key);
	if(fd_p != NULL)
	{
		close(*fd_p);
	}
}

job* queue_transaction(transaction_client* tclient, void* (*transaction)(int fd, int* close_connection_requested, void* additional_params), void* additional_params)
{
	transaction_handler_params* params = (transaction_handler_params*) malloc(sizeof(transaction_handler_params));
	params->tclient = tclient;
	params->additional_params = additional_params;
	params->transaction = transaction;
	job* job_p = get_job( (void*(*)(void*)) transaction_handler, params);
	if(!submit_job(tclient->transaction_executor, job_p))
	{
		delete_job(job_p);
		free(params);
	}
	return job_p;
}

void* get_result_for_transaction(job* job_p, void** input_p)
{
	if(job_p == NULL)
	{
		return NULL;
	}
	transaction_handler_params* params = job_p->input_p;
	(*input_p) = params->additional_params;
	void* result = get_result(job_p);

	// delete the input parameters of the job only after you have got the result
	delete_job(job_p);
	free(params);

	return result;
}

void shutdown_transaction_client(transaction_client* tclient)
{
	// ### TODO
	// need to call close on all connection, some how

	shutdown_executor(tclient->transaction_executor, 1);

	wait_for_all_threads_to_complete(tclient->transaction_executor);
}

void delete_transaction_client(transaction_client* tclient)
{
	delete_executor(tclient->transaction_executor);
	free(tclient);
}