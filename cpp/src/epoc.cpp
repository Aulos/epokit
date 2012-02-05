#include <mcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

#include "libepoc.hpp"

using namespace epokit;

#define KEY_SIZE 16 /* 128 bits == 16 bytes */

const unsigned char KEYS[3][KEY_SIZE]= { 
	{0x31,0x00,0x35,0x54,0x38,0x10,0x37,0x42,0x31,0x00,0x35,0x48,0x38,0x00,0x37,0x50},	// CONSUMER
    {0x31,0x00,0x39,0x54,0x38,0x10,0x37,0x42,0x31,0x00,0x39,0x48,0x38,0x00,0x37,0x50},	// RESEARCHER
	{0x31,0x00,0x35,0x48,0x31,0x00,0x35,0x54,0x38,0x10,0x37,0x42,0x38,0x00,0x37,0x50}	// SPECIAL
};

unsigned int get_level(unsigned char *frame, const unsigned char start_bit);

int Aes128Encoder::Aes128Encoder(HeadsetType type) {
    //libmcrypt initialization
    td_ = mcrypt_module_open(MCRYPT_RIJNDAEL_128, NULL, MCRYPT_ECB, NULL);
    mcrypt_generic_init( (MCRYPT)(td_), (void*)KEYS[type], KEY_SIZE, NULL);
}

int Aes128Encoder::~Aes128Encoder() {
    mcrypt_generic_deinit( (MCRYPT)(td_) );
    mcrypt_module_close( (MCRYPT)(td_) );
}

int Aes128Encoder::encode(unsigned char * raw_frame) {
    mdecrypt_generic ((MCRYPT)(td_), raw_frame, blockSize_);
}

template<int n, int i = 14>
struct get_level {
    enum {
        bit = (14 * n) + i - 1
    };

    enum {
        ix = (bit >> 3),
        shr = ((~bit) & 7)
    };
    static unsigned short get(unsigned char *frame) {
        return get_level<n, i-1>::get(frame) | (((frame[ix] >> shr) & 1) << (i - 1));
    }
};

template <int n>
struct get_level<n, 0> {
    static unsigned short get(unsigned char *) {
        return 0;
    }
};

template<int i>
struct fill_electrodes {
    static void fill(unsigned short * electrodes, unsigned char *frame) {
        electrodes[i-1] = get_level<i>::get(frame);
        fill_electrodes<i-1>::fill(electrodes, frame);
    }
};

template<>
struct fill_electrodes<0> {
    static void fill(unsigned short *, unsigned char *) {}
};

void Frame::Frame(unsigned char *buffer) {
    counter = buffer[0];

    fill_electrodes<16>::fill(electrode, buffer);

    //from qdots branch
    gyro.X = buffer[29] - 0x67;
    gyro.Y = buffer[30] - 0x67;
    //TODO!
    battery = 0;
}

// Helper function
unsigned int get_level(unsigned char *frame, const unsigned char start_bit) {
    unsigned char bit, stop_bit = 14 * (start_bit + 1);
	unsigned int level = 0;
	++frame;

	for (bit = 14 * start_bit; bit < stop_bit; ++bit)
		level |= (((frame[ bit >> 3 ] >> ((~bit) & 7)) & 1) << (stop_bit - bit));

	return (level >> 1) & 16383;
}

int UsbDevice::open(uint32_t vid, uint32_t pid, uint8_t device_index) {
	struct libusb_device **devs, *found = NULL;
	struct libusb_device_handle *dh;
	int i;
	
	if(libusb_init(&context_) < 0){
		return -1;
	}

	int num_devices = 0;
	if((num_devices = libusb_get_device_list(context_, &devs)) < 0) {
		return -1;
	}

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
	if(found == NULL || libusb_open(found, &device_) < 0) {
		libusb_exit(context_);
		return -1;
	}

	{
		int ret[2];
		if(libusb_kernel_driver_active(device_, 0)) {
			libusb_detach_kernel_driver(device_, 0);
		}
		ret[0] = libusb_claim_interface(device_, 0);

		if(libusb_kernel_driver_active(device_, 1)) {
			libusb_detach_kernel_driver(device_, 1);
		}
		ret[1] = libusb_claim_interface(device_, 1);

		if (ret[0] || ret[1]) {
			libusb_exit(context_);
			return -1;
		}
	}
}

void UsbDevice::close() {
    libusb_release_interface(device_, 0);
    libusb_release_interface(device_, 1);
	libusb_close(device_);
    libusb_exit(context_);
}

int UsbDevice::readData(uint8_t *data, int len, uint16_t endpoint) {
    int transferred;
    return ((!libusb_interrupt_transfer(device_, endpoint | LIBUSB_ENDPOINT_IN , data, len, &transferred, 100))
            ? transferred
            : 0);
}

int Device::getCount(uint32_t vid, uint32_t pid) {
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

