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

#ifndef VIEWERRESPONSEBUILDER_H
#define VIEWERRESPONSEBUILDER_H

#include "IPerformanceMonitor.h"
#include "UdpChunk.h"
#include "IResponseListener.h"

#include <stdint.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace DirectRemote {

class ViewerResponseEncoder final {
 private:
  struct ViewerResponseEncoderImpl *pimpl = nullptr;

  void generatePacket();

 public:
  ~ViewerResponseEncoder();
  ViewerResponseEncoder();

  int32_t getClientId();

  void trackMouseAbsolute(float x, float y);
  void trackMouseRelative(float deltaX, float deltaY);
  void trackButton(int8_t sourceType, bool isPressed, int8_t buttonId,
                   int32_t unicodeChar);
  void trackAxis(int8_t sourceType, int8_t axisId, float value);
  void trackProfiling(PerformanceMonitor profiling);
  void trackMetrics(ConnectionMetrics metrics);

  void toPackets(std::vector<UdpPayloadChunk> &outPackets);
};

class ViewerResponseDecoder final {
 private:
  struct ViewerResponseDecoderImpl *pimpl = nullptr;

  std::unique_ptr<ResponseListener> listener;

 public:
  ~ViewerResponseDecoder();
  ViewerResponseDecoder();

  ConnectionMetrics getMetrics();
  int32_t getClientId();
  void setResponseListener(std::unique_ptr<ResponseListener> listener);

  void parsePacket(UdpPayloadChunk packet);
};
}  // namespace DirectRemote

#endif