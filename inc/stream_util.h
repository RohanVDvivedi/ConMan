#ifndef STREAM_UTIL_H
#define STREAM_UTIL_H

#include<stream.h>

#include<dstring.h>

#include<stdint.h>

unsigned int write_to_stream_formatted(stream* ws, const char* cstr_format, int* error, ...);

unsigned int read_uint64_from_stream(stream* rs, uint64_t* data, int* error);

unsigned int skip_whitespaces_from_stream(stream* rs, unsigned int max_whitespaces_to_skip, int* error);

// if return is non zero then an instance of str_to_skip was encountered
unsigned int skip_dstring_from_stream(stream* rs, const dstring* str_to_skip, int* error);

// bytes will be read until the until_str dstring is encountered in the stream
// all bytes read from the stream will be retunred in the return dstring
// on success (if until dstring is encountered) the returned dstring will also contain until_str as its suffix
// on failure (if max_bytes_to_read is encountered OR the stream is closed) then an empty dstring will be returned
// in any case, no more than max_bytes_to_read bytes will be read
dstring read_dstring_until_from_stream(stream* rs, const dstring* until, unsigned int max_bytes_to_read, int* error);

#endif