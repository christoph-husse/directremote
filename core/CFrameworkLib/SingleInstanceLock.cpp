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

#include "SingleInstanceLock.h"

#include <map>

namespace DirectRemote {

#ifdef _MSC_VER

#include <windows.h>

DECLSPEC_EXPORT bool tryAcquireSingleInstanceLock(const char* name) {
  static std::map<std::string, HANDLE> acquiredLocks;

  if (acquiredLocks.find(name) != acquiredLocks.end()) {
    return true;
  }

  HANDLE hMutex = CreateMutexA(nullptr, true, name);

  if ((hMutex == NULL) || (hMutex == INVALID_HANDLE_VALUE)) {
    return false;
  }

  if (WaitForSingleObject(hMutex, 0) != WAIT_OBJECT_0) {
    CloseHandle(hMutex);
    return false;
  }

  acquiredLocks[name] = hMutex;

  return true;
}

DECLSPEC_EXPORT bool couldAcquireSingleInstanceLock(const char* name) {
  HANDLE hMutex = CreateMutexA(nullptr, true, name);

  if ((hMutex == NULL) || (hMutex == INVALID_HANDLE_VALUE)) {
    return false;
  }

  bool canAcquire = WaitForSingleObject(hMutex, 0) == WAIT_OBJECT_0;

  if (canAcquire) {
    ReleaseMutex(hMutex);
  }

  CloseHandle(hMutex);

  return canAcquire;
}

#else

DECLSPEC_EXPORT bool tryAcquireSingleInstanceLock(const char* name) {
  // TODO(natalie): implement on all systems where DRHost is supported
  (void)name;
  return true;
}

DECLSPEC_EXPORT bool couldAcquireSingleInstanceLock(const char* name) {
  // TODO(natalie): implement on all systems where DRHost is supported
  (void)name;
  return true;
}

#endif
}  // namespace DirectRemote
