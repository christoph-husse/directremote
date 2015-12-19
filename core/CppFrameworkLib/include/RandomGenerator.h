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

#ifndef RANDOMGENERATOR_H
#define RANDOMGENERATOR_H

#include <stdint.h>
#include <algorithm>
#include <array>
#include <random>

#undef min
#undef max

namespace DirectRemote {
class RandomGenerator {
 private:
  std::mt19937 twister;

 public:
  RandomGenerator();

  template <class TNumeric>
  TNumeric genInt(TNumeric _min = std::numeric_limits<TNumeric>::min(),
                  TNumeric _max = std::numeric_limits<TNumeric>::max()) {
    std::uniform_int_distribution<TNumeric> dist(_min, _max);
    return dist(twister);
  }

  template <class TNumeric>
  TNumeric genReal(TNumeric _min = std::numeric_limits<TNumeric>::min(),
                   TNumeric _max = std::numeric_limits<TNumeric>::max()) {
    std::uniform_real_distribution<TNumeric> dist(_min, _max);
    return dist(twister);
  }

  template <class TPOD>
  typename std::enable_if<std::is_pod<TPOD>::value, void>::type gen(TPOD &pod) {
    genBytes(&pod, sizeof(pod));
  }

  void genBytes(void *buffer, int byteCount);
};
}  // namespace DirectRemote

#endif