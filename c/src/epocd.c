/* Emotic EPOC daemon that decrypt stream using ECB and RIJNDAEL-128 cipher
 * (well, not yet a daemon...)
 * 
 * Usage: epocd (consumer/research) /dev/emotiv/encrypted output_file
 * 
 * Make sure to pick the right type of device, as this determins the key
 * */

#include <stdio.h>
#include <string.h>

#include "libepoc.h"
   

int main(int argc, char **argv)
{
	FILE *input;
	FILE *output;
	enum headset_type type;

	char raw_frame[32];
	struct epoc_frame frame;
	int source_index = 2;
  
	if (argc < 2)
	{
		fputs("Missing argument\nExpected: epoc [consumer|research] source [dest]\n", stderr);
		fputs("By default, dest = stdout\n", stderr);
		return 1;
	}

	if(strcmp(argv[1], "research") == 0)
		type = RESEARCH_HEADSET;
	else if(strcmp(argv[1], "consumer") == 0)
		type = CONSUMER_HEADSET;
	else{
		type = RESEARCH_HEADSET;
		--source_index;
	}

	input = fopen(argv[source_index], "rb");
	if (input == NULL)
	{
		fputs("File read error: couldn't open the EEG source!", stderr);
		return 1;
	}
  
	epoc_handler * eh = epoc_init(input, type);
  
	if (argc <= source_index) {
		output = stdout;
	} else {
		output = fopen(argv[source_index+1], "wb");
		if (output == NULL)
		{
			fputs("File write error: couldn't open the destination file for uncrypted data", stderr);
			return 1;
		}
	}

	while ( 1 ) {
		int i;
		epoc_get_next_frame(eh, &frame);
		fprintf(output, "%d, %d, %d, %d", frame.counter, frame.gyro.X, frame.gyro.Y, frame.electrode[0]);
		for(i=1; i < 16; ++i)
			  fprintf(output, ", %d", frame.electrode[i]);
		fprintf(output, "\n");
		fflush(output);
	}

	epoc_close(eh);
	return 0;
}
