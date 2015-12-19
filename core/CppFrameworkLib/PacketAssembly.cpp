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

#include "PacketAssembly.h"
#include "ErasureCode.h"

#include <algorithm>

namespace DirectRemote {

bool PacketAssembly::processFrame(const unsigned char *bytes, int byteCount,
                                  float eccPacketsPerDataPacket) {
  data.clear();
  ecc.clear();

  const int msgCount = 1 + std::max(0, byteCount - 1) / MAX_MESSAGE_SIZE;

  for (int i = 0, iMsg = 0; i < std::max(1, byteCount);
       i += MAX_MESSAGE_SIZE, iMsg++) {
    int msgSize = std::min(MAX_MESSAGE_SIZE, byteCount - i);

    if (!processMessageInternal(bytes + i, msgSize, eccPacketsPerDataPacket,
                                dataPerMessage, eccPerMessage, eccRawPerMessage,
                                packetPtrsPerMessage)) {
      return false;
    }

    for (auto &p : dataPerMessage) {
      p.msgCount = static_cast<uint64_t>(msgCount);
      p.msgIndex = static_cast<uint64_t>(iMsg);

      data.push_back(p);
    }

    for (auto &p : eccPerMessage) {
      p.msgCount = static_cast<uint64_t>(msgCount);
      p.msgIndex = static_cast<uint64_t>(iMsg);

      ecc.push_back(p);
    }
  }

  return true;
}

bool PacketAssembly::processMessage(const unsigned char *bytes, int byteCount,
                                    float eccPacketsPerDataPacket) {
  dataPerMessage.clear();
  eccPerMessage.clear();
  eccRawPerMessage.clear();
  packetPtrsPerMessage.clear();

  return processMessageInternal(bytes, byteCount, eccPacketsPerDataPacket, data,
                                ecc, eccRawPerMessage, packetPtrsPerMessage);
}

bool PacketAssembly::processMessageInternal(
    const unsigned char *bytes, size_t byteCount, float eccPacketsPerDataPacket,
    std::vector<UdpChunk> &outData, std::vector<UdpChunk> &outEcc,
    std::vector<unsigned char> &eccRaw,
    std::vector<const unsigned char *> &packetPtrs) {
  outData.clear();
  outEcc.clear();
  eccRaw.clear();
  packetPtrs.clear();

  UdpChunk chunk = {};
  const size_t chunkCount = 1u +
                            (std::max(static_cast<size_t>(1u), byteCount) - 1) /
                                sizeof(chunk.data.bytes);

  if (chunkCount > 127) {
    return false;
  }

  for (size_t chunkIndex = 0, offset = 0; chunkIndex < chunkCount;
       chunkIndex++, offset += sizeof(chunk.data.bytes)) {
    chunk.chunkCount = chunkCount;
    chunk.chunkIndex = chunkIndex;
    chunk.isEccChunk = false;

    chunk.data.isConnected = 1;
    chunk.data.size =
        std::max(static_cast<int64_t>(0),
                 std::min(static_cast<int64_t>(byteCount - offset),
                          static_cast<int64_t>(sizeof(chunk.data.bytes))));

    memcpy(chunk.data.bytes, bytes + offset, chunk.data.size);

    outData.push_back(chunk);
  }

  static_assert(((CHUNK_ECC_SIZE % 8) == 0),
                "outEcc blocks need to be a multiple of 8.");

  packetPtrs.resize(chunkCount);

  outEcc.resize(std::max(
      1u, std::min(128u, static_cast<unsigned>(chunkCount *
                                               eccPacketsPerDataPacket))));
  eccRaw.resize(outEcc.size() * CHUNK_ECC_SIZE);

  for (size_t i = 0; i < outData.size(); i++) {
    packetPtrs[i] = outData[i].ecc.bytes;
  }

  cauchy_256_encode(static_cast<int>(chunkCount),
                    static_cast<int>(outEcc.size()),
                    reinterpret_cast<const unsigned char **>(packetPtrs.data()),
                    eccRaw.data(), CHUNK_ECC_SIZE);

  for (size_t i = 0; i < outEcc.size(); i++) {
    auto &eccChunk = outEcc[i];
    memcpy(eccChunk.ecc.bytes, &eccRaw[i * CHUNK_ECC_SIZE], CHUNK_ECC_SIZE);

    eccChunk.chunkCount = outEcc.size();
    eccChunk.chunkIndex = i;
    eccChunk.isEccChunk = true;
  }

  return true;
}
}  // namespace DirectRemote
