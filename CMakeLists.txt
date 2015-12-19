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

set(CMAKE_CONFIGURATION_TYPES Release;Debug)

project (DirectRemote)

include(cmake/TargetArch.cmake)


if("${PLATFORM}" STREQUAL "")
    target_architecture(TARGET_ARCH)

    if(${MSVC})
        set(PLATFORM "windows")
        set(POSIX OFF)
    else()
        if(${APPLE})
            set(PLATFORM "darwin")
            set(POSIX ON)
        else()
            set(PLATFORM "linux")
            set(POSIX ON)
        endif()
    endif()

    set(PLATFORM "${PLATFORM}-${TARGET_ARCH}")
endif()

message(STATUS "Building for platform '${PLATFORM}'.")

if(${MSVC})
    set(CMAKE_CXX_FLAGS_RELEASE "/MP /EHa /MT /W4 /Ox /Ob2 /Oi /Ot /GL /Gy /D NDEBUG /GR /Zi" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_RELEASE "/MP /EHa /MT /W4 /Ox /Ob2 /Oi /Ot /GL /Gy /D NDEBUG /GR /Zi" CACHE STRING "" FORCE)

    set(CMAKE_CXX_FLAGS_DEBUG "/MP /EHa /W4 /MTd /D NDEBUG /Od /Zi" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_DEBUG "/MP /EHa /W4 /MTd /D NDEBUG /Od /Zi" CACHE STRING "" FORCE)

else()
    if(${APPLE})
        add_definitions(-stdlib=libc++ --std=c++11)
    else()
        set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-rpath '-Wl,\$ORIGIN'")

        add_definitions(
                --std=c++11 
                -fPIC 
                -s
                
                -w -fpermissive
#                -Weverything 
#                -Werror
                
                -Wno-c++98-compat 
                -Wno-c++98-compat-pedantic 
                -Wno-packed 
                -Wno-padded 
                -Wno-nested-anon-types
                -Wno-weak-vtables
                -Wno-missing-prototypes
                -Wno-reserved-id-macro
        )
    endif()
endif()

set(BIN_DIR "${ARTIFACT_DIRECTORY}/")
file(MAKE_DIRECTORY "${BIN_DIR}")

set(DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/")
set(DEPS_PLATFORM_DIR "${DEPS_DIR}/${PLATFORM}")
set(DEPS_GENERIC_DIR "${DEPS_DIR}/generic")

set(BOOST_ROOT "${DEPS_GENERIC_DIR}/boost")
set(BOOST_LIBS "${DEPS_PLATFORM_DIR}/boost/lib")

set(INTEL_MEDIA_SDK_ROOT "${DEPS_PLATFORM_DIR}/intel-inde/")
set(CUDA_ROOT "${DEPS_PLATFORM_DIR}/cuda/")
set(GTEST_ROOT "${DEPS_GENERIC_DIR}/gtest")

link_directories(${BOOST_LIBS})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/core/CFrameworkLib/include
    ${CMAKE_CURRENT_SOURCE_DIR}/core/CppFrameworkLib/include
    ${BOOST_ROOT}
)

file(GLOB PREBUILT_DIR
  "${DEPS_PLATFORM_DIR}/prebuilt/*"
)
file(COPY ${PREBUILT_DIR} DESTINATION "${BIN_DIR}")

add_subdirectory(core)
add_subdirectory(plugins)
add_subdirectory(tests)
add_subdirectory(viewers)
 