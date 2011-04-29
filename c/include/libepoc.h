/* Copyright (c) 2010, Daeken and Skadge
 * Modified by Aulos
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef LIBEPOC_H_
#define LIBEPOC_H_

#include <stdint.h>

#define EPOC_VID		4660
#define EPOC_PID		60674
#define EPOC_IN_ENDPT_1	0x81
#define EPOC_IN_ENDPT_2	0x82

#define EPOC_DRIVER_ERROR -1;

#ifdef __cplusplus
extern "C" {
#endif

enum headset_type {CONSUMER_HEADSET, RESEARCH_HEADSET, SPECIAL_HEADSET};
enum electrodes { 
	F3=0, FC5, AF3, F7, T7, P7, O1, 
	X1, X2, // 2 unknown 
	O2, P8, T8, F8, AF4, F4, FC6 
};

typedef struct {
	struct libusb_context *context;
	struct libusb_device_handle *device;
	struct libusb_transfer *in_transfer;
	struct libusb_transfer *out_transfer;
} epoc_device;

typedef struct {
	epoc_device *device_handler;
	unsigned char block_size;
	void *td;
	unsigned char *buffer;
} epoc_handler;

struct epoc_contact_quality {
	char electrode[16]; // indexed with electrodes enum 
};

struct epoc_gyro {
	int X, Y;
};

struct epoc_frame {
	unsigned char counter; 
	unsigned short electrode[16]; // indexed with electrodes enum
	struct epoc_contact_quality cq;
	struct epoc_gyro gyro;
	char battery;
};

epoc_handler*	epoc_init(epoc_device *device, enum headset_type type);
int				epoc_deinit(epoc_handler *eh);
int	epoc_get_next_raw	(epoc_handler *eh, unsigned char *raw_frame, uint16_t endpoint);
int epoc_get_next_frame	(epoc_handler *eh, struct epoc_frame* frame);
int epoc_get_frame_from_buffer(struct epoc_frame *frame, uint8_t *buffer);

int	epoc_get_count(uint32_t vid, uint32_t pid);
epoc_device *epoc_open(uint32_t vid, uint32_t pid, uint8_t device_index);
int epoc_close(epoc_device *d);

typedef void (*epoc_async_cb)(epoc_handler *, uint8_t *, void *);
int epoc_read_data(epoc_device *d, uint8_t *data, int len, int * transferred, uint16_t endpoint);
int epoc_read_data_async(epoc_handler *eh, uint8_t *data, int len, uint16_t endpoint, epoc_async_cb callback, void *user_data);

struct epoc_pollfd
{
	int fd;
	short events;
};

struct epoc_pollfd **epoc_get_pollfds(epoc_device *d);

int epoc_acquire_lock(epoc_device *d);
int epoc_release_lock(epoc_device *d);

int epoc_handle_events(epoc_device *d);

#ifdef __cplusplus
}
#endif

#endif //LIBEPOC_H_
