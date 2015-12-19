# 
# Copyright (c) 2015 Christoph Husse
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
# and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
# TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
# IN THE SOFTWARE.
# 

cmake_minimum_required(VERSION 2.8)

if(BUILD_CFrameworkLib)

add_definitions(-DDIRECTREMOTE_PLUGIN_NAME=\"CFrameworkLib\")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(sources 
		include/IPerformanceMonitor.h
		PerformanceMonitor.cpp

		ConsoleLogger.cpp

		include/ApplicationMain.h
		ApplicationMain.cpp

		include/SingleInstanceLock.h
		SingleInstanceLock.cpp

		include/UdpChunk.h
)

add_library(CFrameworkLib STATIC ${sources})

endif()
