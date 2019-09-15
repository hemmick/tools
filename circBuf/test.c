#include "circular_buffer.h"
#include <stdio.h>
#include <string.h>

#define DEBUG_HEXDUMP_WRAP_WIDTH 16
#define LINE_COUNT_THRESHOLD 32
void debug_hexdump(const void *const address, size_t nBytes)
{
     if ((NULL == address) || (nBytes <= 0)) return;
     printf("%s", nBytes > DEBUG_HEXDUMP_WRAP_WIDTH ? "\r\n[ " : "[");
     for (size_t i = 1; i <= nBytes; i++) {
      printf("0x%2.2hhX", ((uint8_t*)address)[i-1]);
      // Insert a space if no wrap is occuring...
      if (i%DEBUG_HEXDUMP_WRAP_WIDTH != 0 || (i == nBytes)) {
        printf(" ");
      } else if (nBytes >= LINE_COUNT_THRESHOLD * DEBUG_HEXDUMP_WRAP_WIDTH) { 
        // Terminate with byte count if a wrap is occuring...
        printf("\t(%ld-%ld)\r\n  ", i-DEBUG_HEXDUMP_WRAP_WIDTH, i);
      } else {
        // Terminate a line...
        printf("\r\n  ");
      }
     }

     printf("%s", nBytes > DEBUG_HEXDUMP_WRAP_WIDTH ? "]\r\n" : "]");
}

int main (int argc, char **argv)
{
  char labels[16][16] = {"There's far", " too much to ", "take in here. ", "More to find ", 
                         "than can ever", " be found. ", "But the sun ", "rolling high", 
                         " through the ", "sapphire sky, ", " keeps great ", "and small ",
                         "on the ", "endless round. ", "The Circle", " of Liiiiiife!"};
  //debug_hexdump(labels, 16*16);
  char buffer[1433] = {0}; // Make a large buffer that won't fit the junk, above
  CircBuf_t circBuf;
  bool rc = circular_buffer_init(&circBuf, buffer, 16, sizeof(buffer)/16);
  bool top = true;
  if (true == rc) {
    debug_hexdump(&circBuf, sizeof(circBuf));
  }
  #if defined(DEBUG_PUSH) && (DEBUG_PUSH == 1)
    for (int i = 0; i < sizeof(buffer)/16 + 10; i++) {
      if(top) {
        rc = circular_buffer_push_head(&circBuf, labels[i%16], false);
      } else {
        rc = circular_buffer_push_tail(&circBuf, labels[i%16], false);
      }

      if (false == rc) {
        printf("failed at pushing \"%s\" to %s:%d\r\n", labels[i%16], top ? "head" : "tail", i);
        break;
      } else {
        printf("pushed \"%s\" to %s:%d\r\n", 
               strcmp(labels[i%16],"\r\n")==0?"CRLF":labels[i%16], 
               top ? "head" : "tail", i);
      }
      top = !top;
      //debug_hexdump(buffer, i * 16);
    }
  #endif

  // Let's test the element accessors...
  #if defined(DEBUG_PEEK) && (DEBUG_PEEK == 1)
    do {
      top = !top;
      for (int i = 0; i < circBuf.maxItems; i++) {
        char* b = circular_buffer_peek_at_index(top, circBuf, i);
        if (NULL == b) { 
          printf("Failed at %s idx%-2.2d\r\n", top?"head -":"tail +", i);
          break;
        } else {
          printf("Peeking at %s %-2.2d (%p): \"%s\"\r\n", 
                 top?"head -":"tail +", i,b, strcmp(b,"\r\n")==0?"CRLF":b);
        }
      }
    } while(top);
  #endif
 

  #if defined(DEBUG_POP) && (DEBUG_POP == 1)
    char derp[16] = {0};
    top = true;
    while (true) {
      if (top) {
        rc = circular_buffer_pop_head(&circBuf, derp);
      } else {
        rc = circular_buffer_pop_tail(&circBuf, derp);
      }
      uint16_t c = 0;
      if (false == rc) {
        break;
      } else {
        c = circular_buffer_get_count(circBuf);
        printf("popped: \"%s\" from %s, \t%d items remaining.\r\n", strcmp(derp,"\r\n")==0?"CRLF":derp , top ? "head" : "tail", c);
      }
      top = !top;
      //debug_hexdump(buffer, c * circBuf.itemSz);
    }
  #endif

  return 0;
}
