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

#include "Socket.h"

#if BOOST_OS_WINDOWS

typedef int socklen_t;

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#else

typedef int SOCKET;
#define INVALID_SOCKET ~0

#include <memory.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/in.h>

#define closesocket(handle) ::close(handle)

#endif

#undef min
#undef max

namespace DirectRemote {

std::string SocketAddress::ipAddress() const {
  return inet_ntoa(
      reinterpret_cast<sockaddr_in *>(const_cast<unsigned char *>(data))
          ->sin_addr);
}

int32_t SocketAddress::port() const {
  return ntohs(
      reinterpret_cast<sockaddr_in *>(const_cast<unsigned char *>(data))
          ->sin_port);
}

SocketAddress::SocketAddress() { memset(data, 0, sizeof(data)); }

bool SocketAddress::parse(std::string addressWithPort) {
  static_assert(sizeof(data) >= sizeof(sockaddr_in),
                "Address buffer is not large enough on your system.");

  auto addr = reinterpret_cast<sockaddr_in *>(data);
  auto split = addressWithPort.find(':');

  if (split == std::string::npos) {
    return false;
  }

  auto ipStr = addressWithPort.substr(0, split);
  auto portStr =
      addressWithPort.substr(split + 1, addressWithPort.size() - split - 1);

  auto port = atoi(portStr.c_str());
  if ((port < 0) || (port > 0xFFFF)) {
    return false;
  }

  memset(addr, 0, sizeof(sockaddr_in));
  addr->sin_family = AF_INET;
  addr->sin_port = htons(static_cast<uint16_t>(port));
  addr->sin_addr.s_addr = inet_addr(ipStr.c_str());

  return true;
}

bool Socket::isValid() const { return handle != INVALID_SOCKET; }

Socket::Socket(ESocketProtocol _protocol)
    : handle(INVALID_SOCKET), protocol(_protocol) {
  static bool isInitialized = false;

  if (!isInitialized) {
    isInitialized = true;

#ifdef _MSC_VER
    WSADATA wsaData = {};
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
  }

  create();
}

Socket::Socket(ESocketProtocol _protocol, int64_t _socket)
  : handle(_socket), protocol(_protocol) {
}

void Socket::create() {
  switch (protocol) {
    case ESocketProtocol::Tcp:
      handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      break;

    case ESocketProtocol::Udp:
      handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      break;
  }
}

Socket::~Socket() { close(); }

bool Socket::listen() {
  return ::listen(static_cast<SOCKET>(handle), SOMAXCONN) == 0;
}

Socket Socket::accept() {
  sockaddr_in addr = {};
  socklen_t addrSize = sizeof(addr);
  return Socket(ESocketProtocol::Tcp, ::accept(static_cast<SOCKET>(handle), reinterpret_cast<sockaddr *>(&addr), &addrSize));
}

ssize_t Socket::recv(void *buffer, size_t bufferSize) {
  return ::recv(static_cast<SOCKET>(handle), reinterpret_cast<char *>(buffer),
                bufferSize, 0);
}

ssize_t Socket::recvfrom(void *buffer, size_t bufferSize,
                         SocketAddress &remoteAddress) {
  auto addr = reinterpret_cast<sockaddr_in *>(remoteAddress.data);
  socklen_t addrSize = sizeof(sockaddr_in);

  return ::recvfrom(static_cast<SOCKET>(handle),
                    reinterpret_cast<char *>(buffer), bufferSize, 0,
                    reinterpret_cast<sockaddr *>(addr), &addrSize);
}

ssize_t Socket::send(const void *data, size_t dataSize) {
  return ::send(static_cast<SOCKET>(handle),
                reinterpret_cast<const char *>(data), dataSize, 0);
}

ssize_t Socket::sendto(const void *data, size_t dataSize,
                       SocketAddress remoteAddress) {
  auto addr = reinterpret_cast<sockaddr_in *>(remoteAddress.data);
  auto addrSize = sizeof(sockaddr_in);

  return ::sendto(static_cast<SOCKET>(handle),
                  reinterpret_cast<const char *>(data), dataSize, 0,
                  reinterpret_cast<sockaddr *>(addr),
                  static_cast<socklen_t>(addrSize));
}

bool Socket::connect(SocketAddress remoteAddress) {
  return ::connect(static_cast<SOCKET>(handle),
                   reinterpret_cast<sockaddr *>(remoteAddress.data),
                   sizeof(sockaddr_in)) == 0;
}

bool Socket::bind(SocketAddress remoteAddress) {
  return ::bind(static_cast<SOCKET>(handle),
                reinterpret_cast<sockaddr *>(remoteAddress.data),
                sizeof(sockaddr_in)) == 0;
}

void Socket::close() {
  if (handle != INVALID_SOCKET) {
    closesocket(static_cast<int>(handle));
    handle = INVALID_SOCKET;
  }
}
}  // namespace DirectRemote
