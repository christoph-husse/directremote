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

#ifndef MESSAGEASSEMBLY_H
#define MESSAGEASSEMBLY_H

#include <stdint.h>
#include <map>
#include <memory>
#include <string.h>
#include <unordered_map>
#include <vector>

#include "UdpChunk.h"

namespace DirectRemote {
class MessageAssembly {
 public:
  struct ReassemblyEntry {
    int64_t trackingId;
    size_t receivedDataChunks, receivedEccChunks;
    std::vector<UdpChunk> eccMap, dataMap;
    std::vector<bool> hasDataChunk, hasEccChunk;

    std::vector<unsigned char> data;

    bool hasEnoughChunks();
    bool tryReconstruct();
  };

  std::shared_ptr<ReassemblyEntry> process(UdpChunk chunk,
                                           ConnectionMetrics &metrics);

 private:
  void cleanupHistory(ConnectionMetrics &metrics);
  std::map<int64_t, std::shared_ptr<ReassemblyEntry>> reassembly;
  std::shared_ptr<ReassemblyEntry> reassembleEccPacket(
      UdpChunk chunk, ConnectionMetrics &metrics);
  std::shared_ptr<ReassemblyEntry> reassembleDataPacket(
      UdpChunk chunk, ConnectionMetrics &metrics);
  bool tryReconstruct(std::shared_ptr<ReassemblyEntry> entry,
                      ConnectionMetrics &metrics);
  std::shared_ptr<ReassemblyEntry> getResassmblyEntry(
      int64_t trackingId, ConnectionMetrics &metrics);

  void cleanupHistory();
};
}  // namespace DirectRemote

#endif
