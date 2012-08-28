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

project(mojomail-imap CXX)

include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC2)
webos_component(2 0 0)

set(CMAKE_EXE_LINKER_FLAGS, "-Wl --unresolved-symbols=ignore-in-shared-libs")
webos_add_compiler_flags(ALL "-DMOJ_LINUX -DMOJ_X86")
webos_add_compiler_flags(ALL "-O -ggdb -Wuninitialized -Winit-self")

webos_machine_dep()

include_directories(inc)

# These variables can change the default behaviour and need to be set before calling find_package
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS regex)
include_directories(Boost_INCLUDE_DIRS)

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS})

pkg_check_modules(LUNASERVICE REQUIRED luna-service2)
include_directories(${LUNASERVICE_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LUNASERVICE_CFLAGS})

pkg_check_modules(CJSON cjson)
include_directories(${CJSON_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${CJSON_CFLAGS})

pkg_check_modules(MOJODB mojodb)
include_directories(${MOJODB_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${MOJODB_CFLAGS})

pkg_check_modules(EMAILCOMMON email-common)
include_directories(${EMAILCOMMON_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${EMAILCOMMON_CFLAGS})

pkg_check_modules(PALMSOCKET REQUIRED palmsocket)
include_directories(${PALMSOCKET_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PALMSOCKET_CFLAGS})

pkg_check_modules(PMSTATEMACHINE REQUIRED PmStateMachineEngine)
include_directories(${PMSTATEMACHINE_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMSTATEMACHINE_CFLAGS})

pkg_check_modules(SANDBOX REQUIRED sandbox)
include_directories(${SANDBOX_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${SANDBOX_CFLAGS})

pkg_check_modules(PMLOG REQUIRED PmLogLib)
include_directories(${PMLOG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOG_CFLAGS})

pkg_check_modules(CARES REQUIRED libcares)
include_directories(${CARES_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${CARES_CFLAGS})

find_library(ICU NAMES icuuc)
if(ICU STREQUAL "ICU-NOTFOUND")
   message(FATAL_ERROR "Failed to find ICU4C libaries. Please intall.")
endif()

aux_source_directory(src/activity activity_files)
aux_source_directory(src/client client_files)
aux_source_directory(src/commands commands_files)
aux_source_directory(src/connection connection_files)
aux_source_directory(src/data data_files)
aux_source_directory(src/exceptions exceptions_files)
aux_source_directory(src/parser parser_files)
aux_source_directory(src/sync sync_files)
aux_source_directory(src/protocol protocol_files)
list(REMOVE_ITEM protocol_files "src/protocol/SimpleResponseParser.cpp")

add_executable(mojomail-imap src/ImapBusDispatcher.cpp src/ImapConfig.cpp src/ImapValidator.cpp src/ImapClient.cpp src/ImapServiceApp.cpp ${activity_files} ${client_files} ${commands_files} ${connection_files} ${data_files} ${exceptions_files} ${parser_files} ${sync_files} ${protocol_files}) 

target_link_libraries(mojomail-imap ${GLIB2_LDFLAGS} ${MOJODB_LDFLAGS} ${ICU} ${EMAILCOMMON_LDFLAGS} ${PALMSOCKET_LDFLAGS} ${SANDBOX_LDFLAGS} ${PMLOG_LDFLAGS} ${CARES_LDFLAGS} ${PMSTATEMACHINE_LDFLAGS} ${Boost_LIBRARIES}) 

webos_build_program(NAME mojomail-imap)
webos_build_system_bus_files()
webos_build_db8_files()
