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

if(BUILD_DRHost)

add_definitions(-DDIRECTREMOTE_PLUGIN_NAME=\"DRHost\")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../DRViewerLib/include
)

set(EXE_RESOURCE "")

if(${MSVC})
	set(EXE_RESOURCE "../resources/windows.rc")
endif()

add_executable(
	DRHost

	WIN32

	${EXE_RESOURCE}

	InputInject.h
	InputInject_win32.cpp
	InputInject_linux.cpp

	HostNetworkAbstraction.h
	HostNetworkAbstraction.cpp
	
	main.cpp
)

target_link_libraries(
	DRHost 

	CFrameworkLib 
	CppFrameworkLib
)

if(${MSVC})
	target_link_libraries(DRHost ws2_32.lib)
endif()

if(${POSIX})
	target_link_libraries(
		DRHost 
		boost_program_options
		boost_filesystem
		boost_system
		dl
	)
endif()

add_custom_command(TARGET DRHost POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:DRHost>" "${BIN_DIR}"
)

endif()