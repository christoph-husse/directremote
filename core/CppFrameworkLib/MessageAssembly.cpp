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

#include "MessageAssembly.h"
#include "ErasureCode.h"
#include "Framework.h"

#include <algorithm>

namespace DirectRemote {
void MessageAssembly::cleanupHistory(ConnectionMetrics &metrics) {
  while (reassembly.size() > 512) {
    auto it = reassembly.begin();
    auto entry = it->second;

    metrics.lostPackets += entry->dataMap.size() - entry->receivedDataChunks +
                           entry->eccMap.size() - entry->receivedEccChunks;

    reassembly.erase(it);
  }
}

std::shared_ptr<MessageAssembly::ReassemblyEntry> MessageAssembly::process(
    UdpChunk chunk, ConnectionMetrics &metrics) {
  if (chunk.isEccChunk) {
    return reassembleEccPacket(chunk, metrics);
  }
  return reassembleDataPacket(chunk, metrics);
}

std::shared_ptr<MessageAssembly::ReassemblyEntry>
MessageAssembly::getResassmblyEntry(int64_t trackingId,
                                    ConnectionMetrics &metrics) {
  std::shared_ptr<ReassemblyEntry> entry;

  auto it = reassembly.find(trackingId);

  if (it == reassembly.end()) {
    entry = std::make_shared<ReassemblyEntry>();

    entry->trackingId = trackingId;
    entry->receivedDataChunks = 0;
    entry->receivedEccChunks = 0;

    reassembly.insert(std::make_pair(trackingId, entry));

    cleanupHistory(metrics);
  } else {
    entry = it->second;
  }

  return entry;
}

bool MessageAssembly::tryReconstruct(std::shared_ptr<ReassemblyEntry> entry,
                                     ConnectionMetrics &metrics) {
  if (entry->hasEnoughChunks()) {
    reassembly.erase(entry->trackingId);

    if (!reassembly.empty()) {
      auto maxRemaining = (reassembly.rbegin())->first;

      if (maxRemaining > entry->trackingId) {
        metrics.outOfOrderFrames++;
        reassembly.clear();
      }
    }

    if (entry->tryReconstruct()) {
      metrics.validPackets++;
      return true;
    }
    metrics.invalidFrames++;

  } else {
    metrics.validPackets++;
  }

  return false;
}

std::shared_ptr<MessageAssembly::ReassemblyEntry>
MessageAssembly::reassembleEccPacket(UdpChunk chunk,
                                     ConnectionMetrics &metrics) {
  auto entry = getResassmblyEntry(chunk.trackingId, metrics);

  if (entry->eccMap.empty()) {
    entry->eccMap.resize(chunk.chunkCount);
    entry->hasEccChunk.resize(chunk.chunkCount);
  }

  if ((chunk.chunkIndex < 0) ||
      (chunk.chunkIndex >= entry->hasEccChunk.size())) {
    metrics.invalidPackets++;
    return nullptr;
  }

  bool alreadyReceived = entry->hasEccChunk[chunk.chunkIndex];
  entry->hasEccChunk[chunk.chunkIndex] = true;

  if (!alreadyReceived) {
    entry->eccMap[chunk.chunkIndex] = chunk;
    entry->receivedEccChunks++;

    if (tryReconstruct(entry, metrics)) {
      return entry;
    }
  } else {
    metrics.duplicatePackets++;
  }

  return nullptr;
}

std::shared_ptr<MessageAssembly::ReassemblyEntry>
MessageAssembly::reassembleDataPacket(UdpChunk chunk,
                                      ConnectionMetrics &metrics) {
  auto entry = getResassmblyEntry(chunk.trackingId, metrics);

  if (entry->dataMap.empty()) {
    entry->data.resize(chunk.chunkCount * sizeof(chunk.data.bytes));
    entry->dataMap.resize(chunk.chunkCount);
    entry->hasDataChunk.resize(chunk.chunkCount);
  }

  if ((chunk.chunkIndex < 0) ||
      (chunk.chunkIndex >= entry->hasDataChunk.size())) {
    metrics.invalidPackets++;
    return nullptr;
  }

  bool alreadyReceived = entry->hasDataChunk[chunk.chunkIndex];
  entry->hasDataChunk[chunk.chunkIndex] = true;

  if (!alreadyReceived) {
    entry->dataMap[chunk.chunkIndex] = chunk;
    entry->receivedDataChunks++;

    if (tryReconstruct(entry, metrics)) {
      return entry;
    }
  } else {
    metrics.duplicatePackets++;
  }

  return nullptr;
}

bool MessageAssembly::ReassemblyEntry::hasEnoughChunks() {
  return !dataMap.empty() &&
         (dataMap.size() <= receivedDataChunks + receivedEccChunks);
}

bool MessageAssembly::ReassemblyEntry::tryReconstruct() {
  if (dataMap.size() > receivedDataChunks) {
    // use ECC to reconstruct missing packets
    std::vector<Block> recoveryBlocks;

    recoveryBlocks.resize(receivedDataChunks + receivedEccChunks);

    size_t j = 0;
    const auto K = hasDataChunk.size();
    const auto M = hasEccChunk.size();

    for (size_t i = 0, iEcc = 0; i < K; i++) {
      auto &block = recoveryBlocks[j++];

      if (!hasDataChunk[i]) {
        bool hasEcc = false;

        for (; iEcc < M; iEcc++) {
          if ((hasEcc = hasEccChunk[iEcc])) {
            break;
          }
        }

        if (!hasEcc) {
          return false;
        }

        block.row = static_cast<unsigned char>(K + iEcc);
        block.data = dataMap[i].ecc.bytes;

        memcpy(block.data, eccMap[iEcc].ecc.bytes, CHUNK_ECC_SIZE);

        iEcc++;

        continue;
      } else {
        block.row = static_cast<unsigned char>(i);
        block.data = dataMap[i].ecc.bytes;
      }
    }

    cauchy_256_decode(static_cast<int>(dataMap.size()),
                      static_cast<int>(eccMap.size()), recoveryBlocks.data(),
                      CHUNK_ECC_SIZE);
  }

  data.resize(data.size() - (CHUNK_PAYLOAD_SIZE - dataMap.back().data.size));

  for (size_t i = 0; i < dataMap.size(); i++) {
    memcpy(&data[i * CHUNK_PAYLOAD_SIZE], dataMap[i].data.bytes,
           CHUNK_PAYLOAD_SIZE);
  }

  return true;
}
}  // namespace DirectRemote
