/*

Copyright (c) 2015 Christoph Husse

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef UDPCHUNK_H
#define UDPCHUNK_H

#include <stdint.h>

namespace DirectRemote {

#define UDP_CHUNK_SIZE 512
#define CHUNK_ECC_SIZE (UDP_CHUNK_SIZE - 16)
#define CHUNK_PAYLOAD_SIZE (UDP_CHUNK_SIZE - 18)
#define MAX_MESSAGE_SIZE (CHUNK_PAYLOAD_SIZE * 127)

struct EUdpCommand {
  enum {
    Ping = 0,
    LinkStatus = 1,
  };
};

#include "struct_pack_one.h"
struct UdpChunk {
  uint64_t sessionId : 48;  // must be the first member
  uint64_t isEccChunk : 1;
  uint64_t isControlPacket : 1;

  // ECC protected message chunks
  uint64_t chunkIndex : 7;
  uint64_t chunkCount : 7;

  uint64_t trackingId : 48;

  // there is no cross-message ECC
  uint64_t msgIndex : 8;
  uint64_t msgCount : 8;

  union {
    struct {
      uint16_t size : 15;
      uint16_t isConnected : 1;
      unsigned char bytes[CHUNK_PAYLOAD_SIZE];
    } data;

    struct {
      // ecc portion should cover all except sessionId (which is used for
      // routing)
      unsigned char bytes[CHUNK_ECC_SIZE];
    } ecc;

    struct {
      int32_t command;
      bool isLinkEstablished;
      char peerAddress[32];
      int32_t peerPort;
      char yourAddress[32];
      int32_t yourPort;
    } ctrl;
  };
} PACKED;
#include "struct_pack_default.h"

static_assert(sizeof(UdpChunk) == UDP_CHUNK_SIZE,
              "UdpChunk is not configured correctly.");

struct ConnectionMetrics {
  int64_t lostPackets = 0;
  int64_t lostFrames = 0;
  int64_t invalidFrames = 0;
  int64_t outOfOrderFrames = 0;
  int64_t incomingPackets = 0;
  int64_t validPackets = 0;
  int64_t invalidPackets = 0;
  int64_t duplicatePackets = 0;
};

struct UdpPayloadChunk {
  char payload[CHUNK_PAYLOAD_SIZE];
};

static_assert(sizeof(UdpPayloadChunk) == CHUNK_PAYLOAD_SIZE,
              "UdpPayloadChunk is not configured correctly.");
}  // namespace DirectRemote

#endif  // UDPCHUNK_H
