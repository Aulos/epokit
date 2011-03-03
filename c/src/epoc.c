#include <mcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

#include "libepoc.h"

#define KEY_SIZE 16 /* 128 bits == 16 bytes */

const unsigned char KEYS[3][KEY_SIZE]= { 
	{0x31,0x00,0x35,0x54,0x38,0x10,0x37,0x42,0x31,0x00,0x35,0x48,0x38,0x00,0x37,0x50},	// CONSUMER
	{0x31,0x00,0x39,0x54,0x38,0x10,0x37,0x42,0x31,0x00,0x39,0x48,0x38,0x00,0x37,0x50},	// RESEARCHER
	{0x31,0x00,0x35,0x48,0x31,0x00,0x35,0x54,0x38,0x10,0x37,0x42,0x38,0x00,0x37,0x50}	// SPECIAL
};

unsigned int get_level(unsigned char *frame, const unsigned char start_bit);

epoc_handler *epoc_init(epoc_device* device, enum headset_type type) {
	epoc_handler * handler = (epoc_handler *)malloc( sizeof(epoc_handler) );

	handler->device_handler = device;
	//libmcrypt initialization
	handler->td = mcrypt_module_open(MCRYPT_RIJNDAEL_128, NULL, MCRYPT_ECB, NULL);
	handler->block_size = mcrypt_enc_get_block_size( (MCRYPT)(handler->td) );
	handler->buffer = malloc(2 * handler->block_size); 
	mcrypt_generic_init( (MCRYPT)(handler->td), (void*)KEYS[type], KEY_SIZE, NULL);
	return handler;
}

int epoc_deinit(epoc_handler *eh) {
	mcrypt_generic_deinit( (MCRYPT)(eh->td) );
	mcrypt_module_close( (MCRYPT)(eh->td) );

	free(eh->buffer);
	free(eh);

	return 0;
}
int epoc_get_next_raw(epoc_handler *eh, unsigned char *raw_frame) {
	//Two blocks of 16 bytes must be read.
	int transf = 0;;
	if ( epoc_read_data(eh->device_handler, raw_frame, 2 * eh->block_size, &transf) != 0 || transf != 2 * eh->block_size)
		return -1;

	mdecrypt_generic ((MCRYPT)(eh->td), raw_frame, 2 * eh->block_size);

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
unsigned int get_level(unsigned char *frame, const unsigned char start_bit) {
	unsigned char bit, stop_bit = 14 * start_bit + 14;
	unsigned int level = 0;
	++frame;

	for (bit = 14 * start_bit; bit < stop_bit; ++bit)
		level |= (((frame[ bit >> 3 ] >> ((~bit) & 7)) & 1) << (stop_bit - bit));

	return (level >> 1) & 16383;
}

int	epoc_get_count(uint32_t vid, uint32_t pid) {
	struct libusb_device **devs;
	struct libusb_context *ctx;
	size_t i;
	int count = 0;

	if(libusb_init(&ctx) < 0)
		return EPOC_DRIVER_ERROR;

	if(libusb_get_device_list(ctx, &devs) < 0)
		return EPOC_DRIVER_ERROR;

	for (i = 0; devs[i] != NULL; ++i) {
		struct libusb_device_descriptor desc;
		if(libusb_get_device_descriptor(devs[i], &desc) < 0)
			break;
		if(desc.idVendor == vid && desc.idProduct == pid)
			++count;
	}

	libusb_free_device_list(devs, 1);
	libusb_exit(ctx);
	return count;
}

epoc_device *epoc_open(uint32_t vid, uint32_t pid, uint8_t device_index) {
	epoc_device *handler = (epoc_device*)malloc(sizeof(epoc_device));

	struct libusb_device **devs, *found = NULL;
	struct libusb_device_handle *dh;
	int i;
	
	memset(handler, 0, sizeof(epoc_device));

	if(libusb_init(&handler->context) < 0){
		free(handler);
		return NULL;
	}

	int num_devices = 0;
	if((num_devices = libusb_get_device_list(handler->context, &devs)) < 0)
		return NULL;

	for (i = 0; i < num_devices; ++i) {
		struct libusb_device_descriptor desc;
		if(libusb_get_device_descriptor(devs[i], &desc) < 0)
			break;
		if(found == NULL && desc.idVendor == vid && desc.idProduct == pid && 0 == device_index--)
			found = devs[i];
		else
			libusb_unref_device(devs[i]);
	}

	libusb_free_device_list(devs, 0);
	if(found == NULL || libusb_open(found, &handler->device) < 0) {
		libusb_exit(handler->context);
		free(handler);
		return NULL;
	}

	{
		int ret[2];
		if(libusb_kernel_driver_active(handler->device, 0))
			libusb_detach_kernel_driver(handler->device, 0);
		ret[0] = libusb_claim_interface(handler->device, 0);

		if(libusb_kernel_driver_active(handler->device, 1))
			libusb_detach_kernel_driver(handler->device, 1);
		ret[1] = libusb_claim_interface(handler->device, 1);

		if (ret[0] || ret[1]) {
			libusb_exit(handler->context);
			free(handler);
			return NULL;
		}
	}

	return handler;
}

int epoc_close(epoc_device *h) {
	int ret[2];
	ret[0] = libusb_release_interface(h->device, 0);
	ret[1] = libusb_release_interface(h->device, 1);
	if( ret[0] < 0 || ret[1] < 0 )
		return -1;
	libusb_close(h->device);
	libusb_exit(h->context);
	free(h);
	return 0;
}

int epoc_read_data(epoc_device *d, uint8_t *data, int len, int *transferred) {
	return libusb_interrupt_transfer(d->device, 2 | LIBUSB_ENDPOINT_IN , data, len, transferred, 100);
}

