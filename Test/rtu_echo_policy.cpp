#include "../src/RTUEchoPolicy.h"
#include <cassert>

int main() {
  const uint8_t readReply[] = {2, 4, 4, 0, 1, 0, 2};
  const uint8_t otherReply[] = {2, 4, 4, 0, 1, 0, 3};
  const uint8_t writeReply[] = {2, 16, 0, 1, 0, 2};
  const uint8_t singleWrite[] = {2, 6, 0, 1, 0, 2};
  const uint8_t readRequest[] = {2, 4, 0, 1, 0, 2};
  assert(rtuIsBufferedResponseEcho(true, readReply, sizeof readReply, readReply, sizeof readReply));
  assert(rtuIsBufferedResponseEcho(true, writeReply, sizeof writeReply, writeReply, sizeof writeReply));
  assert(!rtuIsBufferedResponseEcho(false, readReply, sizeof readReply, readReply, sizeof readReply));
  assert(!rtuIsBufferedResponseEcho(true, otherReply, sizeof otherReply, readReply, sizeof readReply));
  assert(!rtuIsBufferedResponseEcho(true, singleWrite, sizeof singleWrite, singleWrite, sizeof singleWrite));
  assert(!rtuIsBufferedResponseEcho(true, readRequest, sizeof readRequest, readRequest, sizeof readRequest));
  assert(!rtuIsBufferedResponseEcho(true, nullptr, 0, nullptr, 0));
}
