#include<stream.h>

#include<stddef.h>
#include<string.h>

void initialize_stream(stream* strm, 
						void* stream_context,
						unsigned int (*read_from_stream_context)(void* stream_context, void* data, unsigned int data_size, int* error),
						unsigned int (*write_to_stream_context)(void* stream_context, const void* data, unsigned int data_size, int* error),
						void (*close_stream_context)(void* stream_context, int* error),
						void (*destroy_stream_context)(void* stream_context),
						void (*post_flush_callback_stream_context)(void* stream_context, int* error),
						unsigned int max_unflushed_bytes_count)
{
	strm->stream_context = stream_context;
	initialize_dpipe(&(strm->unread_data), 0);
	initialize_dpipe(&(strm->unflushed_data), 0);
	strm->max_unflushed_bytes_count = max_unflushed_bytes_count;
	strm->read_from_stream_context = read_from_stream_context;
	strm->end_of_stream_received = 0;
	strm->write_to_stream_context = write_to_stream_context;
	strm->close_stream_context = close_stream_context;
	strm->destroy_stream_context = destroy_stream_context;
	strm->post_flush_callback_stream_context = post_flush_callback_stream_context;
	strm->last_error = 0;
}

int is_readable_stream(stream* strm)
{
	return strm->read_from_stream_context != NULL;
}

int is_writable_stream(stream* strm)
{
	return strm->write_to_stream_context != NULL;
}

static unsigned int min(unsigned int a, unsigned int b)
{
	return (a < b) ? a : b;
}

unsigned int read_from_stream(stream* strm, void* data, unsigned int data_size, int* error)
{
	// intialize error to 0
	(*error) = 0;

	if(strm->read_from_stream_context == NULL)
		return 0;

	unsigned int bytes_read = 0;

	// if the unread_data pipe has remaining bytes then read data from there
	if(!is_empty_dpipe(&(strm->unread_data)))
	{
		bytes_read = read_from_dpipe(&(strm->unread_data), data, data_size, PARTIAL_ALLOWED);

		// shrink unread_data dpipe if it is larger than 4 times what it is required
		if(get_capacity_dpipe(&(strm->unread_data)) >= 4 * get_bytes_readable_in_dpipe(&(strm->unread_data)))
			resize_dpipe(&(strm->unread_data), get_bytes_readable_in_dpipe(&(strm->unread_data)));
	}
	// else make a read call to the stream context
	else if(data_size >= 128 && !strm->end_of_stream_received)
	{
		bytes_read = strm->read_from_stream_context(strm->stream_context, data + bytes_read, data_size - bytes_read, error);

		// if it is EOF, then set the end_of_stream_received flag
		if(bytes_read == 0 && (!(*error)))
			strm->end_of_stream_received = 1;
	}
	// if data_size to be read is lesser than 128 bytes then, we attempt to make a read for a kilo byte from the stream context and then cache remaining bytes
	else if(!strm->end_of_stream_received)
	{
		// read a kilo byte worth of data, from the stream context and cache it
		char data_cache_read[1024];
		unsigned int data_cache_read_capacity = 1024;
		unsigned int data_cache_read_size = strm->read_from_stream_context(strm->stream_context, data_cache_read, data_cache_read_capacity, error);

		// if it is EOF, then set the end_of_stream_received flag
		if(data_cache_read_size == 0 && (!(*error)))
			strm->end_of_stream_received = 1;

		// move the front of cached bytes to output buffer
		// these many bytes from the cached data can be moved to the output buffer
		bytes_read = min(data_size, data_cache_read_size);
		memmove(data, data_cache_read, bytes_read);

		// if there are still bytes left in the cached buffer, then move those bytes to the unread buffer
		if(data_cache_read_size > bytes_read)
		{
			// write the remaining bytes from data_cache_read to unread_data
			void* cache_bytes_to_write = data_cache_read + bytes_read;
			unsigned int cache_bytes_to_write_size = data_cache_read_size - bytes_read;

			unsigned int bytes_written_from_cache_buffer = write_to_dpipe(&(strm->unread_data), cache_bytes_to_write, cache_bytes_to_write_size, ALL_OR_NONE);

			if(bytes_written_from_cache_buffer == 0)
			{
				resize_dpipe(&(strm->unread_data), get_capacity_dpipe(&(strm->unread_data)) + cache_bytes_to_write_size + 1024);
				bytes_written_from_cache_buffer = write_to_dpipe(&(strm->unread_data), cache_bytes_to_write, cache_bytes_to_write_size, ALL_OR_NONE);
			}
		}
	}

	// if an error occurred, then register it as the last one
	if(*error)
		strm->last_error = (*error);

	return bytes_read;
}

int unread_from_stream(stream* strm, const void* data, unsigned int data_size)
{
	if(strm->read_from_stream_context == NULL)
		return 0;

	// just push the data to unread_data pipe
	unsigned int bytes_unread = unread_to_dpipe(&(strm->unread_data), data, data_size, ALL_OR_NONE);

	if(bytes_unread == 0)
	{
		resize_dpipe(&(strm->unread_data), get_capacity_dpipe(&(strm->unread_data)) + data_size + 1024);
		bytes_unread = unread_to_dpipe(&(strm->unread_data), data, data_size, ALL_OR_NONE);
	}

	return bytes_unread == data_size;
}

// INTERNAL FUNCTION ONLY - to be only used by write and flush_all functions
// this function will be used to write unflushed bytes, i.e. to flush them, i.e. to perfrom actual write to stream context
// return value suggests the number of bytes flushed, regardless of the error
static unsigned int write_flushable_bytes(stream* strm, const void* data, unsigned int data_size, int* error)
{
	// return value
	unsigned int bytes_written = 0;
	
	// clear error
	(*error) = 0;

	while(bytes_written < data_size && (*error) == 0)
		bytes_written += strm->write_to_stream_context(strm->stream_context, data + bytes_written, data_size - bytes_written, error);

	return bytes_written;
}

// INTERNAL FUNCTION ONLY - to be only used by write and flush_all functions
// it only flushes all unflushed bytes from stream unflushed_data pipe of the stream
// it may fail on an error, in which case it will return 0 with (*error) being set to respective value
static void flush_all_unflushed_data(stream* strm, int* error)
{
	(*error) = 0;

	while(get_bytes_readable_in_dpipe(&(strm->unflushed_data)) > 0 && (*error) == 0)
	{
		cy_uint data_size;
		const void* data = peek_max_consecutive_from_dpipe(&(strm->unflushed_data), &data_size);

		unsigned bytes_flushed = write_flushable_bytes(strm, data, data_size, error);

		discard_from_dpipe(&(strm->unflushed_data), bytes_flushed);
	}
}

unsigned int write_to_stream(stream* strm, const void* data, unsigned int data_size, int* error)
{
	if(strm->write_to_stream_context == NULL)
		return 0;

	(*error) = 0;

	// if the total unflushed_data_bytes count is lesser than max_unflushed_bytes_count, then just push these new data to unflushed_data pipe
	if(data_size + get_bytes_readable_in_dpipe(&(strm->unflushed_data)) <= strm->max_unflushed_bytes_count)
	{
		unsigned int bytes_written = write_to_dpipe(&(strm->unflushed_data), data, data_size, ALL_OR_NONE);

		if(bytes_written == 0)
		{
			unsigned int additional_space_requirement = data_size - get_bytes_writable_in_dpipe(&(strm->unflushed_data));
			resize_dpipe(&(strm->unflushed_data), min(get_capacity_dpipe(&(strm->unflushed_data)) + additional_space_requirement * 2, strm->max_unflushed_bytes_count));
			bytes_written = write_to_dpipe(&(strm->unflushed_data), data, data_size, ALL_OR_NONE);
		}

		return bytes_written;
	}
	else // we fallback to flush not just the unflushed_data, but also the new data that has arrived
	{
		// flush the unflushed_data first
		flush_all_unflushed_data(strm, error);

		if(*error)
		{
			strm->last_error = (*error);
			return 0; // no bytes from data written, hence we write a 0 here
		}

		// then flush the new arriving data
		unsigned int bytes_written = write_flushable_bytes(strm, data, data_size, error);

		if(*error)
		{
			strm->last_error = (*error);
			return bytes_written;
		}

		// no error, only then we may call post_flush_callback_stream_context
		if(strm->post_flush_callback_stream_context != NULL)
		{
			strm->post_flush_callback_stream_context(strm->stream_context, error);
			if(*error)
				strm->last_error = (*error);
		}

		return bytes_written;
	}
}

void flush_all_from_stream(stream* strm, int* error)
{
	// intialize error to 0
	(*error) = 0;

	// flush all of the unflushed_data
	flush_all_unflushed_data(strm, error);

	resize_dpipe(&(strm->unflushed_data), get_bytes_readable_in_dpipe(&(strm->unflushed_data)));

	if(*error)
	{
		strm->last_error = (*error);
		return;
	}

	// no error, only then we may call post_flush_callback_stream_context
	if(strm->post_flush_callback_stream_context != NULL)
	{
		strm->post_flush_callback_stream_context(strm->stream_context, error);
		if(*error)
			strm->last_error = (*error);
	}
}

void close_stream(stream* strm, int* error)
{
	// intialize error to 0
	(*error) = 0;

	strm->close_stream_context(strm->stream_context, error);

	// if an error occurred, then register it as the last one
	if(*error)
		strm->last_error = (*error);
}

void deinitialize_stream(stream* strm)
{
	deinitialize_dpipe(&(strm->unread_data));
	deinitialize_dpipe(&(strm->unflushed_data));
	strm->destroy_stream_context(strm->stream_context);
	*strm = (stream){};
}