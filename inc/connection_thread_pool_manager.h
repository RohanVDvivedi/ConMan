#ifndef CONNECTION_THREAD_POOL_MANAGER_H
#define CONNECTION_THREAD_POOL_MANAGER_H

#include<stdio.h>
#include<stdlib.h>

#include<executor.h>

#include<connection_mapping.h>

typedef struct connection_thread_pool_manager connection_thread_pool_manager;
struct connection_thread_pool_manager
{
	// this is the executor service the connection thread pool will be built on
	executor* thread_pool;

	// the handler that will be used by the thread_pool,
	// this way you will only have to submit the parameters
	void* (*handler)(void* params);

	// the threads of thread_pool, have to remember which connection they were working on
	// this hashmap is shared among the threads to access, their own file discriptors
	connection_mapper* connection_mapping;
};

#define DEFAULT_NO_CONNECTION_THREAD_DESTROY_TIMEOUT_IN_MICRO_SECONDS 3000 * 1000		// 3 seconds
// in the cached thread_pool if the thread does not receive a job in this time period, it will terminate itself
// this is not applicable for fixed thread pools

// this function will create a CACHED_THREAD_POOL_EXECUTOR executor, each of whose threads, are responsible for handling the connections individually
connection_thread_pool_manager* get_cached_connection_thread_pool_manager(unsigned long long int max_connection_count, void* (*handler)(void* params));

// this function will create a FIXED_THREAD_COUNT_EXECUTOR executor, each of whose threads, are responsible for handling the connections individually
connection_thread_pool_manager* get_fixed_connection_thread_pool_manager(unsigned long long int connection_count, void* (*handler)(void* params));

// you may not call this function, after calling close_all_connections_and_shutdown
// returns 1 if the job was submitted, else 0
int submit_job_parameters(connection_thread_pool_manager* manager, void* params);

// you must call this function before calling delete_connection_thread_pool_manager
void close_all_connections_and_wait_for_shutdown(connection_thread_pool_manager* manager);

// this function will only delete the resources occupied by the manager, 
// you must call void close_all_connections_and_wait_for_shutdown(connection_thread_pool_manager* manager), before deletion
void delete_connection_thread_pool_manager(connection_thread_pool_manager* manager);

#endif