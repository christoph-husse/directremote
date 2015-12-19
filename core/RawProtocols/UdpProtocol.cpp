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

#include "UdpProtocol.h"
#include "ILogger.h"

namespace DirectRemote {

void UdpProtocol::dispose() { socket.close(); }

bool UdpProtocol::connect(std::string address, int64_t sessionId) {
  DR_LOG_INFO("Connecting to '", address, "'...");

  disconnect();

  if (!sockAddress.parse(address)) {
    DR_LOG_ERROR("Given address '", address, "' is invalid.");
    return false;
  }

  socket.create();

  this->sessionId = sessionId;
  state = EProtocolState::WaitingForProxy;
  recvThread = std::thread([this]() { recvThreadImpl(); });

  // try to establish a connection
  for (int i = 0;
       ((i < 10) || (state == EProtocolState::WaitingForPeer)) && (state != EProtocolState::Connected);
       i++) {
    UdpChunk ping = {};
    ping.isControlPacket = 1;
    ping.ctrl.command = EUdpCommand::Ping;
    sendPacket(ping, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(333));
  }

  if (state != EProtocolState::Connected) {
    DR_LOG_ERROR("Connection to '", address,
                 "' could not be established (timeout).");
    return false;
  }

  if (!options.disableReceiveTimeout) {
    connWatcherThread = std::thread([this]() { connWatcherThreadImpl(); });
  }

  return true;
}

void UdpProtocol::sendTo(const unsigned char *bytes, int32_t byteCount,
                         int64_t trackingId) {
  packetAssembly.processFrame(bytes, byteCount, options.eccRatio);
  sendPackets(packetAssembly.data, packetAssembly.ecc, trackingId);
}

void UdpProtocol::sendPackets(std::vector<UdpChunk> &data,
                              std::vector<UdpChunk> &ecc, int64_t trackingId) {
  int j = 0;
  int step = std::max(1, static_cast<int>(data.size()) /
                             std::max(1, static_cast<int>(ecc.size())));

  for (int i = 0, x = 0; x < data.size(); i++) {
    if ((i % step == 0) && (j < ecc.size())) {
      sendPacket(ecc[j++], trackingId);
    } else {
      sendPacket(data[x++], trackingId);
    }
  }

  for (; j < ecc.size(); j++) {
    sendPacket(ecc[j], trackingId);
  }
}

void UdpProtocol::sendPacket(UdpChunk packet, int64_t trackingId) {
  packet.sessionId = sessionId;
  packet.trackingId = trackingId;
  socket.sendto(&packet, UDP_CHUNK_SIZE, sockAddress);
}

void UdpProtocol::handleControlPacket(UdpChunk chunk) {
  switch (state) {
    case EProtocolState::Connected:
      DR_LOG_WARNING(
          "Received control packet while connected. This is unexpected.");
      break;
    case EProtocolState::WaitingForPeer:
      if (chunk.ctrl.command == EUdpCommand::Ping) {
        if (chunk.ctrl.isLinkEstablished) {
          DR_LOG_DEBUG("Connection to peer '", chunk.ctrl.peerAddress, ":",
                       chunk.ctrl.peerPort, "' established.");
          state = EProtocolState::Connected;
        }
      } else {
        DR_LOG_WARNING("Received a non-ping while waiting for peer.");
      }
      break;
    case EProtocolState::WaitingForProxy:
      if (chunk.ctrl.command == EUdpCommand::Ping) {
        DR_LOG_DEBUG("Proxy server responded. My address is '",
                     chunk.ctrl.yourAddress, ":", chunk.ctrl.yourPort,
                     "'. Waiting for peer to connect...");
        state = EProtocolState::WaitingForPeer;
      } else {
        DR_LOG_WARNING("Received a non-ping while waiting for proxy.");
      }
      break;
  }
}

void UdpProtocol::connWatcherThreadImpl() {
  int64_t lastCount = 0;

  while (state != EProtocolState::Disconnected) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(5000));

    if (lastCount == metrics.incomingPackets) {
      disconnect();
    }

    lastCount = metrics.incomingPackets;
  }
}

void UdpProtocol::recvThreadImpl() {
  UdpChunk chunk = {};
  SocketAddress senderAddr;

  while (state != EProtocolState::Disconnected) {
    int res = -1;

    if (socket.recvfrom(&chunk, UDP_CHUNK_SIZE, senderAddr) == UDP_CHUNK_SIZE) {
      if (chunk.isControlPacket) {
        handleControlPacket(chunk);
      } else {
        if (state == EProtocolState::Connected) {
          metrics.incomingPackets++;

          processPacket(messageAssembly.process(chunk, metrics));
        } else {
          DR_LOG_DEBUG("Ignoring packet, since not connected.");
        }
      }
    } else {
      DR_LOG_WARNING("Could not read from socket.");

      std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
  }

  DR_LOG_DEBUG("Receiving thread has terminated.");
}

void UdpProtocol::processPacket(
    std::shared_ptr<FrameAssembly::ReassemblyEntry> entry) {
  if (onReceive && entry) {
    try {
      onReceive(entry->data);
    } catch (std::exception &e) {
      DR_LOG_ERROR("Exception in user-supplied packet handler. [Details: '",
                   e.what(), "']");
    } catch (...) {
      DR_LOG_ERROR("Unknown exception in user-supplied packet handler.");
    }
  }
}

UdpProtocol::UdpProtocol(Options options)
    : socket(ESocketProtocol::Udp), options(options) {}

UdpProtocol::~UdpProtocol() { disconnect(); }

bool UdpProtocol::isConnected() { return state == EProtocolState::Connected; }

void UdpProtocol::disconnect() {
  dispose();

  state = EProtocolState::Disconnected;

  if ((std::this_thread::get_id() != connWatcherThread.get_id()) &&
      connWatcherThread.joinable()) {
    DR_LOG_DEBUG("Waiting for watchdog thread to terminate...");
    connWatcherThread.join();
  }

  if (recvThread.joinable()) {
    DR_LOG_DEBUG("Waiting for receiving thread to terminate...");
    recvThread.join();
  }
}

void UdpProtocol::setReceiveHandler(
    std::function<void(const std::vector<unsigned char> &packet)> onReceive) {
  this->onReceive = onReceive;
}
}
