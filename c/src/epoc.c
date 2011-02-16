#include <mcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libepoc.h"

#define KEY_SIZE 16 /* 128 bits == 16 bytes */

const unsigned char KEYS[3][KEY_SIZE]= { 
	{0x31,0x00,0x35,0x54,0x38,0x10,0x37,0x42,0x31,0x00,0x35,0x48,0x38,0x00,0x37,0x50},	// CONSUMER
	{0x31,0x00,0x39,0x54,0x38,0x10,0x37,0x42,0x31,0x00,0x39,0x48,0x38,0x00,0x37,0x50},	// RESEARCHER
	{0x31,0x00,0x35,0x48,0x31,0x00,0x35,0x54,0x38,0x10,0x37,0x42,0x38,0x00,0x37,0x50}	// SPECIAL
};

int get_level(unsigned char *frame, const unsigned char start_bit);

epoc_handler *epoc_init(FILE* source, enum headset_type type) {
	epoc_handler * handler = (epoc_handler *)malloc( sizeof(epoc_handler) );

	handler->file_handler = source;
	//libmcrypt initialization
	handler->td = mcrypt_module_open(MCRYPT_RIJNDAEL_128, NULL, MCRYPT_ECB, NULL);
	handler->block_size = mcrypt_enc_get_block_size( (MCRYPT)(handler->td) ); //should return a 16bits blocksize
	handler->buffer = malloc(2 * handler->block_size); 
	mcrypt_generic_init( (MCRYPT)(handler->td), (void*)KEYS[type], KEY_SIZE, NULL);
	return handler;
}

int epoc_close(epoc_handler *eh) {
	mcrypt_generic_deinit( (MCRYPT)(eh->td) );
	mcrypt_module_close( (MCRYPT)(eh->td) );

	free(eh->buffer);
	fclose(eh->file_handler);
	free(eh);

	return 0;
}
int epoc_get_next_raw(epoc_handler *eh, unsigned char *raw_frame) {
	//Two blocks of 16 bytes must be read.
	if (fread (raw_frame, 1, 2 * eh->block_size, eh->file_handler) != 2 * eh->block_size)
		return -1;

	mdecrypt_generic ((MCRYPT)(eh->td), raw_frame, 2 * eh->block_size);
	// Above line could be wrong and you could try to split it into two:      
	// mdecrypt_generic ((MCRYPT)(eh->td), raw_frame, eh->block_size);
	// mdecrypt_generic ((MCRYPT)(eh->td), raw_frame + eh->block_size, eh->block_size);

	return 0;
}

int epoc_get_next_frame(epoc_handler *eh, struct epoc_frame* frame) {
	int i;
	epoc_get_next_raw(eh, eh->buffer);

	frame->counter = eh->buffer[0];

	for(i=0; i < 16; ++i)
		frame->electrode[i] = get_level(eh->buffer, i);
	
	//from qdots branch
	frame->gyro.X = eh->buffer[29] - 0x66;
	frame->gyro.Y = eh->buffer[30] - 0x68;
	//TODO!
	frame->battery = 0;

	return 0;
}

// Helper function
int get_level(unsigned char *frame, const unsigned char start_bit) {
	unsigned char bit, stop_bit = 14 * start_bit + 14;
	int level = 0;
	++frame;

	for (bit = 14 * start_bit; bit < stop_bit; ++bit)
		level |= (((frame[ bit >> 3 ] >> ((~bit) & 7)) & 1) << (stop_bit - bit));

	return level;
}

