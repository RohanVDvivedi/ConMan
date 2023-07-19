#include<file_descriptor_stream.h>

#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<limits.h>

#include<cutlery_math.h>

// must always be lesser than or equal to SSIZE_MAX
#define MAX_IO_SIZE (SSIZE_MAX >> 4)

static size_t read_from_fd(void* stream_context, void* data, size_t data_size, int* error)
{
	ssize_t ret = read(*((int*)stream_context), data, min(data_size, MAX_IO_SIZE));
	if(ret == -1)
	{
		*error = errno;
		return 0;
	}
	return ret;
}

static size_t write_to_fd(void* stream_context, const void* data, size_t data_size, int* error)
{
	ssize_t ret = write(*((int*)stream_context), data, min(data_size, MAX_IO_SIZE));
	if(ret == -1)
	{
		*error = errno;
		return 0;
	}
	return ret;
}

static void close_stream_context_fd(void* stream_context, int* error)
{
	if(close(*((int*)stream_context)) == -1)
		*error = errno;
}

static void destroy_stream_context_fd(void* stream_context)
{
	free(stream_context);
}

int initialize_stream_for_fd(stream* strm, int fd)
{
	int* stream_context = malloc(sizeof(int));
	if(stream_context == NULL)
		return 0;
	*stream_context = fd;
	initialize_stream(strm, stream_context, read_from_fd, write_to_fd, close_stream_context_fd, destroy_stream_context_fd, NULL, DEFAULT_MAX_UNFLUSHED_BYTES_COUNT);
	return 1;
}