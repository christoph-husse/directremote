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

#include <functional>
#include "RandomGenerator.h"

namespace DirectRemote {
RandomGenerator::RandomGenerator() {
  std::array<int, std::mt19937::state_size> seed_data;
  std::random_device r;
  std::generate_n(seed_data.data(), seed_data.size(), std::ref(r));
  std::seed_seq seq(std::begin(seed_data), std::end(seed_data));

  twister = std::mt19937(seq);
}

void RandomGenerator::genBytes(void *buffer, int byteCount) {
  for (int i = 0; i < byteCount; i++) {
    (reinterpret_cast<unsigned char *>(buffer))[i] = twister();
  }
}
}  // namespace DirectRemote
