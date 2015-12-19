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

#if BOOST_OS_WINDOWS

#include <Windows.h>

namespace DirectRemote {

bool InputInject::hasViewerOnSameSystem() {
  static PerformanceMonitor timer;
  static double lastTestTime = timer.totalElapsedTime();
  static bool lastTestResult = false;

  if (timer.totalElapsedTime() - lastTestTime < 1) {
    // use cached result
    return lastTestResult;
  }

  lastTestTime = timer.totalElapsedTime();
  lastTestResult = !couldAcquireSingleInstanceLock(
                       "DirectRemote-2ac33a94-f51d-4ad6-b4f8-399f88e1e15e");

  return lastTestResult;
}

bool InputInject::isMouseCaptured() { return isMouseCapturedFlag; }

void InputInject::injectMouseMotion(float mouseX, float mouseY,
                                    float mouseDeltaX, float mouseDeltaY) {
  if (hasViewerOnSameSystem()) {
    return;
  }

  bool isCaptured = false;
  CURSORINFO curInfo = {};
  curInfo.cbSize = sizeof(curInfo);

  if (GetCursorInfo(&curInfo) && (curInfo.flags != CURSOR_SHOWING)) {
    isMouseCapturedFlag = true;

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = mouseDeltaX;
    input.mi.dy = mouseDeltaY;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    SendInput(1, &input, sizeof(input));

  } else {
    isMouseCapturedFlag = false;

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = (LONG)(mouseX * (65536.0f / GetSystemMetrics(SM_CXSCREEN)));
    input.mi.dy = (LONG)(mouseY * (65536.0f / GetSystemMetrics(SM_CYSCREEN)));
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    if (!SendInput(1, &input, sizeof(input))) {
      SetCursorPos(mouseX, mouseY);
    }
  }
}

bool InputInject::buttonToVK(int32_t buttonId, int32_t &vk) {
  vk = 0;

  switch (buttonId) {
    case EKeys::Ctrl:
      vk = VK_CONTROL;
      break;
    case EKeys::Alt:
      vk = VK_MENU;
      break;
    case EKeys::Shift:
      vk = VK_SHIFT;
      break;
    case EKeys::Space:
      vk = VK_SPACE;
      break;
    case EKeys::Backspace:
      vk = VK_BACK;
      break;
    case EKeys::Enter:
      vk = VK_RETURN;
      break;
    case EKeys::Left:
      vk = VK_LEFT;
      break;
    case EKeys::Right:
      vk = VK_RIGHT;
      break;
    case EKeys::Up:
      vk = VK_UP;
      break;
    case EKeys::Down:
      vk = VK_DOWN;
      break;

    case EKeys::N0:
      vk = '0';
      break;
    case EKeys::N1:
      vk = '1';
      break;
    case EKeys::N2:
      vk = '2';
      break;
    case EKeys::N3:
      vk = '3';
      break;
    case EKeys::N4:
      vk = '4';
      break;
    case EKeys::N5:
      vk = '5';
      break;
    case EKeys::N6:
      vk = '6';
      break;
    case EKeys::N7:
      vk = '7';
      break;
    case EKeys::N8:
      vk = '8';
      break;
    case EKeys::N9:
      vk = '9';
      break;

    case EKeys::F1:
      vk = VK_F1;
      break;
    case EKeys::F2:
      vk = VK_F2;
      break;
    case EKeys::F3:
      vk = VK_F3;
      break;
    case EKeys::F4:
      vk = VK_F4;
      break;
    case EKeys::F5:
      vk = VK_F5;
      break;
    case EKeys::F6:
      vk = VK_F6;
      break;
    case EKeys::F7:
      vk = VK_F7;
      break;
    case EKeys::F8:
      vk = VK_F8;
      break;
    case EKeys::F9:
      vk = VK_F9;
      break;
    case EKeys::F10:
      vk = VK_F10;
      break;
    case EKeys::F11:
      vk = VK_F11;
      break;
    case EKeys::F12:
      vk = VK_F12;
      break;

    case EKeys::Escape:
      vk = VK_ESCAPE;
      break;

    case EKeys::A:
      vk = 'A';
      break;
    case EKeys::B:
      vk = 'B';
      break;
    case EKeys::C:
      vk = 'C';
      break;
    case EKeys::D:
      vk = 'D';
      break;
    case EKeys::E:
      vk = 'E';
      break;
    case EKeys::F:
      vk = 'F';
      break;
    case EKeys::G:
      vk = 'G';
      break;
    case EKeys::H:
      vk = 'H';
      break;
    case EKeys::I:
      vk = 'I';
      break;
    case EKeys::J:
      vk = 'J';
      break;
    case EKeys::K:
      vk = 'K';
      break;
    case EKeys::L:
      vk = 'L';
      break;
    case EKeys::M:
      vk = 'M';
      break;
    case EKeys::N:
      vk = 'N';
      break;
    case EKeys::O:
      vk = 'O';
      break;
    case EKeys::P:
      vk = 'P';
      break;
    case EKeys::Q:
      vk = 'Q';
      break;
    case EKeys::R:
      vk = 'R';
      break;
    case EKeys::S:
      vk = 'S';
      break;
    case EKeys::T:
      vk = 'T';
      break;
    case EKeys::U:
      vk = 'U';
      break;
    case EKeys::V:
      vk = 'V';
      break;
    case EKeys::W:
      vk = 'W';
      break;
    case EKeys::X:
      vk = 'X';
      break;
    case EKeys::Y:
      vk = 'Y';
      break;
    case EKeys::Z:
      vk = 'Z';
      break;

    case EKeys::Tab:
      vk = VK_TAB;
      break;

    default:
      return false;
  }

  return true;
}

void InputInject::injectButton(int8_t sourceType, bool isPressed,
                               int8_t buttonId, int32_t unicodeChar) {
  if (hasViewerOnSameSystem()) {
    return;
  }

  switch ((EInputSource::Type)sourceType) {
    case EInputSource::Keyboard: {
      INPUT ip = {};
      int32_t virtualKeyCode;

      if (!buttonToVK(buttonId, virtualKeyCode)) {
        return;
      }

      ip.type = INPUT_KEYBOARD;
      ip.ki.wScan = MapVirtualKey(virtualKeyCode, MAPVK_VK_TO_VSC);
      ip.ki.time = 0;
      ip.ki.dwExtraInfo = 0;
      ip.ki.wVk = virtualKeyCode;
      ip.ki.dwFlags = 0;

      if (!isPressed) {
        ip.ki.dwFlags = KEYEVENTF_KEYUP;
      }

      SendInput(1, &ip, sizeof(INPUT));

    } break;

    case EInputSource::Mouse: {
      INPUT input = {};
      input.type = INPUT_MOUSE;

      if (isPressed) {
        switch (buttonId) {
          case EMouseButtons::Middle:
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
            break;
          case EMouseButtons::Right:
            input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            break;
          default:
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            break;
        }
      } else {
        switch (buttonId) {
          case EMouseButtons::Middle:
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
            break;
          case EMouseButtons::Right:
            input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            break;
          default:
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            break;
        }
      }

      SendInput(1, &input, sizeof(input));
    } break;
  }
}

void InputInject::injectAxis(int8_t sourceType, int8_t axisId, float value) {
  if (hasViewerOnSameSystem()) {
    return;
  }

  switch ((EInputSource::Type)sourceType) {
    case EInputSource::Mouse: {
      INPUT input = {};
      input.type = INPUT_MOUSE;
      input.mi.mouseData = value * WHEEL_DELTA;

      switch (axisId) {
        case EMouseWheel::X:
          input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
          break;
        default:
          input.mi.dwFlags = MOUSEEVENTF_WHEEL;
          break;
      }

      SendInput(1, &input, sizeof(input));
    } break;
  }
}
}

#endif
