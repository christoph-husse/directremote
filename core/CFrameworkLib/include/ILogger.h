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

#ifndef _DIRECTREMOTE_ILOGGER_H_
#define _DIRECTREMOTE_ILOGGER_H_

#include <stdint.h>

#include "Framework.h"

#ifdef __cplusplus

#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include "DynamicPlugin.h"
#include "IPerformanceMonitor.h"

namespace DirectRemote {
extern "C" {

#else
#define EXTERN_C

#endif

#pragma pack(1)

#ifdef __cplusplus
namespace internal {
#endif

struct ILogger;

typedef void ILogger_log(struct ILogger* _this, const char* message);

typedef void ILogger_release(struct ILogger* _this);

struct ILogger {
  ILogger_release* release;

  ILogger_log* warning;
  ILogger_log* fatalError;
  ILogger_log* error;
  ILogger_log* info;
};

DECLSPEC_EXPORT struct ILogger* createLogger();

#ifdef __cplusplus
}
}
#endif

#pragma pack()

#ifdef __cplusplus

class Logger final {
 private:
  internal::ILogger* impl;

  Logger() {}

  void strcat(std::stringstream& ss) {}

  template <class TArg, class... TArgs>
  void strcat(std::stringstream& ss, TArg arg, TArgs... args) {
    ss << arg;
    strcat(ss, args...);
  }

 public:
  static std::shared_ptr<Logger> create(std::string loggers) {
    auto plugin = internal::createLogger();
    if (plugin) {
      auto result = new (std::nothrow) Logger();

      if (result) {
        result->impl = plugin;
        return std::shared_ptr<Logger>(result);
      } else {
        return nullptr;
      }
    } else {
      return nullptr;
    }
  }

  ~Logger() {
    if (impl) {
      impl->release(impl);
      impl = nullptr;
    }
  }

  template <class... TArgs>
  void swallow(TArgs... args) {
    // used to implement optimized, but safe, debug only logging
  }

  template <class... TArgs>
  std::string createMessage(std::string fileName, int line, TArgs... message) {
    std::stringstream ss;
    strcat(ss, message...);
    ss << " ('" << fileName << "' at line " << line << ")";
    return ss.str();
  }

  template <class... TArgs>
  std::string createMessageFormat(std::string fileName, int line,
                                  const char* message, TArgs... args) {
    std::stringstream ss;
    std::string buffer;
    buffer.resize(
        _snprintf(const_cast<char*>(buffer.data()), 0, message, args...));
    _snprintf(const_cast<char*>(buffer.data()), buffer.size(), message,
              args...);

    ss << buffer << " ('" << fileName << "' at line " << line << ")";
    return ss.str();
  }

  void warning(std::string message) {
    if (impl) impl->warning(impl, message.c_str());
  }

  void error(std::string message) {
    if (impl) impl->error(impl, message.c_str());
  }

  void info(std::string message) {
    if (impl) impl->info(impl, message.c_str());
  }

  void fatalError(std::string message) {
    if (impl) {
      impl->fatalError(impl, message.c_str());
    } else {
      std::exit(-1);
    }
  }
};

#define DR_LOG_DEBUG(message, ...)                                    \
  ::DirectRemote::logger->info(::DirectRemote::logger->createMessage( \
      __FILE__, __LINE__, (message), ##__VA_ARGS__));
#define DR_LOG_INFO(message, ...)                                     \
  ::DirectRemote::logger->info(::DirectRemote::logger->createMessage( \
      __FILE__, __LINE__, (message), ##__VA_ARGS__));
#define DR_LOG_WARNING(message, ...)                                     \
  ::DirectRemote::logger->warning(::DirectRemote::logger->createMessage( \
      __FILE__, __LINE__, (message), ##__VA_ARGS__));
#define DR_LOG_ERROR(message, ...)                                     \
  ::DirectRemote::logger->error(::DirectRemote::logger->createMessage( \
      __FILE__, __LINE__, (message), ##__VA_ARGS__));
#define DR_LOG_FATAL(message, ...)                                          \
  ::DirectRemote::logger->fatalError(::DirectRemote::logger->createMessage( \
      __FILE__, __LINE__, (message), ##__VA_ARGS__));

class LoggerExport abstract {
 protected:
  typedef internal::ILogger ILogger;

  ILogger iface;

  LoggerExport() {
    memset(&iface, 0, sizeof(iface));

    iface.release = &LoggerExport::_release;
    iface.fatalError = &LoggerExport::_fatalError;
    iface.info = &LoggerExport::_info;
    iface.warning = &LoggerExport::_warning;
    iface.error = &LoggerExport::_error;
  }

 private:
  static LoggerExport* get(ILogger* _this) {
    return (LoggerExport*)(((int64_t)_this) -
                           (int64_t)(&((LoggerExport*)nullptr)->iface));
  }

  static void _release(ILogger* _this) { delete get(_this); }

  static void _fatalError(ILogger* _this, const char* message) {
    get(_this)->fatalError(message);
  }

  static void _info(ILogger* _this, const char* message) {
    get(_this)->info(message);
  }

  static void _warning(ILogger* _this, const char* message) {
    get(_this)->warning(message);
  }

  static void _error(ILogger* _this, const char* message) {
    get(_this)->error(message);
  }

 public:
  ILogger* toInterface() { return &iface; }

  virtual void fatalError(std::string message) = 0;
  virtual void error(std::string message) = 0;
  virtual void warning(std::string message) = 0;
  virtual void info(std::string message) = 0;
};

static auto logger = Logger::create(DIRECTREMOTE_PLUGIN_NAME);
}
#endif

#endif
