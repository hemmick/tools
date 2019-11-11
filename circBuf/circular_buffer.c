#include "circular_buffer.h"
#include <stddef.h>
#include <assert.h>
#include <string.h>

/* 
 * GNU AFFERO GENERAL PUBLIC LICENSE
 * Version 3, 19 November 2007
 * 
 * Copyleft john.hemmick@gmail.com
 */

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

inline bool circular_buffer_is_valid(CircBuf_t circBuf)
{
  // buf must not be null,
  // internal buf must not be null,
  // and either head and tail are beneath the max idx, or they are all INVALID (therefore buffer is empty)
  return ((NULL != circBuf._buffer) &&
          (((circBuf.head <= CIRCULAR_BUFFER_MAX_IDX) && (circBuf.tail <= CIRCULAR_BUFFER_MAX_IDX)) ||
           circular_buffer_is_empty(circBuf)));
}

// Forward-declared helpers
static inline bool buffer_wrapped(CircBuf_t c)
{
  return (c.head < c.tail) && 
         (NULL != c._buffer) &&
         (c.head <= CIRCULAR_BUFFER_MAX_IDX) && 
         (c.tail <= CIRCULAR_BUFFER_MAX_IDX);
}

inline void * peek_item(bool atHead, CircBuf_t circBuf)
{
  // Returns zero if empty OR if invalid...
  uint8_t count = circular_buffer_get_count(circBuf);
  if (count == 0) {
    return NULL;
  }

  if (atHead) { 
    return circBuf._buffer + (circBuf.head * circBuf.itemSz);
  } else {
    return circBuf._buffer + (circBuf.tail * circBuf.itemSz);
  }
}
#if defined (DEBUG_PEEK) && (DEBUG_PEEK)
void* circular_buffer_peek_at_index(bool fromHead, CircBuf_t circBuf, uint16_t idx)
#else 
static void* circular_buffer_peek_at_index(bool fromHead, CircBuf_t circBuf, uint16_t idx)
#endif
{
  uint32_t count = 0;

  if ( ((count = circular_buffer_get_count(circBuf)) < idx) ) { // Returns zero on empty or invalid state...
    return NULL; // No items exist
  } 
  
  uint16_t headIdx = circBuf.head/circBuf.itemSz;
  uint16_t tailIdx = circBuf.tail/circBuf.itemSz;
  uint16_t endIdx = (circBuf.maxItems - 1);

  if ((circBuf.tail <= circBuf.head)  || // The trivial case
      (!fromHead && (idx <= (endIdx - tailIdx))) || // Counting from tail the index does not require wrap
      (fromHead  && (idx <= headIdx))) {  // Counting from head the index does not require wrap
    // No wrap
    if (fromHead) {
      return circBuf._buffer + (headIdx - idx) * circBuf.itemSz;
    } else {
      return circBuf._buffer + (tailIdx + idx) * circBuf.itemSz;
    }
  } else {
    // Wrap calculation required
    if (fromHead) {

      return circBuf._buffer + (endIdx - idx + headIdx) * circBuf.itemSz;
    } else {
      return circBuf._buffer + ((tailIdx + idx)%circBuf.maxItems) * circBuf.itemSz;
    }
  }
}

bool get_distance_to_element(bool fromHead, 
                             CircBuf_t circBuf, 
                             void *element, 
                             uint16_t *output)
{
  if ((NULL == element) || // no element being searched
      (circular_buffer_is_valid(circBuf)) || // buffer is not valid
      ((uint8_t*)element < circBuf._buffer) || // Element before buffer min
      ((circBuf._buffer + circBuf.maxItems-1 * circBuf.itemSz) < (uint8_t*)element) || // Element after buffer max
      (NULL == output)) { // No place to store distance
    return false;
  }
  uint32_t byteOffset = 0;
  if (fromHead) {
    byteOffset = ((uint8_t*)element - circBuf._buffer);
  } else {
    byteOffset = (circBuf._buffer + circBuf.maxItems-1 * circBuf.itemSz) - (uint8_t*)element;
  }
  *output = byteOffset;
  return true;
}

bool circular_buffer_is_empty(CircBuf_t circBuf)
{
  return ((circBuf.head == CIRCULAR_BUFFER_INVALID) && (circBuf.tail == CIRCULAR_BUFFER_INVALID));
}

uint16_t circular_buffer_get_count(CircBuf_t circBuf)
{
  if ((circular_buffer_is_empty(circBuf)) || 
      (NULL == circBuf._buffer) || // invalid state...
      (circBuf.head > CIRCULAR_BUFFER_MAX_IDX) || // invalid state...
      (circBuf.tail > CIRCULAR_BUFFER_MAX_IDX)) { // invalid state...
    return 0;
  } else if (circBuf.head == circBuf.tail) {
    // special case, there's only one entry
    return 1;
  } else if (circBuf.head < circBuf.tail) {
    // wrap occured
    return (circBuf.itemSz * circBuf.maxItems - circBuf.tail + circBuf.head) / circBuf.itemSz + 1;
  } else {
    // circBuf->head > circBuf->tail
    return (circBuf.head - circBuf.tail) / circBuf.itemSz + 1;
  }
}

uint16_t get_circular_buffer_slots(CircBuf_t circBuf)
{
  if ((NULL == circBuf._buffer) || // invalid state...
      ((false == circular_buffer_is_empty(circBuf)) && // the buffer is not empty, but the indices are greater than max...
       ((circBuf.head > CIRCULAR_BUFFER_MAX_IDX) || (circBuf.tail > CIRCULAR_BUFFER_MAX_IDX)))) { // invalid state...
    return 0;
  }

  return (circBuf.maxItems - circular_buffer_get_count(circBuf));
}

bool circular_buffer_init(CircBuf_t *circBuf, 
                          void *internalBuffer, 
                          uint16_t itemSz, 
                          uint16_t maxItems)
{
  static_assert(((sizeof(circBuf->itemSz) == sizeof(uint16_t)) && 
                 (sizeof(circBuf->maxItems) == sizeof(uint16_t)) &&
                 (sizeof(circBuf->head) == sizeof(uint32_t)) &&
                 (sizeof(circBuf->tail) == sizeof(uint32_t))), 
                 "CircBuf_t struct changed");
  if ( (NULL == circBuf) ||
       (NULL == internalBuffer) || 
       (0 == itemSz) ||
       (0 == maxItems) ) {
    return false;
  }
  
  *circBuf = (CircBuf_t){
      .itemSz = itemSz,
      .maxItems = maxItems,
      .head = CIRCULAR_BUFFER_INVALID,
      .tail = CIRCULAR_BUFFER_INVALID,
      ._buffer = (uint8_t*)internalBuffer
  };

  memset(internalBuffer, 0, itemSz * maxItems);

  return true;
}

bool circular_buffer_push(bool toHead, CircBuf_t *circBuf, void *input, bool overwrite)
{
  if ( (NULL == circBuf) || 
       (false == circular_buffer_is_valid(*circBuf))) {
    return false;
  }

  uint16_t count = circular_buffer_get_count(*circBuf);
  uint8_t* finger = NULL;
  
  // If overwrite is not enabled but this push doesn't fit... there's no space
  if ((false == overwrite) && (count + 1 > circBuf->maxItems)) {
    return false;
  }

  // If there is space, push if there's space, or overwite the other end, otherwise
  uint32_t maxBytes = (circBuf->maxItems * circBuf->itemSz);
  if ((count + 1 <= circBuf->maxItems)) { // No overwrite required...
    if (count == 0) {
      // Special case, when buffer is empty, the indices are both CIRCULAR_BUFFER_INVALID; reset them now
      circBuf->head = circBuf->tail = 0;
    } else if (toHead) {
      // If we pushed to head, update the head index
      circBuf->head = (circBuf->head + circBuf->itemSz)%maxBytes;
    } else {
      // if we pushed to tail, update the tail index
      if(circBuf->tail < circBuf->itemSz) {
        // Underflow!
        circBuf->tail = maxBytes - circBuf->itemSz;
      } else {
        circBuf->tail -= circBuf->itemSz;
      }
    }

    // Indices are now updated, so update the finger
    finger = circBuf->_buffer + (toHead ? circBuf->head : circBuf->tail);
  } else { // Overwrite was necessary...
    assert(true == overwrite);
    
    // Grab a thing to overwrite from the opposite end of the buffer
    finger = (uint8_t*)circular_buffer_peek_at_index(!toHead, *circBuf, 0);

    // Update indices...
    if (toHead) {
      // if we overwrote head...
      uint16_t temp = circBuf->head;
      circBuf->head = (circBuf->head + circBuf->itemSz)%maxBytes;
      circBuf->tail = temp; // tail is now at previous head
    } else {
      // if we overwrote tail...
      uint16_t temp = circBuf->tail;
      if (circBuf->tail <= circBuf->itemSz) {
        // Underflow!
        circBuf->tail = maxBytes - circBuf->itemSz;
      } else {
        circBuf->tail -= circBuf->itemSz;
      }
      circBuf->head = temp; // head is now at previous tail
    }
  }

  assert((NULL != finger));
  assert(circBuf->_buffer <= finger);
  assert(finger <= (circBuf->_buffer + circBuf->maxItems * circBuf->itemSz));

  // Finger now points to next insertion point, so insert the item
  return (NULL != memcpy(finger, input, circBuf->itemSz));  
}

inline bool circular_buffer_push_head(CircBuf_t *circBuf, void *input, bool overwrite)
{
  return circular_buffer_push(true, circBuf, input, overwrite);
}

inline bool circular_buffer_push_tail(CircBuf_t *circBuf, void *input, bool overwrite)
{
  return circular_buffer_push(false, circBuf, input, overwrite);
}

bool circular_buffer_pop(bool fromHead, CircBuf_t *circBuf, void *output)
{
  if ( (NULL == circBuf) ||
       (false == circular_buffer_is_valid(*circBuf)) ||
       (circular_buffer_is_empty(*circBuf)) ) {
    return false;
  }
  // Grab the element being popped
  uint8_t *finger = (uint8_t*)circular_buffer_peek_at_index(fromHead, *circBuf, 0);
  uint32_t maxBytes = circBuf->itemSz * circBuf->maxItems;
  if (NULL == finger ) {
    // buffer is empty
    return false;
  } else if (circBuf->tail == circBuf->head) {
    // Special case where there's only one entry (tail=head=element)
    finger = circBuf->_buffer + (circBuf->head * circBuf->itemSz);
    circBuf->head = circBuf->tail = CIRCULAR_BUFFER_INVALID; // Denote an empty buffer
  } else {
    // Normal case
    if (fromHead) {
      if (circBuf->head < circBuf->itemSz) {
        // head underflow
        circBuf->head = maxBytes - circBuf->itemSz;
      } else {
        circBuf->head -= circBuf->itemSz;
      }
    } else { // from tail
      circBuf->tail = (circBuf->tail + circBuf->itemSz)%maxBytes;
    }
  }
  // Nobody cares about output...
  if (NULL == output) {
    memset(finger, 0, circBuf->itemSz);
    return true;
  }

  // Otherwise, user wants output, so copy it out
  return ( (NULL != memcpy(output, finger, circBuf->itemSz)) &&
           (NULL != memset(finger, 0, circBuf->itemSz)) );
}

inline bool circular_buffer_pop_head(CircBuf_t *circBuf, void *output)
{
  return circular_buffer_pop(true, circBuf, output);
}

inline bool circular_buffer_pop_tail(CircBuf_t *circBuf, void *output)
{
  return circular_buffer_pop(false, circBuf, output);
}
