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

#include "InputInject.h"
#include "ViewerApi.h"
#include "SingleInstanceLock.h"
#include "IPerformanceMonitor.h"

#if BOOST_OS_LINUX

namespace DirectRemote {

bool InputInject::hasViewerOnSameSystem() {
  return false;
}

bool InputInject::isMouseCaptured() { return false; }

void InputInject::injectMouseMotion(float mouseX, float mouseY,
                                    float mouseDeltaX, float mouseDeltaY) {
}

bool InputInject::buttonToVK(int32_t buttonId, int32_t &vk) {
  return true;
}

void InputInject::injectButton(int8_t sourceType, bool isPressed,
                               int8_t buttonId, int32_t unicodeChar) {
}

void InputInject::injectAxis(int8_t sourceType, int8_t axisId, float value) {
}
}

#endif
