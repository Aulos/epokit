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

enum headset_type {CONSUMER_HEADSET, RESEARCH_HEADSET};

struct epoc_contact_quality {
    char F3, FC6, P7, T8, F7, F8, T7, P8, AF4, F4, AF3, O2, O1, FC5;
};

struct epoc_frame {
    int F3, FC6, P7, T8, F7, F8, T7, P8, AF4, F4, AF3, O2, O1, FC5;
    struct epoc_contact_quality cq;
    char gyroX, gyroY;
    char battery;
};

int epoc_init(FILE *input, enum headset_type type);
int epoc_close(FILE * input);

int epoc_get_next_raw(FILE *input, char raw_frame[32]);
int epoc_get_next_frame(FILE *input, struct epoc_frame* frame);


#endif //LIBEPOC_H_
