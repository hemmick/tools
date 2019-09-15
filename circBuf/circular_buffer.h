#ifndef _CIRCULAR_BUFFER_H__
#define _CIRCULAR_BUFFER_H__
#define DEBUG_PUSH 1
#define DEBUG_POP 1
#define DEBUG_PEEK 1
#include <stdint.h>
#include <stdbool.h>
/* 
 * GNU AFFERO GENERAL PUBLIC LICENSE
 * Version 3, 19 November 2007
 * 
 * Copyleft john.hemmick@gmail.com
 */

typedef struct _circular_buffer_ {
  uint16_t itemSz;  // The size of an individual item in the circular buffer
  uint16_t maxItems;// The maximum count of items that can be stored in this buffer
  uint32_t head;    // The current head of this buffer
  uint32_t tail;    // The current tail of this buffer
  uint8_t *_buffer; // The memory being used by this buffer
} CircBuf_t;

typedef enum _circular_buffer_return_codes {
  // All other return codes up to CIRCULAR_BUFFER_MAX_IDX
  // may be used to denote 'how many' buffer items there are
  // do not use any values below CIRCULAR_BUFFER_MAX_IDX
  CIRCULAR_BUFFER_MAX_IDX = (((uint32_t)UINT16_MAX) * UINT16_MAX), // The maximum possible index of head/tail
  
  // everything after max_idx and buffer_invalid can be used for return codes

  CIRCULAR_BUFFER_INVALID = (UINT32_MAX) // Returns if a circular buffer function fails
} CircBufReturnCode;

// returns true if the circular buffer is valid, false otherwise
bool circular_buffer_is_valid(CircBuf_t circBuf);

// Returns true if a circular buffer is empty, false otherwise
bool circular_buffer_is_empty(CircBuf_t circBuf);

// Returns the number of elements in a circular buffer.
// Invalid circular buffers return a count of zero.
uint16_t circular_buffer_get_count(CircBuf_t circBuf);

// Returns the number of empty slots in a circular buffer.
// Invalid circular buffers return a slot count of zero.
uint16_t circular_buffer_get_slots(CircBuf_t circBuf);

// Returns true if a circular buffer has been initialized, false otherwise
bool circular_buffer_init(CircBuf_t *circBuf, 
                          void *internalBuffer, 
                          uint16_t itemSz, 
                          uint16_t maxItems);

void * peek_item(bool atHead, CircBuf_t circBuf);

bool circular_buffer_push(bool toHead, CircBuf_t *circBuf, void *input, bool overwrite);
bool circular_buffer_push_head(CircBuf_t *circBuf, void *input, bool overwrite);
bool circular_buffer_push_tail(CircBuf_t *circBuf, void *input, bool overwrite);

bool circular_buffer_pop(bool fromHead, CircBuf_t *circBuf, void *output);
bool circular_buffer_pop_head(CircBuf_t *circBuf, void *output);
bool circular_buffer_pop_tail(CircBuf_t *circBuf, void *output);

#if (defined(DEBUG_PEEK) && DEBUG_PEEK)
  // Temporarily exposed for testing
  void* circular_buffer_peek_at_index(bool fromHead, CircBuf_t circBuf, uint16_t idx);
#endif

bool circular_buffer_get_distance_to_element(bool fromHead, 
                                             CircBuf_t circBuf, 
                                             void *element, 
                                             uint16_t *output);
#endif
