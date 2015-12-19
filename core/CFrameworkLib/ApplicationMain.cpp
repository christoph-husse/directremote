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

#ifdef _MSC_VER

#include <Windows.h>
#include <codecvt>
#include <locale>

namespace DirectRemote {

void enableDebugMemory() {
  int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
  // tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;
  _CrtSetDbgFlag(tmpFlag);
}

void freeCommandLine(int argc, const char** argv) {
  for (int i = 0; i < argc; i++) {
    free(const_cast<void*>(static_cast<const void*>(argv[i])));
  }
  free(argv);
}

std::vector<std::string> parseCommandLine(int& outArgc, const char**& outArgv) {
  std::vector<std::string> res;
  outArgc = 0;
  auto argv = CommandLineToArgvW(GetCommandLineW(), &outArgc);
  outArgv = new const char* [outArgc];

  for (int i = 0; i < outArgc; i++) {
    auto str =
        std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(argv[i]);
    outArgv[i] = strdup(str.c_str());
    res.push_back(str);
  }

  return res;
}
}

#else

namespace DirectRemote {

void enableDebugMemory() {}

std::vector<std::string> parseCommandLine(int argc, const char** argv) {
  std::vector<std::string> res;
  for (int i = 0; i < argc; i++) {
    res.push_back(argv[i]);
  }
  return res;
}
}  // namespace DirectRemote
#endif
