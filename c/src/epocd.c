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
  
  if (argc < 3)
  {
    fputs("Missing argument\nExpected: epoc [consumer|research] source [dest]\n", stderr);
    fputs("By default, dest = stdout\n", stderr);
    return 1;
  }

  int source_index = 2;
  
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
  
  if (argc == 3) {
      output = stdout;
  } else {
      output = fopen(argv[3], "wb");
      if (input == NULL)
      {
        fputs("File write error: couldn't open the destination file for uncrypted data", stderr);
        return 1;
      }
  }

  while ( 1 ) {
      epoc_get_next_frame(eh, &frame);
      printf("F3: %d\n", frame.electrode[F3]);
      fflush(stdout);
  }

  epoc_close(eh);
  return 0;
}
