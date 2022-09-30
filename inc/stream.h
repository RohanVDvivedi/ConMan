#ifndef STREAM_H
#define STREAM_H

#include<dpipe.h>

typedef struct stream stream;
struct stream
{
	void* stream_context;

	// this pipe stores data that was unread_to_stream
	dpipe unread_data;

	// returns bytes read from data, (atmost data_size number of bytes will be touched)
	// on error return 0 bytes read and set the value of (non-zero) error
	unsigned int (*read_from_stream_context)(void* stream_context, void* data, unsigned int data_size, int* error);

	// returns bytes written from data (that consists of data_size number of bytes to be written)
	// on error return 0 bytes read and set the value of (non-zero) error
	unsigned int (*write_to_stream_context)(void* stream_context, const void* data, unsigned int data_size, int* error);

	// closes the stream context
	void (*close_stream_context)(void* stream_context, int* error);

	// this function will be called to destroy the stream_context
	void (*destroy_stream_context)(void* stream_context);

	int error;
};

void initialize_stream(
						stream* strm, 
						void* stream_context,
						unsigned int (*read_from_stream_context)(void* stream_context, void* data, unsigned int data_size, int* error),
						unsigned int (*write_to_stream_context)(void* stream_context, const void* data, unsigned int data_size, int* error),
						void (*close_stream_context)(void* stream_context, int* error),
						void (*destroy_stream_context)(void* stream_context)
					);

int is_readable_stream(stream* strm);

int is_writable_stream(stream* strm);

int get_error_stream(stream* strm);

unsigned int read_from_stream(stream* rs, void* data, unsigned int data_size);

// returns 1 on success else a 0
int unread_from_stream(stream* rs, const void* data, unsigned int data_size);

unsigned int write_to_stream(stream* ws, const void* data, unsigned int data_size);

void close_stream(stream* strm);

void deinitialize_stream(stream* strm);

#endif