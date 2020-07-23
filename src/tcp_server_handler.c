#include<tcp_server_handler.h>

#include<handler_wrapper_params.h>

static void* handler_wrapper(void* handler_wrapper_input_params_v_p)
{
	handler_wrapper_input_params* handler_data = ((handler_wrapper_input_params*)handler_wrapper_input_params_v_p);

	handler_data->handler(handler_data->fd, handler_data->additional_params);

	// phase 5
	// closing client socket
	close(handler_data->fd);

	free(handler_data);
	return NULL;
}

int tcp_server_handler(int listen_fd, void* additional_params, void (*handler)(int conn_fd, void* additional_params), unsigned int thread_count)
{
	// phase 3
	// listenning on the socket file discriptor 
	int err = listen(listen_fd, DEFAULT_BACKLOG_QUEUE_SIZE);
	if(err == -1)
		return err;

	// start accepting in loop
	struct sockaddr_in client_addr;		socklen_t client_len = sizeof(client_addr);
	executor* connection_executor = get_executor(CACHED_THREAD_POOL_EXECUTOR, thread_count, DEFAULT_NO_CONNECTION_THREAD_DESTROY_TIMEOUT_IN_MICRO_SECONDS, NULL, NULL, NULL);
	while(1)
	{
		// phase 4
		// accept uses backlog queue connection and de-queues them 
		err = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
		if(err == -1)
		{
			// break the listenning loop, if the listen_fd file discriptor is closed
			if(errno == EBADF || errno == ECONNABORTED || errno == EINVAL || errno == ENOTSOCK || errno == EPERM)
				break;
		}
		int conn_fd = err;

		// serve the connection that has been accepted, submit it to executor, to assign a thread to it
		submit_function(connection_executor, handler_wrapper, get_new_handler_wrapper_input_params(conn_fd, additional_params, handler));
	}

	shutdown_executor(connection_executor, 1);

	wait_for_all_threads_to_complete(connection_executor);

	delete_executor(connection_executor);

	return 0;
}
