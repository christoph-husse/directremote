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

#include <string>
#include <unordered_map>
#include <stdint.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <mutex>

#include "UdpChunk.h"
#include "Socket.h"

using namespace DirectRemote;

#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#endif

#define TIMEOUT_SECONDS 3

class IdMapping {
 private:
  std::thread connThread;

 public:
  bool isValid = false;
  int32_t sourceTimeStamp = 0;
  int32_t targetTimeStamp = 0;
  Socket sourceSock = Socket(ESocketProtocol::Tcp);
  Socket targetSock = Socket(ESocketProtocol::Tcp);
  SocketAddress sourceAddr;
  SocketAddress targetAddr;

  IdMapping(SocketAddress sourceAddr, int32_t currentTime) {
    sourceAddr = sourceAddr;
    sourceTimeStamp = currentTime;
    targetTimeStamp = currentTime;
  }

  void processPacket(UdpChunk chunk, SocketAddress chunkOrigin) {
    if (chunk.isControlPacket) {
      switch (chunk.ctrl.command) {
        case EUdpCommand::Ping:
          memset(chunk.ecc.bytes, 0, sizeof(chunk.ecc.bytes));

          chunk.ctrl.command = EUdpCommand::Ping;
          chunk.ctrl.isLinkEstablished = isValid;
          strncpy(chunk.ctrl.yourAddress, chunkOrigin.ipAddress().c_str(),
                  sizeof(chunk.ctrl.yourAddress));
          chunk.ctrl.yourPort = chunkOrigin.port();

          if (isValid) {
            if (memcmp(&sourceAddr, &chunkOrigin, sizeof(chunkOrigin)) == 0) {
              strncpy(chunk.ctrl.peerAddress, targetAddr.ipAddress().c_str(),
                      sizeof(chunk.ctrl.peerAddress));
              chunk.ctrl.peerPort = targetAddr.port();
            } else {
              strncpy(chunk.ctrl.peerAddress, sourceAddr.ipAddress().c_str(),
                      sizeof(chunk.ctrl.peerAddress));
              chunk.ctrl.peerPort = chunkOrigin.port();
            }
          }

          targetSock.send(&chunk, sizeof(chunk));
          break;
      }
    }

    Socket *remoteSocket;

    if (sourceAddr.equalTo(chunkOrigin)) {
      if (!isValid) {
        return;
      }

      sourceTimeStamp = currentTime;
      remoteSocket = &targetSock;
    } else {
      if (!isValid) {
        targetAddr = chunkOrigin;
        isValid = true;
      }

      if (targetAddr.equalTo(chunkOrigin)) {
        targetTimeStamp = currentTime;
        remoteSocket = &sourceSock;
      } else {
        mappings.erase(it);

        return;
      }
    }

    if (!chunk.isControlPacket) {
      remoteSocket->send(&chunk, sizeof(chunk));
    }
  }
};

int main(int argc, char **argv) {
  Socket sock(ESocketProtocol::Tcp);
  SocketAddress sockAddress;
  std::unordered_map<int64_t, IdMapping> mappings;
  int32_t currentTime = 0;
  std::mutex mutex;
  bool isTerminated = false;
  int appResult = -1;

  auto timingThread = std::thread([&]() {
    while (!isTerminated) {
      currentTime++;

      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });

  if (!sock.isValid()) {
    goto ERROR_EXIT;
  }

  sockAddress.parse("0.0.0.0:41988");

  if (!sock.bind(sockAddress) || !sock.listen()) {
    goto ERROR_EXIT;
  }

  while (!isTerminated) {
    SocketAddress sourceAddr;
    Socket clientSock = sock.accept(sourceAddr);
    UdpChunk chunk = {};

    if (!sock.recvAll(&chunk, sizeof(chunk))) {
      continue;
    }

    if (mappings.size() > 5000) {
      mappings.clear();
    }

    SocketAddress targetAddr;
    auto sessionId = chunk.sessionId;
    auto it = mappings.find(sessionId);

    if ((it == mappings.end()) ||
        (currentTime - it->second.sourceTimeStamp > 3) ||
        (currentTime - it->second.targetTimeStamp > 3)) {
      IdMapping m(sourceAddr, currentTime);

      mappings[sessionId] = m;
    } else {
      it->second.processPacket(chunk);
    }
  }

  appResult = 0;

ERROR_EXIT:
  isTerminated = true;

  if (timingThread.joinable()) {
    timingThread.join();
  }

  return appResult;
}