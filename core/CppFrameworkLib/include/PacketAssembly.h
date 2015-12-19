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

#ifndef PACKETASSEMBLY_H
#define PACKETASSEMBLY_H

#include <memory>
#include <stdint.h>
#include <string.h>
#include <unordered_map>
#include <vector>

#include "IPerformanceMonitor.h"
#include "UdpChunk.h"

namespace DirectRemote {

class PacketAssembly {
 private:
  std::vector<const unsigned char *> packetPtrsPerMessage;
  std::vector<unsigned char> eccRawPerMessage;
  std::vector<UdpChunk> dataPerMessage;
  std::vector<UdpChunk> eccPerMessage;

  static bool processMessageInternal(
      const unsigned char *bytes, size_t byteCount,
      float eccPacketsPerDataPacket, std::vector<UdpChunk> &outData,
      std::vector<UdpChunk> &outEcc, std::vector<unsigned char> &eccRaw,
      std::vector<const unsigned char *> &packetPtrs);

 public:
  std::vector<UdpChunk> data;
  std::vector<UdpChunk> ecc;

  bool processFrame(const unsigned char *bytes, int byteCount,
                    float eccPacketsPerDataPacket = 0.1f);

  bool processMessage(const unsigned char *bytes, int byteCount,
                      float eccPacketsPerDataPacket = 0.1f);
};
}  // namespace DirectRemote

#endif