#include<stdio.h>
#include<stdlib.h>

#include<writable_dstring_stream.h>

#include<dstring.h>
#include<string.h>

char* input[10] = {
	"Hello, Rohan !!\n",
	"How are you?\n",
	"How are you doing?\n",
	"Get well soon, son.\n",
	"Thank you so much, Ms. Rupa.\n",
	"You are always welcomed here.\n",
	"I know that Ms. Rupa.\n",
	"I am so glad that you are here now.\n",
	"Yeah, its now or never.\n",
	"Praise the lord.\n"
};

int main()
{
	stream strm;
	initialize_writable_dstring_stream(&strm);

	int error;

	for(int i = 0; i < sizeof(input)/sizeof(input[0]); i++)
		write_to_stream(&strm, input[i], strlen(input[i]), &error);

	printf(printf_dstring_format, printf_dstring_params(strm.stream_context));

	close_stream(&strm, &error);

	deinitialize_stream(&strm);

	return 0;
}