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

#ifndef VIEWERAPI_H
#define VIEWERAPI_H

#include "Framework.h"

#include <stdint.h>

#ifdef __cplusplus
namespace DirectRemote {
extern "C" {
#endif

struct EInputSource {
  enum Type {
    Keyboard = 0,
    Mouse = 1,
    GamePad = 2,
  };

  Type unused;
};

struct EKeys {
  enum Type {
    Ctrl = 0,
    Shift = 1,
    Alt = 2,
    A = 3,
    B = 4,
    C = 5,
    D = 6,
    E = 7,
    F = 8,
    G = 9,
    H = 10,
    I = 11,
    J = 12,
    K = 13,
    L = 14,
    M = 15,
    N = 16,
    O = 17,
    P = 18,
    Q = 19,
    R = 20,
    S = 21,
    T = 22,
    U = 23,
    V = 24,
    W = 25,
    X = 26,
    Y = 27,
    Z = 28,
    Enter = 29,
    Backspace = 30,
    Space = 31,
    Left = 32,
    Right = 33,
    Up = 34,
    Down = 35,
    F1 = 36,
    F2 = 37,
    F3 = 38,
    F4 = 39,
    F5 = 40,
    F6 = 41,
    F7 = 42,
    F8 = 43,
    F9 = 44,
    F10 = 45,
    F11 = 46,
    F12 = 47,
    N1 = 48,
    N2 = 49,
    N3 = 50,
    N4 = 51,
    N5 = 52,
    N6 = 53,
    N7 = 54,
    N8 = 55,
    N9 = 56,
    N0 = 57,
    Tab = 58,
    Escape = 59,
  };

  Type unused;
};

struct EMouseWheel {
  enum Type {
    X = 0,
    Y = 1,
  };

  Type unused;
};

struct EMouseButtons {
  enum Type {
    Left = 0,
    Middle = 1,
    Right = 2,
  };

  Type unused;
};

struct EGamePadButtons {
  enum Type {
    X = 0,
    Y = 1,
    A = 2,
    B = 3,
    Back = 4,
    Start = 5,
    L1 = 6,
    L2 = 7,
    Left = 8,
    Right = 9,
    Up = 10,
    Down = 11,
    Home = 12,
    LB = 13,
    RB = 14,
  };

  Type unused;
};

typedef struct ViewerImpl *PViewer;
typedef void (*DOnBackbufferInfo)(int32_t width, int32_t height,
                                  void *userContext);

typedef void (*DOnAudioSamples)(uint16_t *pcmSamples, int32_t sampleCount,
                                int32_t frequency, int32_t channelCount,
                                void *userContext);

DECLSPEC_EXPORT PViewer viewerCreate(int argc, const char *const *argv,
                                     DOnBackbufferInfo onBackBufferInfo,
                                     DOnAudioSamples onAudioSamples,
                                     void *userContext);

#pragma pack(1)
struct MouseInfo {
  bool isCaptured;
};
#pragma pack()

DECLSPEC_EXPORT void viewerGetMouseInfo(PViewer viewer, MouseInfo *outMouse);

DECLSPEC_EXPORT bool viewerUpdate(PViewer viewer);

DECLSPEC_EXPORT void viewerConfigureBackbuffer(PViewer viewer,
                                               void *pixelsRgba32,
                                               int32_t width, int32_t height);

DECLSPEC_EXPORT void viewerTrackMouseAbsolute(PViewer viewer, float x, float y);

DECLSPEC_EXPORT void viewerTrackMouseRelative(PViewer viewer, float deltaX,
                                              float deltaY);

DECLSPEC_EXPORT void viewerTrackButton(PViewer viewer, int8_t deviceId,
                                       bool isPressed, int8_t buttonId,
                                       int32_t unicodeChar);

DECLSPEC_EXPORT void viewerTrackAxis(PViewer viewer, int8_t deviceId,
                                     int8_t axisId, float value);

DECLSPEC_EXPORT void viewerRelease(PViewer viewer);

#ifdef __cplusplus
}
}  // namespace DirectRemote
#endif

#endif