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

namespace epokit {

const unsigned EPOC_VID = 4660;
const unsigned EPOC_PID = 60674;
const unsigned EPOC_IN_ENDPT_1	= 0x81;
const unsigned EPOC_IN_ENDPT_2	= 0x82;

const signed EPOC_DRIVER_ERROR = -1;

enum class HeadsetType: unsigned char {
	CONSUMER, 
	RESEARCH,
	SPECIAL
};

enum class Electrodes: unsigned char { 
	F3=0, FC5, AF3, F7, T7, P7, O1, 
	X1, X2, // 2 unknown 
	O2, P8, T8, F8, AF4, F4, FC6 
};

struct Frame {
    Frame(unsigned char *buffer);
    unsigned char counter;
    unsigned short electrode[16]; // indexed with electrodes enum
    struct {
        char electrode[16]; // indexed with electrodes enum
    } cq;
    struct {
        int X, Y;
    } gyro;
    char battery;
};

class AES128Encoder {
public:
    AES128Encoder(HeadsetType type);
    ~AES128Encoder();

    void encode(unsigned char * raw_frame);

protected:
    void *td_;
    static const unsigned char BlockSize = 32;
};

class UsbDevice {
public:
    int open(uint32_t vid = EPOC_VID, uint32_t pid = EPOC_PID, uint8_t device_index = 0);
    void close();
    int readData(uint8_t *data, int len, uint16_t endpoint);

public:
	static int getCount(uint32_t vid, uint32_t pid);

protected:
	struct libusb_context *context_;
	struct libusb_device_handle *device_;
};

template<class Device = UsbDevice, class Encoder = AES128Encoder>
class Handler: public Device, public Encoder {
public:
    Handler(HeadsetType type)
        : Encoder(type), buffer_(0) {}
    ~Handler() {
        Device::close();
    }

    Frame getNextFrame() {
        getNextRaw(buffer_, 2);
        return Frame(buffer_);
    }

private:
    int	getNextRaw(unsigned char *raw_frame, uint16_t endpoint = 1) {
        if ( Device::readData(raw_frame, Encoder::BlockSize, endpoint) != Encoder::BlockSize) {
            return -1;
        }
        Encoder::encode(raw_frame);
        return 0;
    }

private:
    unsigned char buffer_[Encoder::BlockSize];
};

}
#endif //LIBEPOC_H_
