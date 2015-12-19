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

#include "ILogger.h"

#if BOOST_COMP_MSVC
#include <Windows.h>
#endif

#include <mutex>
#include <iostream>

namespace DirectRemote {

struct IInternalLogger {
  virtual ~IInternalLogger() {}

  virtual void fatalError(std::string message) = 0;
  virtual void error(std::string message) = 0;
  virtual void warning(std::string message) = 0;
  virtual void info(std::string message) = 0;
};

enum class ELogLevel {
  Warning,
  Error,
  Info,
  Fatal,
};

class LoggerTextBase abstract : public IInternalLogger {
 protected:
  virtual void write(ELogLevel level, std::string message) = 0;

 public:
  void fatalError(std::string message) override {
    write(ELogLevel::Fatal, "[FATAL]: " + message + "\n");
  }

  void error(std::string message) override {
    write(ELogLevel::Error, "[ERROR]: " + message + "\n");
  }

  void warning(std::string message) override {
    write(ELogLevel::Warning, "[WARNING]: " + message + "\n");
  }

  void info(std::string message) override {
    write(ELogLevel::Info, "[INFO]: " + message + "\n");
  }
};

class LoggerDebugConsole : public LoggerTextBase {
 protected:
  void write(ELogLevel level, std::string message) override {
#if BOOST_COMP_MSVC
    OutputDebugStringA(message.c_str());
#else
// TODO: anything to do here?
#endif
  }
};

class LoggerConsole : public LoggerTextBase {
 protected:
  void write(ELogLevel level, std::string message) override {
#if BOOST_COMP_MSVC
    HANDLE handle;

    switch (level) {
      case ELogLevel::Warning:
        handle = GetStdHandle(STD_OUTPUT_HANDLE);
        break;
      case ELogLevel::Info:
        handle = GetStdHandle(STD_OUTPUT_HANDLE);
        break;
      default:
        handle = GetStdHandle(STD_ERROR_HANDLE);
        break;
    }

    CONSOLE_SCREEN_BUFFER_INFO info = {};

    GetConsoleScreenBufferInfo(handle, &info);

    switch (level) {
      case ELogLevel::Warning:
        SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN);
        break;
      case ELogLevel::Info:
        break;
      default:
        SetConsoleTextAttribute(handle, FOREGROUND_RED);
        break;
    }

    WriteFile(handle, message.c_str(), message.size(), nullptr, nullptr);

    SetConsoleTextAttribute(handle, info.wAttributes);
#else
    std::cout << message;
#endif
  }
};

class LoggerMultiplexer : public IInternalLogger {
 private:
  std::mutex mutex;
  std::vector<std::shared_ptr<IInternalLogger>> loggers;

 public:
  LoggerMultiplexer() {
    // TODO: configure logger from file
    addLogger(std::make_shared<LoggerConsole>());
    addLogger(std::make_shared<LoggerDebugConsole>());
  }

  void addLogger(std::shared_ptr<IInternalLogger> logger) {
    loggers.push_back(logger);
  }

  void fatalError(std::string message) override {
    std::lock_guard<std::mutex> lock(mutex);

    for (auto logger : loggers) logger->fatalError(message);
  }

  void error(std::string message) override {
    std::lock_guard<std::mutex> lock(mutex);

    for (auto logger : loggers) logger->error(message);
  }

  void warning(std::string message) override {
    std::lock_guard<std::mutex> lock(mutex);

    for (auto logger : loggers) logger->warning(message);
  }

  void info(std::string message) override {
    std::lock_guard<std::mutex> lock(mutex);

    for (auto logger : loggers) logger->info(message);
  }
};

class Logger_default : public LoggerExport {
 private:
  std::shared_ptr<IInternalLogger> logger;

 public:
  Logger_default(std::shared_ptr<IInternalLogger> logger) : logger(logger) {
    std::ios::sync_with_stdio();
  }

  void fatalError(std::string message) override {
    logger->fatalError(message);

    std::exit(-1);
  }

  void error(std::string message) override { logger->error(message); }

  void warning(std::string message) override { logger->warning(message); }

  void info(std::string message) override { logger->info(message); }
};

extern "C" DECLSPEC_EXPORT internal::ILogger* createLogger() {
  static auto logger = std::make_shared<LoggerMultiplexer>();

  auto result = new (std::nothrow) Logger_default(logger);
  if (!result) {
    return nullptr;
  }

  return result->toInterface();
}
}  // namespace DirectRemote