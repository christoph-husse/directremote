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

#include "HostNetworkAbstraction.h"

using namespace DirectRemote;

void HostNetworkAbstraction::onMouseAbsolute(float x, float y) {
  mouseX = x;
  mouseY = y;
  if (options.enableInput()) {
    inputInjector.injectMouseMotion(x * screenWidth, y * screenHeight, 0, 0);
  }
}

void HostNetworkAbstraction::onMouseRelative(float deltaX, float deltaY) {
  if (options.enableInput()) {
    inputInjector.injectMouseMotion(mouseX * screenWidth, mouseY * screenHeight,
                                    deltaX, deltaY);
  }
}

void HostNetworkAbstraction::onButtonEvent(int8_t sourceType, bool isPressed,
                                           int8_t buttonId,
                                           int32_t unicodeChar) {
  if (options.enableInput()) {
    inputInjector.injectButton(sourceType, isPressed, buttonId, unicodeChar);
  }
}

void HostNetworkAbstraction::onAxisEvent(int8_t sourceType, int8_t axisId,
                                         float value) {
  if (options.enableInput()) {
    inputInjector.injectAxis(sourceType, axisId, value);
  }
}

void HostNetworkAbstraction::onProfilingEvent(PerformanceMonitor profiling) {
  cleanupPendingPackets();

  viewerId = 0;  // conn.getClientId();

  auto trackingId = profiling.trackingId();
  auto it = pendingPackets.find(trackingId);
  if (it == pendingPackets.end()) {
    return;
  }

  auto pending = it->second;
  pendingPackets.erase(trackingId);

  accumulateMetric(profiling);

  pending.perfMon.recordStackedTime(EPerfMetric::TimeNetworkRoundtrip);
  accumulateMetric(pending.perfMon);

  if (localControlId == pending.controlId) {
    if (controlPacket) {
      free(controlPacket);
      controlPacket = nullptr;
    }
  }
}

void HostNetworkAbstraction::accumulateMetric(PerformanceMonitor perfMon) {
  std::lock_guard<std::recursive_mutex> lock(mutex);

  perfMon.recordCounter(EPerfMetric::HostLostFrames, metrics.lostFrames);

  metrics = ConnectionMetrics();

  while (accumulator.size() > 1000) {
    accumulator.erase(accumulator.begin());
  }
  accumulator.push_back(perfMon);
}

void HostNetworkAbstraction::cleanupPendingPackets() {
  auto it = pendingPackets.begin();

  while (it != pendingPackets.end()) {
    if (it->second.perfMon.totalElapsedTime() < 1) {
      break;
    }

    metrics.lostFrames++;

    accumulateMetric(it->second.perfMon);

    it++;
  }

  pendingPackets.erase(pendingPackets.begin(), it);
}

HostNetworkAbstraction::HostNetworkAbstraction(ProgramOptions options)
    : options(options) {}

int32_t HostNetworkAbstraction::getViewerId() { return viewerId; }

std::vector<PerformanceMonitor>
HostNetworkAbstraction::getAndResetAccumulatedMetrics() {
  std::lock_guard<std::recursive_mutex> lock(mutex);

  return std::move(accumulator);
}

void HostNetworkAbstraction::disconnect() {
  std::lock_guard<std::recursive_mutex> lock(mutex);

  conn = nullptr;
}

bool HostNetworkAbstraction::isConnected() { return conn->isConnected(); }

bool HostNetworkAbstraction::connect(int64_t sessionId) {
  disconnect();

  std::lock_guard<std::recursive_mutex> lock(mutex);

  auto tmpConn = HostProtocol::create(options.protocolPlugin());

  if (!tmpConn || !tmpConn->connect(options.protocolString(), sessionId)) {
    return false;
  }

  conn = tmpConn;
  conn->setResponseListener(ResponseListener::wrapTakeOwnership(
      std::unique_ptr<ResponseListenerAdapter>(
          new ResponseListenerAdapter(this))));

  return true;
}

void HostNetworkAbstraction::sendAudioFrame(EncodedAudioPacket *packet) {
  conn->sendAudioFrame(packet);
}

void HostNetworkAbstraction::sendVideoFrame(EncodedVideoPacket *packet,
                                            PerformanceMonitor perfMon) {
  std::lock_guard<std::recursive_mutex> lock(mutex);

  packet->trackingId = perfMon.trackingId();
  packet->isMouseCaptured = inputInjector.isMouseCaptured();
  screenWidth = packet->width;
  screenHeight = packet->height;

  if (packet->mustDeliver) {
    localControlId = remoteControlId + 1;

    if (controlPacket) {
      free(controlPacket);
    }

    // build a control packet to initiate state transition
    int packetSize = sizeof(EncodedVideoPacket) + packet->payloadSize;
    controlPacket = (EncodedVideoPacket *)malloc(packetSize);
    memcpy(controlPacket, packet, packetSize);
  }

  perfMon.recordCounter(EPerfMetric::EncodedDatarate,
                        packet->payloadSize + sizeof(EncodedVideoPacket));

  if (controlPacket) {
    packet = controlPacket;
  } else {
    localControlId++;
  }

  PendingPacket pending = {};
  pending.controlId = localControlId;
  pending.perfMon = perfMon;
  pendingPackets[perfMon.trackingId()] = pending;

  cleanupPendingPackets();

  conn->sendVideoFrame(packet);
}
