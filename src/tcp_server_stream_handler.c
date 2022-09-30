#include<tcp_server_stream_handler.h>

#include<file_descriptor_stream.h>
#include<stream_handler_wrapper_params.h>

#include<executor.h>

#include<netinet/in.h>
#include<unistd.h>
#include<errno.h>

static void* stream_handler_wrapper(void* stream_handler_wrapper_input_params_v_p)
{
	stream_handler_wrapper_input_params* handler_data = ((stream_handler_wrapper_input_params*)stream_handler_wrapper_input_params_v_p);

	stream strm;
	
	int streams_initialized = 0;

	if(handler_data->ssl_ctx == NULL)
	{
		initialize_stream_for_fd(&strm, handler_data->fd);
		streams_initialized = 1;
	}
	else // SSL_accept the connection 
		streams_initialized = initialize_stream_for_ssl_server(&strm, handler_data->ssl_ctx, handler_data->fd);

	// only if the streams were successfully initialized
	// then we may call the handler
	if(streams_initialized)
		handler_data->stream_handler(&strm, handler_data->additional_params);

	// phase 5
	// close the stream, and the underlying filedescriptor
	close_stream(&strm);

	// deinitializing stream
	deinitialize_stream(&strm);

	free(handler_data);
	return NULL;
}

int tcp_server_stream_handler(int listen_fd, void* additional_params, void (*stream_handler)(stream* strm, void* additional_params), unsigned int thread_count, SSL_CTX* ssl_ctx)
{
	// phase 3
	// listenning on the socket file discriptor 
	int err = listen(listen_fd, DEFAULT_BACKLOG_QUEUE_SIZE);
	if(err == -1)
		return err;

	// start accepting in loop
	struct sockaddr_in client_addr;		socklen_t client_len = sizeof(client_addr);
	executor* connection_executor = new_executor(CACHED_THREAD_POOL_EXECUTOR, thread_count, thread_count * 8, DEFAULT_NO_CONNECTION_THREAD_DESTROY_TIMEOUT_IN_MICRO_SECONDS, NULL, NULL, NULL);
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
		// here wait for 10 milliseconds to timeout job submission
		submit_job(connection_executor, stream_handler_wrapper, new_stream_handler_wrapper_input_params(conn_fd, ssl_ctx, additional_params, stream_handler), NULL, 10 * 1000);
	}

	shutdown_executor(connection_executor, 1);

	wait_for_all_threads_to_complete(connection_executor);

	delete_executor(connection_executor);

	return 0;
}