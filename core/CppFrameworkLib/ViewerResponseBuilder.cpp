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

#include "ViewerResponseBuilder.h"

#include <random>
#include <map>
#include <stack>
#include <algorithm>
#include <vector>

namespace DirectRemote {

#pragma pack(1)
struct ButtonPacket {
  int16_t inputId;
  int8_t deviceId;
  bool isPressed;
  int8_t buttonId;
  int32_t unicodeChar;
};

struct AxisPacket {
  int16_t inputId;
  int8_t deviceId;
  int8_t axisId;
  float value;
};

struct MetricEntry {
  int8_t metricId;
  float value;
};

struct ProfilingPacket {
  int64_t trackingId;
  int8_t metricIndex;
};

#define VIEWER_RESPONSE_MAGIC ((int64_t)0x40a18bb97919adf4LL)

struct ViewerReponsePacket {
  int64_t magic;
  int32_t clientId;
  float mouseX;
  float mouseY;
  float mouseDeltaX;
  float mouseDeltaY;
  ConnectionMetrics metrics;

  int8_t axisCount;
  AxisPacket axisValues[14];

  int8_t buttonCount;
  ButtonPacket buttonValues[10];

  int8_t profilingCount;
  ProfilingPacket profilingEntries[5];

  MetricEntry metricEntries[27];
};

#define MAX_METRIC_COUNT                                \
  ((int)((sizeof(ViewerReponsePacket().metricEntries) / \
          sizeof(ViewerReponsePacket().metricEntries[0]))))
#define MAX_AXIS_COUNT                              \
  ((int)(sizeof(ViewerReponsePacket().axisValues) / \
         sizeof(ViewerReponsePacket().axisValues[0])))
#define MAX_BUTTON_COUNT                              \
  ((int)(sizeof(ViewerReponsePacket().buttonValues) / \
         sizeof(ViewerReponsePacket().buttonValues[0])))
#define MAX_PROFILING_COUNT                               \
  ((int)(sizeof(ViewerReponsePacket().profilingEntries) / \
         sizeof(ViewerReponsePacket().profilingEntries[0])))

#pragma pack()

static_assert(sizeof(ViewerReponsePacket) <= sizeof(UdpPayloadChunk),
              "ViewerReponsePacket is too large.");

struct ViewerResponseEncoderImpl {
  int16_t uniquenessCounter = std::random_device()();
  int32_t clientId = std::random_device()();
  float mouseX = 0;
  float mouseY = 0;
  float mouseDeltaX = 0;
  float mouseDeltaY = 0;
  ConnectionMetrics metrics = {};
  int profilingMetricCount = 0;

  std::vector<ViewerReponsePacket> packetQueue;

  std::vector<PerformanceMonitor> profilingQueueHistory;
  std::vector<ButtonPacket> buttonQueueHistory;
  std::vector<AxisPacket> axisQueueHistory;

  std::vector<PerformanceMonitor> profilingQueue;
  std::vector<ButtonPacket> buttonQueue;
  std::vector<AxisPacket> axisQueue;
};

struct ViewerResponseDecoderImpl {
  int32_t clientId = std::random_device()();
  float mouseX = 0;
  float mouseY = 0;
  float mouseDeltaX = 0;
  float mouseDeltaY = 0;
  ConnectionMetrics metrics = {};

  std::stack<PerformanceMonitor> decodedProfiling;
  std::map<int32_t, bool> profilingMap;
  std::map<int32_t, bool> buttonMap;
  std::map<int32_t, bool> axisMap;
};

//////////////////////////////////////////////////////////////////////
// Encoder
//////////////////////////////////////////////////////////////////////

ViewerResponseEncoder::ViewerResponseEncoder()
    : pimpl(new ViewerResponseEncoderImpl()) {}

ViewerResponseEncoder::~ViewerResponseEncoder() {
  if (pimpl) {
    delete pimpl;
  }
}

int32_t ViewerResponseEncoder::getClientId() { return pimpl->clientId; }

void ViewerResponseEncoder::trackMouseAbsolute(float x, float y) {
  pimpl->mouseX = x;
  pimpl->mouseY = y;
}

template <class TQueue, class TTuple>
bool pushToQueue(TQueue &queue, TQueue &history, TTuple value, int maxCount) {
  queue.push_back(value);
  history.push_back(value);

  while (history.size() > maxCount) {
    history.erase(history.begin());
  }

  if (queue.size() >= maxCount) {
    return true;
  }

  return false;
}

void ViewerResponseEncoder::trackButton(int8_t sourceType, bool isPressed,
                                        int8_t buttonId, int32_t unicodeChar) {
  ButtonPacket p = {};
  p.inputId = pimpl->uniquenessCounter++;
  p.buttonId = buttonId;
  p.deviceId = sourceType;
  p.isPressed = isPressed;
  p.unicodeChar = unicodeChar;

  if (pushToQueue(pimpl->buttonQueue, pimpl->buttonQueueHistory, p,
                  MAX_BUTTON_COUNT)) {
    generatePacket();
  }
}

void ViewerResponseEncoder::trackAxis(int8_t sourceType, int8_t axisId,
                                      float value) {
  AxisPacket p = {};
  p.inputId = pimpl->uniquenessCounter++;
  p.axisId = axisId;
  p.deviceId = sourceType;
  p.value = value;

  if (pushToQueue(pimpl->axisQueue, pimpl->axisQueueHistory, p,
                  MAX_AXIS_COUNT)) {
    generatePacket();
  }
}

void ViewerResponseEncoder::trackProfiling(PerformanceMonitor profiling) {
  int metricCount = 0;
  for (int y = 0; y < EPerfMetric::Max; y++) {
    metricCount +=
        profiling.hasRecord(static_cast<EPerfMetric::Type>(y)) ? 1 : 0;
  }

  if (pimpl->profilingMetricCount + metricCount > MAX_METRIC_COUNT) {
    generatePacket();
  }

  pimpl->profilingMetricCount += metricCount;

  if (pushToQueue(pimpl->profilingQueue, pimpl->profilingQueueHistory,
                  profiling, MAX_PROFILING_COUNT)) {
    generatePacket();
  }
}

void ViewerResponseEncoder::trackMetrics(ConnectionMetrics metrics) {
  pimpl->metrics = metrics;
}

void ViewerResponseEncoder::toPackets(
    std::vector<UdpPayloadChunk> &outPackets) {
  generatePacket();

  for (auto e : pimpl->packetQueue) {
    UdpPayloadChunk chunk = {};
    memcpy(&chunk, &e, sizeof(e));

    outPackets.push_back(chunk);
  }

  while (pimpl->packetQueue.size() > 1) {
    pimpl->packetQueue.erase(pimpl->packetQueue.begin());
  }
}

void ViewerResponseEncoder::trackMouseRelative(float deltaX, float deltaY) {
  pimpl->mouseDeltaX += deltaX;
  pimpl->mouseDeltaY += deltaY;
}

void ViewerResponseEncoder::generatePacket() {
  ViewerReponsePacket p = {};

  p.magic = VIEWER_RESPONSE_MAGIC;
  p.clientId = pimpl->clientId;
  p.mouseX = pimpl->mouseX;
  p.mouseY = pimpl->mouseY;
  p.mouseDeltaX = pimpl->mouseDeltaX;
  p.mouseDeltaY = pimpl->mouseDeltaY;
  p.metrics = pimpl->metrics;

  pimpl->mouseDeltaX = 0;
  pimpl->mouseDeltaY = 0;
  pimpl->profilingMetricCount = 0;

  for (int i = 0;
       (i < pimpl->axisQueueHistory.size()) && (p.axisCount < MAX_AXIS_COUNT);
       i++) {
    p.axisValues[p.axisCount++] = pimpl->axisQueueHistory[i];
  }
  pimpl->axisQueue.clear();

  for (int i = 0; (i < pimpl->buttonQueueHistory.size()) &&
                  (p.buttonCount < MAX_BUTTON_COUNT);
       i++) {
    p.buttonValues[p.buttonCount++] = pimpl->buttonQueueHistory[i];
  }
  pimpl->buttonQueue.clear();

  int metricIndex = 0;
  for (int i = pimpl->profilingQueueHistory.size() - 1;
       (i >= 0) && (p.profilingCount < MAX_PROFILING_COUNT); i--) {
    auto &mon = pimpl->profilingQueueHistory[i];
    ProfilingPacket prof = {};
    prof.trackingId = mon.trackingId();

    bool hasSpace = true;

    for (int y = 0; y < EPerfMetric::Max; y++) {
      auto metric = static_cast<EPerfMetric::Type>(y);

      if (mon.hasRecord(metric)) {
        if (metricIndex < MAX_METRIC_COUNT) {
          MetricEntry m = {};

          m.metricId = y;
          m.value = mon.query(metric);

          p.metricEntries[metricIndex++] = m;
        } else {
          hasSpace = false;
          break;
        }
      }
    }

    if (hasSpace) {
      prof.metricIndex = metricIndex;
      p.profilingEntries[p.profilingCount++] = prof;
    } else {
      break;
    }
  }
  pimpl->profilingQueue.clear();

  pimpl->packetQueue.push_back(p);
}

//////////////////////////////////////////////////////////////////////
// Decoder
//////////////////////////////////////////////////////////////////////

ViewerResponseDecoder::~ViewerResponseDecoder() {
  if (pimpl) {
    delete pimpl;
  }
}

ViewerResponseDecoder::ViewerResponseDecoder()
    : pimpl(new ViewerResponseDecoderImpl) {}

void ViewerResponseDecoder::setResponseListener(
    std::unique_ptr<ResponseListener> listener) {
  this->listener = std::move(listener);
}

ConnectionMetrics ViewerResponseDecoder::getMetrics() { return pimpl->metrics; }

int32_t ViewerResponseDecoder::getClientId() { return pimpl->clientId; }

void ViewerResponseDecoder::parsePacket(UdpPayloadChunk packet) {
  ViewerReponsePacket p = *(reinterpret_cast<ViewerReponsePacket *>(&packet));

  if (p.magic != VIEWER_RESPONSE_MAGIC) {
    return;
  }

  pimpl->clientId = p.clientId;
  pimpl->mouseX = p.mouseX;
  pimpl->mouseY = p.mouseY;
  pimpl->mouseDeltaX = p.mouseDeltaX;
  pimpl->mouseDeltaY = p.mouseDeltaY;
  pimpl->metrics = p.metrics;

  if (listener) {
    listener->onMouseAbsolute(pimpl->mouseX, pimpl->mouseY);
    listener->onMouseRelative(pimpl->mouseDeltaX, pimpl->mouseDeltaY);
  }

  for (int i = 0;
       i < std::max(0, std::min(static_cast<int>(p.axisCount), MAX_AXIS_COUNT));
       i++) {
    auto axis = p.axisValues[i];

    if (pimpl->axisMap.find(axis.inputId) == pimpl->axisMap.end()) {
      pimpl->axisMap[axis.inputId] = true;

      if (listener) {
        listener->onAxisEvent(axis.deviceId, axis.axisId, axis.value);
      }
    }
  }

  for (int i = 0; i < std::max(0, std::min(static_cast<int>(p.buttonCount),
                                           MAX_BUTTON_COUNT));
       i++) {
    auto button = p.buttonValues[i];

    if (pimpl->buttonMap.find(button.inputId) == pimpl->buttonMap.end()) {
      pimpl->buttonMap[button.inputId] = true;

      if (listener) {
        listener->onButtonEvent(button.deviceId, button.isPressed,
                                button.buttonId, button.unicodeChar);
      }
    }
  }

  int iMetric = 0;
  for (int i = 0; i < std::max(0, std::min(static_cast<int>(p.profilingCount),
                                           MAX_PROFILING_COUNT));
       i++) {
    auto prof = p.profilingEntries[i];

    if (pimpl->profilingMap.find(prof.trackingId) ==
        pimpl->profilingMap.end()) {
      pimpl->profilingMap[prof.trackingId] = true;

      if (listener) {
        PerformanceMonitor perfMon(prof.trackingId);

        for (int y = iMetric;
             y < std::max(0, std::min(static_cast<int>(prof.metricIndex),
                                      MAX_METRIC_COUNT));
             y++, iMetric++) {
          auto m = p.metricEntries[iMetric];
          perfMon.recordRaw(static_cast<EPerfMetric::Type>(m.metricId),
                            m.value);
        }

        pimpl->decodedProfiling.push(perfMon);
      }
    }
  }

  while (!pimpl->decodedProfiling.empty()) {
    listener->onProfilingEvent(pimpl->decodedProfiling.top());
    pimpl->decodedProfiling.pop();
  }
}
}  // namespace DirectRemote
