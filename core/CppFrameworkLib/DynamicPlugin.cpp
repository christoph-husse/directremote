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

#include "DynamicPlugin.h"
#include "ILogger.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"

#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#pragma clang diagnostic pop

#ifdef _MSC_VER

#include <vector>
#include <Windows.h>

#endif

namespace fs = boost::filesystem;

namespace DirectRemote {

typedef void* (*CreatePluginProcedure)();

#ifdef _MSC_VER

void* tryCreatePlugin(std::string file, std::vector<std::string> funcNames) {
  file = fs::canonical(file).string();

  HMODULE hMod =
      LoadLibraryExA(file.c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                                             LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
  if (hMod == nullptr) {
    return nullptr;
  }

  CreatePluginProcedure proc = nullptr;

  for (const auto& funcName : funcNames) {
    CreatePluginProcedure candidate =
        (CreatePluginProcedure)GetProcAddress(hMod, funcName.c_str());
    if (candidate) {
      proc = candidate;
      break;
    }
  }

  if (proc) {
    void* result = proc();
    if (result) {
      return result;
    }
  }

  return nullptr;
}

#else

#include <dlfcn.h>

void* tryCreatePlugin(std::string file, std::vector<std::string> funcNames) {
  DR_LOG_INFO("Trying to load plugin at '", file, "'.");

  auto hMod = dlopen (file.c_str(), RTLD_LAZY);
  if (!hMod) {
    return nullptr;
  }

  CreatePluginProcedure proc = nullptr;

  DR_LOG_INFO("Hello2");

  for (const auto& funcName : funcNames) {
    CreatePluginProcedure candidate =
        (CreatePluginProcedure)dlsym(hMod, funcName.c_str());
    if ((dlerror() == NULL) && candidate) {
      proc = candidate;
      DR_LOG_INFO("Hello3");
      break;
    }
  }

  if (proc) {
    void* result = proc();
    if (result) {
      return result;
    }
  }

  return nullptr;
}

#endif

std::vector<std::string> listFiles(std::string directory,
                                   std::string filename) {
  fs::directory_iterator end_iter;
  std::vector<std::string> result;

  if (fs::exists(directory) && fs::is_directory(directory)) {
    for (fs::directory_iterator dir_iter(directory); dir_iter != end_iter;
         ++dir_iter) {
      if (fs::is_regular_file(dir_iter->status())) {
        auto path = fs::canonical(dir_iter->path()).string();
        if (boost::ends_with(path, filename)) {
          result.push_back(path);
        }
      }
    }
  }

  return result;
}

EXTERN_C DECLSPEC_EXPORT void* createPlugin(const char* guid,
                                            const char* pluginPrefix,
                                            const char* pluginOrderString_) {
  std::vector<std::string> funcNames;
  std::vector<std::string> pluginOrder;
  std::string baseName = "createPlugin_" + std::string(guid);
  const std::string pluginOrderString = pluginOrderString_;
  std::string moduleExtension;
  std::string modulePrefix;

  funcNames.push_back(baseName);

#if BOOST_COMP_MSVC
  moduleExtension = ".dll";

  funcNames.push_back("_" + baseName);
  funcNames.push_back(baseName + "@0");
  funcNames.push_back("_" + baseName + "@0");
#else

  moduleExtension = ".so";
  modulePrefix = "lib";
#endif


  if (!pluginOrderString.empty()) {
    if (pluginOrderString.find(",") != std::string::npos) {
      boost::char_separator<char> sep(",");
      for (auto part : boost::tokenizer<boost::char_separator<char>>(
               pluginOrderString, sep)) {
        if (!part.empty()) {
          pluginOrder.push_back(part);
        }
      }
    } else {
      pluginOrder.push_back(pluginOrderString);
    }

    for (auto pluginName : pluginOrder) {
      auto plugin = tryCreatePlugin(
          modulePrefix + pluginPrefix + "_" + pluginName + moduleExtension, funcNames);
      if (plugin) {
        return plugin;
      }
    }
  }

  for (auto file : listFiles("./", moduleExtension)) {
    auto plugin = tryCreatePlugin(file, funcNames);
    if (plugin) {
      return plugin;
    }
  }

  return nullptr;
}
}  // namespace DirectRemote
