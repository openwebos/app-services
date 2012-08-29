# @@@LICENSE
#
#      Copyright (c) 2007-2012 Hewlett-Packard Development Company, L.P.
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
#
# LICENSE@@@

cmake_minimum_required(VERSION 2.8.7)

project(email-common CXX)

include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC2)
webos_component(2 0 0)

webos_add_compiler_flags(ALL "-DMOJ_LINUX -DMOJ_X86")
webos_add_compiler_flags(ALL "-O2 -ggdb -Wuninitialized -Winit-self -Wno-error=strict-aliasing")

webos_machine_dep()

# These variables can change the default behaviour and need to be set before calling find_package
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS regex system filesystem) 
include_directories(Boost_INCLUDE_DIRS)

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS})

pkg_check_modules(CJSON cjson)
include_directories(${CJSON_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${CJSON_CFLAGS})

#pkg_check_modules(MOJODB mojodb)
#include_directories(${MOJODB_INCLUDE_DIRS})
#webos_add_compiler_flags(ALL ${MOJODB_CFLAGS})

pkg_check_modules(PALMSOCKET REQUIRED palmsocket)
include_directories(${PALMSOCKET_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PALMSOCKET_CFLAGS})

pkg_check_modules(SANDBOX REQUIRED sandbox)
include_directories(${SANDBOX_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${SANDBOX_CFLAGS})

pkg_check_modules(PMLOG REQUIRED PmLogLib)
include_directories(${PMLOG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOG_CFLAGS})

pkg_check_modules(LUNASERVICE REQUIRED luna-service2)
include_directories(${LUNASERVICE_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LUNASERVICE_CFLAGS})

pkg_check_modules(JEMALLOC_MT REQUIRED jemalloc_mt)
include_directories(${JEMALLOC_MT_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${JEMALLOC_MT_CFLAGS})

find_library(MJCORE NAMES mojocore libmojocore)
if(MJCORE STREQUAL "MJCORE-NOTFOUND")
        message(FATAL_ERROR "Failed to find libmojocore. Please install openwebos/db8")
endif()

find_library(MJLUNA NAMES mojoluna libmojoluna)
if(MJLUNA STREQUAL "MJLUNA-NOTFOUND")
        message(FATAL_ERROR "Failed to find libmojoluna. Please install openwebos/db8")
endif()

find_library(MJDB NAMES mojodb libmojodb)
if(MJDB STREQUAL "MJDB-NOTFOUND")
        message(FATAL_ERROR "Failed to find mojodb. Please install openwebos/db8")
endif()

find_path(MJ_INC NAMES mojodb/core/MojErr.h)
if(MJ_INC STREQUAL "MJ_INC-NOTFOUND")
        message(FATAL_ERROR "Failed to find mojodb header. Please install openwebos/db8")
endif()
include_directories(${MJ_INC}/mojodb)

find_library(ICU NAMES icuuc)
if(ICU STREQUAL "ICU-NOTFOUND")
   message(FATAL_ERROR "Failed to find ICU4C libaries. Please intall.")
endif()

find_library(ICUI18N NAMES icui18n)
if(ICUI18N STREQUAL "ICUI18N-NOTFOUND")
   message(FATAL_ERROR "Failed to find ICU-I18N libaries. Please intall.")
endif()

find_library(LIBZ NAMES z)
if(LIBZ STREQUAL "LIBZ-NOTFOUND")
   message(FATAL_ERROR "Failed to find libz libraries.")
endif()

include_directories(include/public/email-common)

aux_source_directory(src/activity activity_files)
aux_source_directory(src/client client_files)
aux_source_directory(src/data data_files)
aux_source_directory(src/exceptions exceptions_files)
aux_source_directory(src/mimeparser mimeparser_files)
aux_source_directory(src/stream stream_files)
aux_source_directory(src/async async_files)
list(REMOVE_ITEM async_files "src/async/AsyncBuffer.cpp")
aux_source_directory(src/email email_files)
aux_source_directory(src/glibcurl glibcurl_files)
aux_source_directory(src/network network_files)
aux_source_directory(src/util util_files)

add_library(email-common SHARED src/CommonErrors.cpp ${activity_files} ${client_files} ${data_files} ${exceptions_files} ${mimeparser_files} ${stream_files} ${async_files} ${email_files} ${glibcurl_files} ${network_files} ${util_files})

target_link_libraries(email-common ${GLIB2_LDFLAGS} ${CJSON_LDFLAGS} ${MJCORE} ${MJLUNA} ${MJDB} ${PALMSOCKET_LDFLAGS} ${SANDBOX_LDFLAGS} ${PMLOG_LDFLAGS} ${LUNASERVICE_LDFLAGS} ${JEMALLOC_MT_LDFLAGS} ${ICU} ${ICUI18N} ${LIBZ} ${Boost_LIBRARIES} pthread curl)

webos_build_pkgconfig()
webos_build_library(NAME email-common)
#install(FILES files/etc/palm/filecache_types/attachment DESTINATION /etc/palm/filecache_types)
#install(FILES files/etc/palm/filecache_types/email DESTINATION /etc/palm/filecache_types)
