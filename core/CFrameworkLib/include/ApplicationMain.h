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

#ifndef APPLICATIONMAIN_H
#define APPLICATIONMAIN_H

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include <stdint.h>
#include <string>
#include <vector>

#include "Framework.h"

namespace DirectRemote {

#ifdef _MSC_VER

void __declspec(dllexport) enableDebugMemory();
std::vector<std::string> __declspec(dllexport)
    parseCommandLine(int& outArgc, const char**& outArgv);

void freeCommandLine(int argc, const char** argv);

#define APPLICATION_MAIN()                                                  \
  struct ApplicationMainImplementation {                                    \
    int main(std::vector<std::string> cmdArgs, const int argc,              \
             const char** const argv);                                      \
  };                                                                        \
  int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,       \
                        LPSTR lpCmdLine, int nCmdShow) {                    \
    enableDebugMemory();                                                    \
    int argc;                                                               \
    const char** argv;                                                      \
    auto cmdArgs = parseCommandLine(argc, argv);                            \
    int res = ApplicationMainImplementation().main(cmdArgs, argc, argv);    \
    freeCommandLine(argc, argv);                                            \
    return res;                                                             \
  }                                                                         \
  int ApplicationMainImplementation::main(std::vector<std::string> cmdArgs, \
                                          const int argc,                   \
                                          const char** const argv)

#else

void DECLSPEC_EXPORT enableDebugMemory();
std::vector<std::string> DECLSPEC_EXPORT
parseCommandLine(int argc, const char** argv);

#define APPLICATION_MAIN()                                                    \
  struct ApplicationMainImplementation {                                      \
    int main(std::vector<std::string> cmdArgs, const int argc,                \
             const char** const argv);                                        \
  };                                                                          \
  int main(int argc, const char** argv) {                                     \
    enableDebugMemory();                                                      \
    return ApplicationMainImplementation().main(parseCommandLine(argc, argv), \
                                                argc, argv);                  \
  }                                                                           \
  int ApplicationMainImplementation::main(std::vector<std::string> cmdArgs,   \
                                          const int argc,                     \
                                          const char** const argv)
#endif
}  // namespace DirectRemote

#endif  // APPLICATIONMAIN_H
