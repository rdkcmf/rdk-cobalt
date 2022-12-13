# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Liberty Global Service B.V.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.5)
project(cobalt-wsrpc)
find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED YES)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -g")

include_directories(.)

pkg_search_module(JSONCPP REQUIRED "jsoncpp")
find_package(rpcserver REQUIRED)

if (WSRPC_COBALT_EVERGREEN_LITE)
    add_definitions(-DENABLE_EVERGREEN_LITE=1)
    find_library(COBALT_LIBRARY loader_app REQUIRED)
else()
    find_library(COBALT_LIBRARY cobalt REQUIRED)
endif()

include_directories(
        ${JSONCPP_INCLUDE_DIRS}
        ${RPCSERVER_INCLUDE_DIRS}
)

link_directories(
        ${JSONCPP_LIBRARY_DIRS}
        ${RPCSERVER_LIBRARY_DIRS}
)

set(SOURCE_FILES
        main.cc
        cobalt_rpc_ws.cc
        )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME}
        ${JSONCPP_LIBRARIES}
        rpcserver
        ${COBALT_LIBRARY}
        )

install (TARGETS ${PROJECT_NAME} DESTINATION bin)