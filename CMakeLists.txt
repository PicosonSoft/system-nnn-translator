# Copyright (C) 2025 Rodrigo Jose Hernandez Cordoba
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

cmake_minimum_required(VERSION 3.12.0)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
  cmake_policy(SET CMP0072 NEW)
  if(POLICY CMP0020)
    cmake_policy(SET CMP0020 NEW)
  endif(POLICY CMP0020)
  if(POLICY CMP0053)
    cmake_policy(SET CMP0053 NEW)
  endif(POLICY CMP0053)
endif(COMMAND cmake_policy)

project(system-nnn-translate)
enable_language(CXX)
enable_language(C)
include(FetchContent)

if(NOT ENV{MSYSTEM_PREFIX} STREQUAL "")
  list(APPEND CMAKE_PREFIX_PATH "$ENV{MSYSTEM_PREFIX}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

find_package(Iconv REQUIRED)
find_package(CURL REQUIRED)
find_library(SOCKET_LIB
             NAMES ws2_32
             HINTS ${CMAKE_INSTALL_FULL_LIBDIR}
)

find_path(JSON_HPP_DIR nlohmann/json.hpp)

if(JSON_HPP_DIR)
  message(STATUS "Found nlohmann/json.hpp in: ${JSON_HPP_DIR}")
  include_directories(${JSON_HPP_DIR})
else()
  message(FATAL_ERROR "json.hpp not found.")
endif()


add_executable(decode decode.cpp)

add_executable(extract extract.cpp)
target_link_libraries(extract PRIVATE Iconv::Iconv)

add_executable(inject inject.cpp)
target_link_libraries(inject PRIVATE Iconv::Iconv)

add_executable(azure-translate azure-translate.cpp)
target_link_libraries(azure-translate PRIVATE CURL::libcurl)


FetchContent_Declare(ollama-hpp
  GIT_REPOSITORY "https://github.com/jmont-dev/ollama-hpp.git"
  GIT_TAG 237ceace488791b527f0f5b7b69b11ffa2fd073b
)

FetchContent_MakeAvailable(ollama-hpp)

find_path(OLLAMA_HPP_DIR ollama.hpp PATHS ${CMAKE_BINARY_DIR}/_deps/ollama-hpp-src/singleheader)

if(OLLAMA_HPP_DIR)
  message(STATUS "Found ollama-hpp in: ${OLLAMA_HPP_DIR}")
  include_directories(${OLLAMA_HPP_DIR})
else()
  message(WARNING "ollama.hpp not found.")
endif()

add_executable(ollama-translate ollama-translate.cpp)
target_link_libraries(ollama-translate PRIVATE ${SOCKET_LIB})
target_compile_definitions(ollama-translate PUBLIC -D_WIN32_WINNT=0x602)
