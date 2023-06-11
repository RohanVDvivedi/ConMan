#include<stacked_stream_util.h>

#include<stdlib.h>

void close_deinitialize_free_all_from_stacked_stream(stacked_stream* sstrm, int operate_on)
{
	while(!is_empty_stacked_stream(sstrm, operate_on))
	{
		stream* strm = get_top_of_stacked_stream(sstrm, operate_on);
		pop_from_stacked_stream(sstrm, operate_on);
		int error = 0;
		close_stream(strm, &error);
		deinitialize_stream(strm);
		free(strm);
	}
}