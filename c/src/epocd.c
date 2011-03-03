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
	FILE *output;
	enum headset_type type;

	char raw_frame[32];
	struct epoc_frame frame;
	int source_index = 2;
	int src_ind = 0, count_devs;

	if(argc > 1 && strcmp(argv[1], "research") == 0)
		type = RESEARCH_HEADSET;
	else if(argc > 1 && strcmp(argv[1], "consumer") == 0)
		type = CONSUMER_HEADSET;
	else{
		type = RESEARCH_HEADSET;
		--source_index;
	}

	if (argc > source_index )
		src_ind = atoi(argv[source_index]);
 
	output = stdout;
	if (argc > source_index + 1) {
		output = fopen(argv[source_index+1], "wb");
		if (output == NULL)
		{
			fputs("File write error: couldn't open the destination file for uncrypted data\n", stderr);
			return 1;
		}
	}
	  
	if((count_devs = epoc_get_count(EPOC_VID, EPOC_PID)) <= src_ind){
		fprintf(stderr, "Cannot find device with vid: %d; pid: %d\n", EPOC_VID, EPOC_PID);
		return 1;
	}
	printf("Found %d devices\n", count_devs);

	epoc_device *device = epoc_open(EPOC_VID, EPOC_PID, src_ind);
	if(device == NULL) {
		fprintf(stderr, "Cannot open the device %d\n", src_ind);
		return 1;
	}
	printf("Device %d opened...\n", src_ind);

	epoc_handler *eh = epoc_init(device, type);
	if(eh == NULL) {
		fprintf(stderr, "Cannot init the device!\n");
		epoc_close(device);
		return 1;
	}
	printf("Device %d inited...\nReading...", src_ind);
 
	while ( 1 ) {
		int i;
		epoc_get_next_frame(eh, &frame);
		fprintf(output, "%d, %d, %d, %d", frame.counter, frame.gyro.X, frame.gyro.Y, frame.electrode[0]);
		for(i=1; i < 16; ++i)
			  fprintf(output, ", %d", frame.electrode[i]);
		fprintf(output, "\n");
		fflush(output);
	}

	epoc_deinit(eh);
	epoc_close(device);
	return 0;
}
