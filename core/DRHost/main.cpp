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

#include "ApplicationMain.h"
#include "IVideoDecoder.h"
#include "IVideoEncoder.h"
#include "IUserInterface.h"
#include "IDesktopMirror.h"
#include "ViewerApi.h"
#include "SingleInstanceLock.h"
#include "ProgramOptions.h"
#include "ShowConsole.h"
#include "ILogger.h"
#include "HostNetworkAbstraction.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <array>
#include <map>

#undef min
#undef max

using namespace DirectRemote;

#ifdef _MSC_VER
#pragma comment(lib, "Imm32.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "Version.lib")
#endif

APPLICATION_MAIN() {
  if (!tryAcquireSingleInstanceLock(
          "DirectRemote-1d25e93d-4b27-4859-85ba-cd2aa5e18e55")) {
    DR_LOG_FATAL(
        "There is already one instance of DRHost running on this "
        "system. Aborting...");
    return -1;
  }

  ProgramOptions options;
  if (!options.parse(cmdArgs, true)) {
    return -1;
  }

  if (options.showConsole()) {
    showConsole();
  }

  DR_LOG_INFO("Running host...");

  auto mirror = DesktopMirror::create();
  auto encoder = VideoEncoder::create();
  auto ui = UserInterface::create();
  HostNetworkAbstraction protocol(options);
  int32_t currentViewerId = 0;
  EncoderParameters encParams = {};

  encParams.gopLength = options.keyFrameDistance();
  encParams.targetBitrateInKbps = options.targetBitrateKbps();

  if (!protocol.connect(options.sessionId())) {
    return -1;
  }

  encoder->setOutputHandler(
      [&](EncodedVideoPacket *packet, PerformanceMonitor perfMon) {

        protocol.sendVideoFrame(packet, perfMon);

        EncodedAudioPacket *p =
            (EncodedAudioPacket *)malloc(sizeof(EncodedAudioPacket));
        memset(p, 0, sizeof(EncodedAudioPacket));
        protocol.sendAudioFrame(p);
      });

  mirror->setCaptureHandler(
      [&](AbstractTexture screen, PerformanceMonitor perfMon) {
        if (protocol.getViewerId() != currentViewerId) {
          currentViewerId = protocol.getViewerId();
          encoder->initializeByFrame(screen, encParams);
        }

        encoder->encodeFrame(screen, perfMon);
      });

  int64_t frameIndex = 0;
  while (protocol.isConnected()) {
    double frameSeconds;
    PerformanceMonitor frameTimer(0);
    PerformanceMonitor perfMon(frameIndex++);

    perfMon.recordCounter(EPerfMetric::CaptureFps, 1);

    mirror->capture(perfMon);

    for (auto &e : protocol.getAndResetAccumulatedMetrics()) {
      ui->updateMetrics(e);
    }

    frameSeconds = frameTimer.totalElapsedTime();

    std::this_thread::sleep_for(std::chrono::milliseconds(
        std::max(0, (int)(33 - frameSeconds * 1000))));
  }

  if (!protocol.isConnected()) {
    DR_LOG_ERROR("Lost connection to peer. Shutting down.");
  }

  return 0;
}
