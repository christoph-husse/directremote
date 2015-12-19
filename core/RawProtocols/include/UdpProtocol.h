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

#ifndef UDPPROTOCOL_H
#define UDPPROTOCOL_H

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

#include "Framework.h"

#include "FrameAssembly.h"
#include "PacketAssembly.h"
#include "Socket.h"

namespace DirectRemote {

enum class EProtocolState {
  Disconnected = 0,
  WaitingForPeer = 1,
  WaitingForProxy = 2,
  Connected = 3,
};

class UdpProtocol final {
 public:
  struct Options {
    bool disableReceiveTimeout = false;
    float eccRatio = 0.1f;
  };

 protected:
  Socket socket;
  SocketAddress sockAddress;
  PacketAssembly packetAssembly;
  FrameAssembly messageAssembly;
  EProtocolState state = EProtocolState::Disconnected;
  std::function<void(const std::vector<unsigned char> &packet)> onReceive;
  std::thread recvThread;
  std::thread connWatcherThread;
  int64_t sessionId = 0;
  std::mutex connMutex;
  std::condition_variable ctrlCondition;
  Options options;
  ConnectionMetrics metrics;

  void dispose();

  void sendPackets(std::vector<UdpChunk> &data, std::vector<UdpChunk> &ecc,
                   int64_t trackingId);

  void sendPacket(UdpChunk packet, int64_t trackingId);

  void recvThreadImpl();

  void connWatcherThreadImpl();

  void processPacket(std::shared_ptr<FrameAssembly::ReassemblyEntry> entry);

  void handleControlPacket(UdpChunk chunk);

 public:
  UdpProtocol(Options options = {});

  ~UdpProtocol();

  bool isConnected();

  void disconnect();

  bool connect(std::string address, int64_t sessionId);

  void sendTo(const unsigned char *bytes, int32_t byteCount,
              int64_t trackingId);

  void setReceiveHandler(
      std::function<void(const std::vector<unsigned char> &packet)> onReceive);
};
}  // namespace DirectRemote

#endif
