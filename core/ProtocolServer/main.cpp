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
#include <chrono>
#include <stdlib.h>
#include <thread>
#include <mutex>

#include "ILogger.h"
#include "UdpChunk.h"
#include "Socket.h"

using namespace DirectRemote;

#if BOOST_COMP_MSVC
#pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#endif


struct IdMapping {
  bool isValid = false;
  SocketAddress sourceAddr = {};
  SocketAddress targetAddr = {};
};

int main(int argc, char **argv) {
  Socket sock(ESocketProtocol::Udp);
  SocketAddress sockAddress;
  std::unordered_map<int64_t, IdMapping> mappings;
  int exitCode = -1;
  bool isTerminated = false;
  std::thread httpAnnouceThread;

  DR_LOG_INFO("Running ProtocolServer...");

  if (!sock.isValid()) {
    goto ERROR_EXIT;
  }

  sockAddress.parse("0.0.0.0:41988");

  httpAnnouceThread = std::thread([=]() {
    Socket httpSock(ESocketProtocol::Tcp);
    std::string response = "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Access-Control-Allow-Origin: *\r\n"
      "Access-Control-Allow-Methods: GET\r\n"
      "Access-Control-Allow-Headers: Content-Type\r\n"
      "Content-Length: 45\r\n\r\n"
      "{\"id\":\"1a19e18c-9424-4f9f-896c-c498e722da10\"}";

    if (!httpSock.bind(sockAddress)) {
      DR_LOG_FATAL("Could not bind to TCP endpoint ", sockAddress.ipAddress(), ":", sockAddress.port(), ".");
      return;
    }

    if (!httpSock.listen()) {
      DR_LOG_FATAL("Could not listen on TCP endpoint ", sockAddress.ipAddress(), ":", sockAddress.port(), ".");
      return;
    }

    while (!isTerminated) {
      Socket client = httpSock.accept();
      unsigned char buffer[1024 * 128] = {};
      client.recv(buffer, sizeof(buffer) - 1);
      client.send(response.c_str(), response.size());
      client.close();
    }
  });

  if (!sock.bind(sockAddress)) {
    goto ERROR_EXIT;
  }

  while (true) {
    SocketAddress sourceAddr;
    UdpChunk chunk = {};

    int msgLen = sock.recvfrom(&chunk, sizeof(chunk), sourceAddr);

    if (msgLen < 0) {
      continue;
    }

    if (mappings.size() > 5000) {
      mappings.clear();
    }

    auto sessionId = chunk.sessionId;
    auto it = mappings.find(sessionId);

    if (it == mappings.end()) {
      DR_LOG_DEBUG("Starting new pairing for '", sourceAddr.ipAddress(), ":",
                   sourceAddr.port(), "'.");

      IdMapping m = {};
      m.sourceAddr = sourceAddr;

      mappings[sessionId] = m;
    } else {
      IdMapping &m = it->second;

      if (chunk.isControlPacket) {
        switch (chunk.ctrl.command) {
          case EUdpCommand::Ping:
            memset(chunk.ecc.bytes, 0, sizeof(chunk.ecc.bytes));

            chunk.ctrl.command = EUdpCommand::Ping;
            chunk.isControlPacket = 1;
            chunk.ctrl.isLinkEstablished = m.isValid;
            strncpy(chunk.ctrl.yourAddress, sourceAddr.ipAddress().c_str(),
                    sizeof(chunk.ctrl.yourAddress));
            chunk.ctrl.yourPort = sourceAddr.port();

            if (!m.isValid) {
              if (memcmp(&m.sourceAddr, &sourceAddr, sizeof(sourceAddr)) != 0) {
                m.targetAddr = sourceAddr;
                m.isValid = true;
              }
            }

            if (m.isValid) {
              if (memcmp(&m.sourceAddr, &sourceAddr, sizeof(sourceAddr)) == 0) {
                strncpy(chunk.ctrl.peerAddress,
                        m.targetAddr.ipAddress().c_str(),
                        sizeof(chunk.ctrl.peerAddress));
                chunk.ctrl.peerPort = m.targetAddr.port();
              } else {
                strncpy(chunk.ctrl.peerAddress,
                        m.sourceAddr.ipAddress().c_str(),
                        sizeof(chunk.ctrl.peerAddress));
                chunk.ctrl.peerPort = m.sourceAddr.port();
              }

              DR_LOG_DEBUG("Processing ping from '", sourceAddr.ipAddress(),
                           ":", sourceAddr.port(), "'. Now paired with '",
                           chunk.ctrl.peerAddress, ":", chunk.ctrl.peerPort,
                           "'!");
            } else {
              DR_LOG_DEBUG("Processing ping from '", sourceAddr.ipAddress(),
                           ":", sourceAddr.port(),
                           "'. Waiting for peer to connect...");
            }

            sock.sendto(&chunk, sizeof(chunk), sourceAddr);
            break;
        }
      } else {
        if (!m.isValid) {
          mappings.erase(it);
        } else {
          SocketAddress targetAddr;

          if (memcmp(&m.sourceAddr, &sourceAddr, sizeof(sourceAddr)) == 0) {
            targetAddr = m.targetAddr;
          } else {
            if (memcmp(&m.targetAddr, &sourceAddr, sizeof(sourceAddr)) == 0) {
              targetAddr = m.sourceAddr;
            } else {
              mappings.erase(it);

              continue;
            }
          }

          sock.sendto(&chunk, sizeof(chunk), targetAddr);
        }
      }
    }
  }

  exitCode = 0;

ERROR_EXIT:
  isTerminated = true;

  if (httpAnnouceThread.joinable()) {
    httpAnnouceThread.join();
  }

  return exitCode;
}