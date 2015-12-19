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

#ifndef HOSTNETWORKABSTRACTION_H
#define HOSTNETWORKABSTRACTION_H

#include "IHostProtocol.h"
#include "IPerformanceMonitor.h"
#include "IResponseListener.h"
#include "UdpChunk.h"
#include "InputInject.h"
#include "ProgramOptions.h"

#include <map>

namespace DirectRemote {
class HostNetworkAbstraction : public IResponseListener {
 private:
  struct PendingPacket {
    int32_t controlId;
    PerformanceMonitor perfMon;
  };

  struct ConnectionMetrics {
    int64_t lostFrames = 0;
  };

  ConnectionMetrics metrics;
  std::shared_ptr<HostProtocol> conn;
  InputInject inputInjector;
  int32_t localControlId = 0;
  int32_t remoteControlId = 0;
  EncodedVideoPacket *controlPacket = nullptr;
  std::map<int64_t, PendingPacket> pendingPackets;
  std::recursive_mutex mutex;
  std::vector<PerformanceMonitor> accumulator;
  int32_t viewerId;
  int32_t screenWidth = 0, screenHeight = 0;
  float mouseX = 0, mouseY = 0;
  ProgramOptions options;

  void onMouseAbsolute(float x, float y) override;

  void onMouseRelative(float deltaX, float deltaY) override;

  void onButtonEvent(int8_t sourceType, bool isPressed, int8_t buttonId,
                     int32_t unicodeChar) override;

  void onAxisEvent(int8_t sourceType, int8_t axisId, float value) override;

  void onProfilingEvent(PerformanceMonitor profiling) override;

  void accumulateMetric(PerformanceMonitor perfMon);

  void cleanupPendingPackets();

 public:
  HostNetworkAbstraction(ProgramOptions options);

  int32_t getViewerId();

  std::vector<PerformanceMonitor> getAndResetAccumulatedMetrics();

  void disconnect();

  bool isConnected();

  bool connect(int64_t sessionId);

  void sendVideoFrame(EncodedVideoPacket *packet, PerformanceMonitor perfMon);

  void sendAudioFrame(EncodedAudioPacket *packet);
};
}

#endif
