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

#ifndef SOCKET_H
#define SOCKET_H

#include "Framework.h"
#include <stdint.h>
#include <string>

namespace DirectRemote {

enum class ESocketProtocol {
  Tcp,
  Udp,
};

class SocketAddress {
 private:
  friend class Socket;
  unsigned char data[30];

 public:
  SocketAddress();

  bool parse(std::string addressWithPort);
  std::string ipAddress() const;
  int32_t port() const;

  bool equalTo(const SocketAddress &other) const {
    return memcmp(other.data, data, sizeof(data)) == 0;
  }
};

class Socket {
 private:
  int64_t handle;
  ESocketProtocol protocol;

  Socket(ESocketProtocol _protocol, int64_t _socket);
 public:
  explicit Socket(ESocketProtocol protocol);
  ~Socket();

  bool listen();
  ssize_t recv(void *buffer, size_t bufferSize);
  ssize_t recvfrom(void *buffer, size_t bufferSize,
                   SocketAddress &remoteAddress);

  ssize_t send(const void *data, size_t dataSize);
  ssize_t sendto(const void *data, size_t dataSize,
                 SocketAddress remoteAddress);

  bool isValid() const;
  bool connect(SocketAddress remoteAddress);
  bool bind(SocketAddress remoteAddress);
  Socket accept();
  void close();
  void create();
};
}  // namespace DirectRemote

#endif
