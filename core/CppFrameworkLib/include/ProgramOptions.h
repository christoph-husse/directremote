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

#ifndef PROGRAMOPTIONS_H
#define PROGRAMOPTIONS_H

#include <map>
#include <memory>
#include <stdint.h>
#include <string.h>
#include <unordered_map>
#include <vector>

#include "MessageAssembly.h"
#include "UdpChunk.h"

namespace DirectRemote {

class ProgramOptions {
 private:
  std::string m_protocolPlugin;
  std::string m_protocolString;
  bool m_enableInput;
  bool m_showConsole;
  uint64_t m_sessionId;
  int32_t m_peerTimeout;
  int32_t m_keyFrameDistance;
  int32_t m_targetBitrateKbps;

 public:
  bool parse(int argc, const char *const *argv,
             bool printHelpOnFailure = false);

  bool parse(std::vector<std::string> cmdArgs, bool printHelpOnFailure = false);

  std::string protocolPlugin() const { return m_protocolPlugin; }
  std::string protocolString() const { return m_protocolString; }
  bool enableInput() const { return m_enableInput; }
  bool showConsole() const { return m_showConsole; }
  uint64_t sessionId() const { return m_sessionId; }
  int32_t peerTimeout() const { return m_peerTimeout; }
  int32_t keyFrameDistance() const { return m_keyFrameDistance; }
  int32_t targetBitrateKbps() const { return m_targetBitrateKbps; }
};
}  // namespace DirectRemote

#endif
