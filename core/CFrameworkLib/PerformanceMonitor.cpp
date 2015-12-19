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

#include "IPerformanceMonitor.h"

#include <chrono>
#include <cmath>
#include <stddef.h>
#include <vector>
#include <unordered_map>
#include <array>
#include <random>
#include <algorithm>

namespace DirectRemote {

#ifdef _MSC_VER
#include <Windows.h>

static int64_t queryPerformanceCounter() {
  int64_t time;
  QueryPerformanceCounter((LARGE_INTEGER*)&time);
  return time;
}

static int64_t queryPerformanceFrequency() {
  int64_t freq;
  QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
  return freq;
}

#elif defined(__MACH__)

#include <mach/mach_time.h>

// This function returns the rational number inside the given interval with
// the smallest denominator (and smallest numerator breaks ties; correctness
// proof neglects floating-point errors).
static mach_timebase_info_data_t bestFrac(double a, double b) {
  if (floor(a) < floor(b)) {
    mach_timebase_info_data_t rv = {};

    rv.numer = (int)ceil(a);
    rv.denom = 1;

    return rv;
  }

  double m = floor(a);
  mach_timebase_info_data_t next = bestFrac(1 / (b - m), 1 / (a - m));
  mach_timebase_info_data_t rv = {};
  rv.numer = (int)m * next.numer + next.denom;
  rv.denom = next.numer;

  return rv;
}

static uint64_t getExpressibleSpan(uint32_t numer, uint32_t denom) {
  // This is just less than the smallest thing we can multiply numer by
  // without
  // overflowing. ceilLog2(numer) = 64 - number of leading zeros of numer
  uint64_t maxDiffWithoutOverflow =
      ((uint64_t)1 << (64 - (uint64_t)ceil(log2(numer)))) - 1;
  return maxDiffWithoutOverflow * numer / denom;
}

// Returns monotonic time in nanos, measured from the first time the function
// is called in the process.  The clock may run up to 0.1% faster or slower
// than the "exact" tick count. However, although the bound on the error is
// the same as for the pragmatic answer, the error is actually minimized over
// the given accuracy bound.
static int64_t queryPerformanceCounter() {
  uint64_t now = mach_absolute_time();
  static struct Data {
    mach_timebase_info_data_t tb;
    uint64_t bias;

    Data(uint64_t bias_) : bias(bias_) {
      mach_timebase_info(&tb);
      double frac = (double)tb.numer / tb.denom;
      uint64_t spanTarget = 315360000000000000llu;  // 10 years
      if (getExpressibleSpan(tb.numer, tb.denom) >= spanTarget) {
        return;
      }

      for (double errorTarget = 1 / 1024.0; errorTarget > 0.000001;) {
        mach_timebase_info_data_t newFrac =
            bestFrac((1 - errorTarget) * frac, (1 + errorTarget) * frac);
        if (getExpressibleSpan(newFrac.numer, newFrac.denom) < spanTarget) {
          break;
        }

        tb = newFrac;
        errorTarget = fabs((double)tb.numer / tb.denom - frac) / frac / 8;
      }
    }
  } data(now);

  return (now - data.bias) * data.tb.numer / data.tb.denom;
}

static int64_t queryPerformanceFrequency() { return 1000LL * 1000LL * 1000LL; }

#else  // assuming linux compatible

static int64_t queryPerformanceCounter() {
  timespec ts = {};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec) * 1000LL * 1000LL * 1000LL + (ts.tv_nsec);
}

static int64_t queryPerformanceFrequency() {
  timespec ts = {};
  clock_getres(CLOCK_MONOTONIC, &ts);
  return 1000LL * 1000LL * 1000LL / ts.tv_nsec;
}

#endif

class PerformanceMonitorExport final {
 private:
  internal::IPerformanceMonitor iface;

  struct MetricValue {
    double value;
  };

  std::unordered_map<int, MetricValue> values;
  int64_t creationTime;
  int64_t startTime;
  int64_t tid;

  static PerformanceMonitorExport* get(
      const internal::IPerformanceMonitor* _this) {
    return reinterpret_cast<PerformanceMonitorExport*>(_this->__context);
  }

  static void _release(struct internal::IPerformanceMonitor* _this) {
    if (_this) {
      delete get(_this);
    }
  }

  static struct internal::IPerformanceMonitor* _duplicate(
      const struct internal::IPerformanceMonitor* _this) {
    auto result = new (std::nothrow) PerformanceMonitorExport(*get(_this));
    if (result) {
      return result->toInterface();
    }

    return nullptr;
  }

  static void _recordStackedTime(struct internal::IPerformanceMonitor* _this,
                                 enum EPerfMetric::Type metric) {
    get(_this)->recordStackedTime(metric);
  }

  static void _recordAbsoluteTime(struct internal::IPerformanceMonitor* _this,
                                  enum EPerfMetric::Type metric) {
    get(_this)->recordAbsoluteTime(metric);
  }

  static void _recordCounter(struct internal::IPerformanceMonitor* _this,
                             enum EPerfMetric::Type metric, double value) {
    get(_this)->recordCounter(metric, value);
  }

  static bool _hasRecord(struct internal::IPerformanceMonitor* _this,
                         enum EPerfMetric::Type metric) {
    return get(_this)->hasRecord(metric);
  }

  static void _recordRaw(struct internal::IPerformanceMonitor* _this,
                         enum EPerfMetric::Type metric, double value) {
    get(_this)->recordRaw(metric, value);
  }

  static double _totalElapsedTime(
      const struct internal::IPerformanceMonitor* _this) {
    return get(_this)->totalElapsedTime();
  }

  static double _query(const struct internal::IPerformanceMonitor* _this,
                       enum EPerfMetric::Type metric) {
    return get(_this)->query(metric);
  }

  static const char* _metricToString(EPerfMetric::Type metricType) {
    return PerformanceMonitorExport::metricToString(metricType);
  }

  static void _assignFrom(struct internal::IPerformanceMonitor* _this,
                          struct internal::IPerformanceMonitor* source) {
    get(_this)->assignFrom(get(source));
  }

  static int64_t _trackingId(
      const struct internal::IPerformanceMonitor* _this) {
    return get(_this)->trackingId();
  }

 public:
  virtual ~PerformanceMonitorExport() {}

  PerformanceMonitorExport(const PerformanceMonitorExport& src) {
    this->iface = src.iface;
    this->iface.__context = this;
    this->values = src.values;
    this->creationTime = src.creationTime;
    this->startTime = src.startTime;
    this->tid = src.tid;
  }

  PerformanceMonitorExport() {
    iface.assignFrom = _assignFrom;
    iface.duplicate = _duplicate;
    iface.hasRecord = _hasRecord;
    iface.metricToString = _metricToString;
    iface.query = _query;
    iface.recordAbsoluteTime = _recordAbsoluteTime;
    iface.recordCounter = _recordCounter;
    iface.recordRaw = _recordRaw;
    iface.trackingId = _trackingId;
    iface.recordStackedTime = _recordStackedTime;
    iface.release = _release;
    iface.totalElapsedTime = _totalElapsedTime;
  }

  void start(int64_t trackingId) {
    this->tid = trackingId;

    creationTime = queryPerformanceCounter();
    startTime = queryPerformanceCounter();
  }

  double totalElapsedTime() {
    int64_t freq, current;

    current = queryPerformanceCounter();
    freq = queryPerformanceFrequency();

    return (current - creationTime) / static_cast<double>(freq);
  }

  void recordAbsoluteTime(EPerfMetric::Type metric) {
    recordRaw(metric, totalElapsedTime());
  }

  int64_t trackingId() { return tid; }

  void recordStackedTime(EPerfMetric::Type metric) {
    int64_t freq, oldStart = startTime;

    startTime = queryPerformanceCounter();
    freq = queryPerformanceFrequency();

    double elapsed = (startTime - oldStart) / static_cast<double>(freq);
    recordRaw(metric, elapsed);
  }

  void recordCounter(EPerfMetric::Type metric, double value) {
    recordRaw(metric, value);
  }

  bool hasRecord(EPerfMetric::Type metric) {
    return values.find(metric) != values.end();
  }

  void recordRaw(EPerfMetric::Type metric, double seconds) {
    values[metric] = {seconds};
  }

  double query(EPerfMetric::Type metric) {
    auto it = values.find(metric);
    if (it == values.end()) {
      return 0;
    }
    return it->second.value;
  }

  void assignFrom(PerformanceMonitorExport* accumulated) {
    for (auto& e : accumulated->values) {
      values[e.first] = e.second;
    }
  }

  static const char* metricToString(EPerfMetric::Type metricType) {
    switch (metricType) {
      case EPerfMetric::Min:
        return "Min";
      case EPerfMetric::RequestedDatarate:
        return "RequestedDatarate";
      case EPerfMetric::EncodedDatarate:
        return "EncodedDatarate";
      case EPerfMetric::CaptureFps:
        return "CaptureFps";
      case EPerfMetric::ViewerFps:
        return "ViewerFps";
      case EPerfMetric::HostLostFrames:
        return "HostLostFrames";
      case EPerfMetric::ViewerLostPackets:
        return "ViewerLostPackets";
      case EPerfMetric::ViewerLostFrames:
        return "ViewerLostFrames";
      case EPerfMetric::ViewerOutOfOrderFrames:
        return "ViewerOutOfOrderFrames";
      case EPerfMetric::ViewerIncomingPackets:
        return "ViewerIncomingPackets";
      case EPerfMetric::ViewerValidPackets:
        return "ViewerValidPackets";
      case EPerfMetric::ViewerInvalidPackets:
        return "ViewerInvalidPackets";
      case EPerfMetric::ViewerDuplicatePackets:
        return "ViewerDuplicatePackets";
      case EPerfMetric::ViewerUnableToDecodeFrame:
        return "ViewerUnableToDecodeFrame";
      case EPerfMetric::ViewerInsufficentFrameData:
        return "ViewerInsufficentFrameData";
      case EPerfMetric::CountersEnd:
        return "CountersEnd";
      case EPerfMetric::CaptureFrameDelta:
        return "CaptureFrameDelta";
      case EPerfMetric::ViewerFrameDelta:
        return "ViewerFrameDelta";
      case EPerfMetric::CpuUsage:
        return "CpuUsage";
      case EPerfMetric::GopLength:
        return "GopLength";
      case EPerfMetric::TimeReconfigureEncoder:
        return "TimeReconfigureEncoder";
      case EPerfMetric::TimeReconfigureCapture:
        return "TimeReconfigureCapture";
      case EPerfMetric::TimeScreenCapture:
        return "TimeScreenCapture";
      case EPerfMetric::TimeImportToEncoder:
        return "TimeImportToEncoder";
      case EPerfMetric::TimeEncoderPreprocessing:
        return "TimeEncoderPreprocessing";
      case EPerfMetric::TimeEncoding:
        return "TimeEncoding";
      case EPerfMetric::TimeNetworkRoundtrip:
        return "TimeNetworkRoundtrip";

      // coming from remote end
      case EPerfMetric::TimeReconfigureDecoder:
        return "TimeReconfigureDecoder";
      case EPerfMetric::TimeDecoding:
        return "TimeDecoding";
      case EPerfMetric::TimeExportFromDecoder:
        return "TimeExportFromDecoder";
      case EPerfMetric::TimeRendering:
        return "TimeRendering";
      case EPerfMetric::TimePresented:
        return "TimePresented";
      case EPerfMetric::Max:
        return "Max";  // MUST BE LAST MEMBER
    }
  }

  internal::IPerformanceMonitor* toInterface() {
    iface.__context = this;
    return &iface;
  }
};

extern "C" struct internal::IPerformanceMonitor* createPerformanceMonitor(
    int64_t trackingId) {
  auto result = new (std::nothrow) PerformanceMonitorExport();
  if (result) {
    result->start(trackingId);

    return result->toInterface();
  }

  return nullptr;
}
}  // namespace DirectRemote
