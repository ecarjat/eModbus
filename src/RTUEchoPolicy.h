#pragma once
#include <stddef.h>
#include <stdint.h>

// Only discard responses whose length cannot also be a valid request for the
// same function. Single-write responses deliberately echo their requests.
inline bool rtuResponseCannotBeRequest(const uint8_t* frame, size_t size) {
  if (!frame || size < 3) return false;
  switch (frame[1]) {
    case 0x01: case 0x02: case 0x03: case 0x04:
      return size != 6 && size == size_t(3 + frame[2]);
    case 0x0f: case 0x10:
      return size == 6;
    default:
      return false;
  }
}

// The caller consumes the buffered candidate after one receive attempt.
inline bool rtuIsBufferedResponseEcho(bool buffered, const uint8_t* incoming, size_t incomingSize,
                                      const uint8_t* response, size_t responseSize) {
  if (!buffered || !incoming || !rtuResponseCannotBeRequest(response, responseSize) ||
      incomingSize != responseSize) return false;
  for (size_t i = 0; i < incomingSize; ++i) {
    if (incoming[i] != response[i]) return false;
  }
  return true;
}
