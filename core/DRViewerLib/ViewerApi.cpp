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

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include "ViewerApi.h"

#include "IVideoDecoder.h"
#include "IVideoEncoder.h"
#include "IDesktopMirror.h"
#include "IViewerProtocol.h"
#include "ApplicationMain.h"
#include "IPerformanceMonitor.h"
#include "ViewerResponseBuilder.h"
#include "SingleInstanceLock.h"
#include "ProgramOptions.h"
#include "ILogger.h"
#include "ShowConsole.h"

#include <thread>
#include <future>
#include <mutex>
#include <random>
#include <chrono>
#include <queue>
#include <algorithm>
#include <condition_variable>

#include <stdlib.h>

namespace DirectRemote {

struct ReceivedFrame {
  PerformanceMonitor perfMon;
  AbstractTexture texture;
};

struct ViewerImpl {
  std::mutex mutex;
  ProgramOptions options;
  std::shared_ptr<ReceivedFrame> readyFrame;
  std::condition_variable mayHaveNewFrame;
  std::condition_variable processedFrame;
  ViewerResponseEncoder packetBuilder;
  DOnBackbufferInfo onBackBufferInfo;
  DOnAudioSamples onAudioSamples;
  int64_t frameIndex = 0;
  std::unique_ptr<std::future<void>> backchannelSend;
  std::shared_ptr<ViewerProtocol> protocol;
  std::shared_ptr<VideoDecoder> decoder;
  AbstractTexture backBuffer = {};
  void *userContext;
  MouseInfo mouseInfo = {};

  ~ViewerImpl() {
    if (protocol) {
      protocol->disconnect();
    }

    protocol.reset();
    decoder.reset();
  }

  bool initialize(int argc, const char *const *argv) {
    std::srand(std::chrono::steady_clock::now().time_since_epoch().count());
    options.parse(argc, argv);

    if (options.showConsole()) {
      showConsole();
    }

    DR_LOG_INFO("Running viewer...");

    decoder = VideoDecoder::create();
    protocol = ViewerProtocol::create(options.protocolPlugin());

    if (!decoder || !protocol) {
      return false;
    }

    if (!protocol->connect(options.protocolString(), options.sessionId())) {
      return false;
    }

    if (!tryAcquireSingleInstanceLock(
            "DirectRemote-2ac33a94-f51d-4ad6-b4f8-399f88e1e15e")) {
      DR_LOG_ERROR(
          "There is already one instance of DRViewer running "
          "on this system.");
      return false;
    }

    decoder->setOutputHandler(
        [&](AbstractTexture frame, PerformanceMonitor perfMon) {
          std::unique_lock<std::mutex> lock(mutex);

          readyFrame = std::make_shared<ReceivedFrame>();
          readyFrame->perfMon = perfMon;
          readyFrame->texture = frame;

          mayHaveNewFrame.notify_all();

          processedFrame.wait_for(lock, std::chrono::milliseconds(30));

          readyFrame = nullptr;
        });

    protocol->setAudioHandler([&](const EncodedAudioPacket *packet) {
      if (onAudioSamples) {
        // TODO: translate to PCM samples
        uint16_t pcmSamples[512];

        memset(pcmSamples, 0, sizeof(pcmSamples));

        onAudioSamples(pcmSamples, sizeof(pcmSamples) / sizeof(pcmSamples[0]),
                       44100, 2, userContext);
      }
    });

    protocol->setVideoHandler([&](const EncodedVideoPacket *packet) {
      mouseInfo.isCaptured = packet->isMouseCaptured;
      decoder->decodePacket(packet, backBuffer,
                            PerformanceMonitor(packet->trackingId));
    });

    return true;
  }

  bool update() {
    PerformanceMonitor perfMon;
    bool hasFrame = false;

    {
      std::unique_lock<std::mutex> lock(mutex);

      mayHaveNewFrame.wait_for(lock, std::chrono::milliseconds(10),
                               [&]() { return !!readyFrame; });

      if (readyFrame) {
        auto tex = readyFrame->texture;
        perfMon = readyFrame->perfMon;
        hasFrame = true;

        if (onBackBufferInfo) {
          onBackBufferInfo(tex.width, tex.height, userContext);
        }

        processedFrame.notify_all();
      }
    }

    if (hasFrame) {
      perfMon.recordStackedTime(EPerfMetric::TimeRendering);
      perfMon.recordCounter(EPerfMetric::ViewerFps, 1);

      packetBuilder.trackProfiling(perfMon);
    }

    std::vector<UdpPayloadChunk> responseChunks;
    packetBuilder.toPackets(responseChunks);

    if (!backchannelSend ||
        backchannelSend->wait_for(std::chrono::milliseconds(1)) ==
            std::future_status::ready) {
      backchannelSend.reset(
          new std::future<void>(std::async(std::launch::async, [=]() {

            for (auto packet : responseChunks) {
              protocol->sendToHost(&packet, sizeof(packet), -1);
            }
          })));
    }

    if (!protocol->isConnected()) {
      DR_LOG_ERROR("Lost connection to peer.");
    }

    return protocol->isConnected();
  }

  void setBackbuffer(void *pixelsRgba32, int32_t width, int32_t height) {
    backBuffer.data = pixelsRgba32;
    backBuffer.width = width;
    backBuffer.height = height;
  }
};

DECLSPEC_EXPORT void viewerGetMouseInfo(PViewer viewer, MouseInfo *outMouse) {
  *outMouse = viewer->mouseInfo;
}

DECLSPEC_EXPORT void viewerConfigureBackbuffer(PViewer viewer,
                                               void *pixelsRgba32,
                                               int32_t width, int32_t height) {
  viewer->setBackbuffer(pixelsRgba32, width, height);
}

DECLSPEC_EXPORT void viewerTrackMouseAbsolute(PViewer viewer, float x,
                                              float y) {
  viewer->packetBuilder.trackMouseAbsolute(x, y);
}

DECLSPEC_EXPORT void viewerTrackMouseRelative(PViewer viewer, float deltaX,
                                              float deltaY) {
  viewer->packetBuilder.trackMouseRelative(deltaX, deltaY);
}

DECLSPEC_EXPORT void viewerTrackButton(PViewer viewer, int8_t deviceId,
                                       bool isPressed, int8_t buttonId,
                                       int32_t unicodeChar) {
  viewer->packetBuilder.trackButton(deviceId, isPressed, buttonId, unicodeChar);
}

DECLSPEC_EXPORT void viewerTrackAxis(PViewer viewer, int8_t deviceId,
                                     int8_t axisId, float value) {
  viewer->packetBuilder.trackAxis(deviceId, axisId, value);
}

DECLSPEC_EXPORT PViewer viewerCreate(int argc, const char *const *argv,
                                     DOnBackbufferInfo onBackBufferInfo,
                                     DOnAudioSamples onAudioSamples,
                                     void *userContext) {
  PViewer viewer = new ViewerImpl;
  viewer->onBackBufferInfo = onBackBufferInfo;
  viewer->onAudioSamples = onAudioSamples;
  viewer->userContext = userContext;

  if (!viewer->initialize(argc, argv)) {
    delete viewer;
    return nullptr;
  }

  return viewer;
}

DECLSPEC_EXPORT bool viewerUpdate(PViewer viewer) {
  if (!viewer) {
    return false;
  }

  return viewer->update();
}

DECLSPEC_EXPORT void viewerRelease(PViewer viewer) {
  if (viewer) {
    delete viewer;
  }
}
}  // namespace DirectRemote