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

#ifndef IPERFORMANCEMONITOR_H
#define IPERFORMANCEMONITOR_H

#include <stdint.h>
#include "Framework.h"
#include <stdlib.h>

#ifdef __cplusplus
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace DirectRemote {
extern "C" {
#endif

#pragma pack(1)

struct EPerfMetric {
  enum Type {
    Min,

    RequestedDatarate,

    EncodedDatarate,
    CaptureFps,

    ViewerFps,

    HostLostFrames,

    ViewerLostPackets,
    ViewerLostFrames,
    ViewerOutOfOrderFrames,
    ViewerIncomingPackets,
    ViewerValidPackets,
    ViewerInvalidPackets,
    ViewerDuplicatePackets,
    ViewerUnableToDecodeFrame,
    ViewerInsufficentFrameData,

    CountersEnd,

    CaptureFrameDelta,
    ViewerFrameDelta,
    CpuUsage,
    GopLength,

    TimeReconfigureEncoder,
    TimeReconfigureCapture,

    TimeScreenCapture,
    TimeImportToEncoder,
    TimeEncoderPreprocessing,
    TimeEncoding,

    TimeNetworkRoundtrip,

    // coming from remote end
    TimeReconfigureDecoder,

    TimeDecoding,
    TimeExportFromDecoder,
    TimeRendering,
    TimePresented,

    Max,  // MUST BE LAST MEMBER
  };

  Type unused;
};

#ifdef __cplusplus
namespace internal {
#endif

struct IPerformanceMonitor {
  void* __context;  // for internal use only

  void (*release)(struct IPerformanceMonitor* perfMon);

  struct IPerformanceMonitor* (*duplicate)(
      const struct IPerformanceMonitor* perfMon);

  void (*recordStackedTime)(struct IPerformanceMonitor* perfMon,
                            enum EPerfMetric::Type metric);
  void (*recordAbsoluteTime)(struct IPerformanceMonitor* perfMon,
                             enum EPerfMetric::Type metric);
  void (*recordCounter)(struct IPerformanceMonitor* perfMon,
                        enum EPerfMetric::Type metric, double value);

  bool (*hasRecord)(struct IPerformanceMonitor* perfMon,
                    enum EPerfMetric::Type metric);

  void (*recordRaw)(struct IPerformanceMonitor* perfMon,
                    enum EPerfMetric::Type metric, double value);

  int64_t (*trackingId)(const struct IPerformanceMonitor* perfMon);
  double (*totalElapsedTime)(const struct IPerformanceMonitor* perfMon);
  double (*query)(const struct IPerformanceMonitor* perfMon,
                  enum EPerfMetric::Type metric);

  const char* (*metricToString)(EPerfMetric::Type metricType);
  void (*assignFrom)(struct IPerformanceMonitor* perfMon,
                     struct IPerformanceMonitor* source);
};

DECLSPEC_EXPORT struct IPerformanceMonitor* createPerformanceMonitor(
    int64_t trackingId);

#ifdef __cplusplus
}  // namespace internal
}
#endif

#pragma pack()

#ifdef __cplusplus

class PerformanceMonitor final {
 private:
  internal::IPerformanceMonitor* impl = nullptr;
  bool dontRelease = false;

 public:
  explicit PerformanceMonitor(int64_t trackingId = -1) {
    impl = internal::createPerformanceMonitor(trackingId);
  }

  static PerformanceMonitor wrapNoOwnership(
      internal::IPerformanceMonitor* perfMon) {
    if (perfMon) {
      PerformanceMonitor result;
      result.impl = perfMon;
      result.dontRelease = true;
      return result;
    }
    return PerformanceMonitor();
  }

  ~PerformanceMonitor() {
    if (impl && !dontRelease) {
      impl->release(impl);
      impl = nullptr;
    }
  }

  PerformanceMonitor(const PerformanceMonitor& src) {
    impl = src.impl->duplicate(src.impl);
  }

  PerformanceMonitor(PerformanceMonitor&& src) {
    std::swap(impl, src.impl);
    std::swap(dontRelease, src.dontRelease);
  }

  PerformanceMonitor& operator=(const PerformanceMonitor& src) {
    if (&src != this) {
      if (impl && !dontRelease) {
        impl->release(impl);
        impl = nullptr;
      }

      impl = src.impl->duplicate(src.impl);
    }

    return *this;
  }

  PerformanceMonitor& operator=(PerformanceMonitor&& src) {
    std::swap(impl, src.impl);
    std::swap(dontRelease, src.dontRelease);
    return *this;
  }

  int64_t trackingId() { return impl->trackingId(impl); }

  void recordStackedTime(EPerfMetric::Type metric) {
    impl->recordStackedTime(impl, metric);
  }

  void recordAbsoluteTime(EPerfMetric::Type metric) {
    impl->recordAbsoluteTime(impl, metric);
  }

  void recordCounter(EPerfMetric::Type metric, double value) {
    impl->recordCounter(impl, metric, value);
  }

  void recordRaw(EPerfMetric::Type metric, double value) {
    impl->recordRaw(impl, metric, value);
  }

  double totalElapsedTime() { return impl->totalElapsedTime(impl); }

  double query(EPerfMetric::Type metric) { return impl->query(impl, metric); }

  bool hasRecord(EPerfMetric::Type metric) {
    return impl->hasRecord(impl, metric);
  }

  static std::string metricToString(EPerfMetric::Type metric) {
    static auto impl = internal::createPerformanceMonitor(0);
    return impl->metricToString(metric);
  }

  void assignFrom(const PerformanceMonitor& source) {
    impl->assignFrom(impl, source.impl);
  }

  internal::IPerformanceMonitor* toInterface() { return impl; }
};
}  // namespace DirectRemote
#endif

#endif  // IPERFORMANCEMONITOR_H
