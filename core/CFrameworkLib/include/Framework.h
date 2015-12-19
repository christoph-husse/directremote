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

#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include <boost/predef.h>

#undef BOOST_COMP_MSVC

#if !BOOST_COMP_GNUC && !BOOST_COMP_CLANG
#define BOOST_COMP_MSVC 1
#else
#define BOOST_COMP_MSVC 0

#include <pthread.h>
#endif

#if BOOST_COMP_MSVC
#define DECLSPEC_EXPORT __declspec(dllexport)
typedef intptr_t ssize_t;

#else
#define DECLSPEC_EXPORT
#include <string.h>
#include <utility>
#define abstract
#endif

namespace DirectRemote {

struct NonCopyable {
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable(NonCopyable&&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
  NonCopyable& operator=(NonCopyable&&) = delete;
};

}

#endif  // FRAMEWORK_H
