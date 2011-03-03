/* Emotic EPOC daemon that decrypt stream using ECB and RIJNDAEL-128 cipher
 * (well, not yet a daemon...)
 * 
 * Usage: epocd (consumer/research) /dev/emotiv/encrypted output_file
 * 
 * Make sure to pick the right type of device, as this determins the key
 * */

#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "libepoc.h"
   

int main(int argc, char **argv)
{
	FILE *output = stdout;
	enum headset_type type = RESEARCH_HEADSET;

	char raw_frame[32];
	struct epoc_frame frame;
	int source_index = 0, 
		count_devs, 
		c;

	opterr = 0;

	while ((c = getopt (argc, argv, "t:n:o:")) != -1)
		switch (c) {
		case 't':
			if(optarg[0] == 'r')
				type = RESEARCH_HEADSET;
			else if(optarg[0] == 'c')
				type = CONSUMER_HEADSET;
			break;
		case 'n':
			source_index = atoi(optarg);
			break;
		case 'o':
			output = fopen(optarg, "wb");
			if (output == NULL) {
				fputs("File write error: couldn't open the destination file for uncrypted data\n", stderr);
				return 1;
			}
			break;
		default:
			fputs("Bad arguments\n", stderr);
			return 1;
		}
	  
	if((count_devs = epoc_get_count(EPOC_VID, EPOC_PID)) <= source_index){
		fprintf(stderr, "Cannot find device with vid: %d; pid: %d\n", EPOC_VID, EPOC_PID);
		return 1;
	}
	printf("Found %d devices\n", count_devs);

	epoc_device *device = epoc_open(EPOC_VID, EPOC_PID, source_index);
	if(device == NULL) {
		fprintf(stderr, "Cannot open the device %d\n", source_index);
		return 1;
	}
	printf("Device %d opened...\n", source_index);

	epoc_handler *eh = epoc_init(device, type);
	if(eh == NULL) {
		fprintf(stderr, "Cannot init the device!\n");
		epoc_close(device);
		return 1;
	}
	printf("Device %d inited...\nReading...\n", source_index);
 
	while ( 1 ) {
		int i;
		epoc_get_next_frame(eh, &frame);
		fprintf(output, "%d %d %d", frame.counter, frame.gyro.X, frame.gyro.Y);
		for(i=0; i < 16; ++i)
			  fprintf(output, " %d", frame.electrode[i]);
		fprintf(output, "\n");
		fflush(output);
	}

	epoc_deinit(eh);
	epoc_close(device);
	return 0;
}
