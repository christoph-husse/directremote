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

#include "FrameAssembly.h"

#include <algorithm>

namespace DirectRemote {
void FrameAssembly::cleanupHistory(ConnectionMetrics &metrics) {
  while (reassembly.size() > 5) {
    auto it = reassembly.begin();
    auto entry = it->second;

    metrics.lostFrames++;

    reassembly.erase(it);
  }
}

std::shared_ptr<FrameAssembly::ReassemblyEntry>
FrameAssembly::getResassmblyEntry(int64_t trackingId,
                                  ConnectionMetrics &metrics) {
  std::shared_ptr<ReassemblyEntry> entry;

  auto it = reassembly.find(trackingId);

  if (it == reassembly.end()) {
    entry = std::make_shared<ReassemblyEntry>();

    entry->trackingId = trackingId;
    entry->receivedMsgCount = 0;

    reassembly.insert(std::make_pair(trackingId, entry));

    cleanupHistory(metrics);
  } else {
    entry = it->second;
  }

  return entry;
}

bool FrameAssembly::tryReconstruct(std::shared_ptr<ReassemblyEntry> entry,
                                   ConnectionMetrics &metrics) {
  if (entry->receivedMsgCount == entry->msgMap.size()) {
    reassembly.erase(entry->trackingId);

    if (!reassembly.empty()) {
      auto maxRemaining = (reassembly.rbegin())->first;

      if (maxRemaining > entry->trackingId) {
        metrics.outOfOrderFrames++;
        reassembly.clear();
      }
    }

    int offset = 0;
    for (auto msg : entry->messages) {
      int msgSize = msg->data.size();
      entry->data.resize(offset + msgSize);

      std::copy(msg->data.begin(), msg->data.end(),
                entry->data.begin() + offset);

      offset += msgSize;
    }

    return true;
  }
  metrics.validPackets++;

  return false;
}

std::shared_ptr<FrameAssembly::ReassemblyEntry> FrameAssembly::process(
    UdpChunk chunk, ConnectionMetrics &metrics) {
  auto entry = getResassmblyEntry(chunk.trackingId, metrics);

  if (entry->msgMap.empty()) {
    entry->msgMap.resize(chunk.msgCount);
    entry->messages.resize(chunk.msgCount);
  }

  if ((chunk.msgIndex < 0) || (chunk.msgIndex >= entry->msgMap.size())) {
    metrics.invalidPackets++;
    return nullptr;
  }

  if (!entry->messages[chunk.msgIndex]) {
    std::shared_ptr<MessageAssembly> msgAssembly =
        entry->msgMap[chunk.msgIndex];

    if (!msgAssembly) {
      msgAssembly = entry->msgMap[chunk.msgIndex] =
          std::make_shared<MessageAssembly>();
    }

    auto msg = msgAssembly->process(chunk, metrics);

    if (msg) {
      entry->messages[chunk.msgIndex] = msg;
      entry->receivedMsgCount++;
    }
  }

  if (tryReconstruct(entry, metrics)) {
    return entry;
  }

  return nullptr;
}
}  // namespace DirectRemote
