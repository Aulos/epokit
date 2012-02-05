/* Emotic EPOC daemon that decrypt stream using ECB and RIJNDAEL-128 cipher
 * (well, not yet a daemon...)
 * 
 * Usage: epocd (consumer/research) /dev/emotiv/encrypted output_file
 * 
 * Make sure to pick the right type of device, as this determins the key
 * */

#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <getopt.h>

#include "epokit.hpp"
   
using namespace epokit;
using namespace std;

int main(int argc, char **argv)
{
	FILE *output = stdout;
    HeadsetType type = HeadsetType::RESEARCH;

	int source_index = 0, 
		count_devs, 
		c;

	opterr = 0;

	while ((c = getopt (argc, argv, "t:n:o:")) != -1)
		switch (c) {
		case 't':
			if(optarg[0] == 'r')
                type = HeadsetType::RESEARCH;
			else if(optarg[0] == 'c')
                type = HeadsetType::CONSUMER;
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
	  
    if((count_devs = UsbDevice::getCount(EPOC_VID, EPOC_PID)) <= source_index){
		fprintf(stderr, "Cannot find device with vid: %d; pid: %d\n", EPOC_VID, EPOC_PID);
		return 1;
	}
	printf("Found %d devices\n", count_devs);

    Handler<> eh(type);
    if(eh.open(EPOC_VID, EPOC_PID, source_index) != 0) {
		fprintf(stderr, "Cannot open the device %d\n", source_index);
		return 1;
	}
	printf("Device %d opened...\n", source_index);

	while ( 1 ) {
		int i;
        Frame frame = eh.getNextFrame();
		fprintf(output, "%d %d %d", frame.counter, frame.gyro.X, frame.gyro.Y);
		for(i=0; i < 16; ++i)
			  fprintf(output, " %d", frame.electrode[i]);
		fprintf(output, "\n");
		fflush(output);
	}
	return 0;
}
