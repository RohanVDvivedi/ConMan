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

void* transaction_handler(transaction_handler_params* params);

transaction_client* get_transaction_client(connection_group* conn_group, unsigned long long int connection_count)
{
	transaction_client* tclient = (transaction_client*) malloc(sizeof(transaction_client));
	tclient->conn_group = conn_group;
	tclient->manager = get_fixed_connection_thread_pool_manager(connection_count, (void* (*)(void*))(transaction_handler));
	return tclient;
}

void* transaction_handler(transaction_handler_params* params)
{
	int fd = -1;

	// find fd_p, that can be used by the current thread
	fd = get_file_discriptor_for_current_thread(params->tclient->manager);

	// if not found make_connection, and insert the entry
	if(fd == -1)
	{
		// try to make a connection
		fd = make_connection(params->tclient->conn_group);

		// insert a new entry for thread_id_to_file_discriptor hashmap
		// we insert entry only if we were successfull in. making a connection
		if(fd != -1)
		{
			insert_mapping_with_current_thread(params->tclient->manager, fd);
		}
	}

	// the fd_p = NULL check here
	// is to confirm, if the connection, we tried to make was successfull
	if(fd == -1)
	{
		printf("making connection failed\n");
		return NULL;
	}

	int close_connection_requested = 0;

	// execute the transaction
	void* result = params->transaction(fd, &close_connection_requested, params->additional_params);

	if(close_connection_requested)
	{
		remove_mapping_for_current_thread(params->tclient->manager);

		// close the connection
		close_connection(fd);
	}

	return result;
}

job* queue_transaction(transaction_client* tclient, void* (*transaction)(int fd, int* close_connection_requested, void* additional_params), void* additional_params)
{
	transaction_handler_params* params = (transaction_handler_params*) malloc(sizeof(transaction_handler_params));
	params->tclient = tclient;
	params->additional_params = additional_params;
	params->transaction = transaction;
	job* job_p = submit_job_with_promise(tclient->manager, params);
	if(job_p == NULL)
	{
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
	void* result = get_promised_result(job_p);

	// delete the input parameters of the job only after you have got the result
	free(params);

	return result;
}

void shutdown_transaction_client(transaction_client* tclient)
{
	close_all_connections_and_wait_for_shutdown(tclient->manager);
}

void delete_transaction_client(transaction_client* tclient)
{
	delete_connection_thread_pool_manager(tclient->manager);
	free(tclient);
}