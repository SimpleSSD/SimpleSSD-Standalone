# Copyright (C) 2017 CAMELab
#
# This file is part of SimpleSSD.
#
# SimpleSSD is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# SimpleSSD is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with SimpleSSD.  If not, see <http://www.gnu.org/licenses/>.

# Set version and project name
cmake_minimum_required(VERSION 3.10)
project(simplessd-standalone)

# Validate build type
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Select CMake build type.")
endif ()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")

string (TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)

if (NOT uppercase_CMAKE_BUILD_TYPE MATCHES "^(DEBUG|RELEASE)$")
  message (FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif ()

# Validate build path
# https://github.com/llvm/llvm-project/blob/master/llvm/CMakeLists.txt#L249
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR AND NOT MSVC)
  message(FATAL_ERROR "In-source builds are not allowed.
Please create a directory and run cmake from there, passing the path
to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them.")
endif ()

# Add options for debug build
option(DRAMPOWER_VERBOSE "Show warnings from DRAMPower library." OFF)

# Set DRAMPower path
set(DRAMPOWER_SOURCE_DIR
  "${PROJECT_SOURCE_DIR}/lib/drampower/src"
  CACHE PATH
  ""
  FORCE
)

# Check SimpleSSD is valid
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/simplessd/CMakeLists.txt")
  message(FATAL_ERROR "Failed to find SimpleSSD. \
Check you initialized submodules.")
endif ()

# Check pthread
find_package(Threads REQUIRED)

# Add subproject
add_subdirectory(${PROJECT_SOURCE_DIR}/simplessd
  ${CMAKE_BINARY_DIR}/simplessd
)

# Set include directories
include_directories(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/simplessd
  ${DRAMPOWER_SOURCE_DIR}
)

# Make version
set(INPUT_VERSION_FILE "${PROJECT_SOURCE_DIR}/main/version.cc.in")
set(OUTPUT_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/version.cc")
set(PROJECT_DIRECTORY "${PROJECT_SOURCE_DIR}")
set(VERSION_TARGET "standalone_version")

include(${PROJECT_SOURCE_DIR}/simplessd/util/scripts/simplessd_version.cmake)

# Linker flags
set(LD_FLAGS "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Specify source files
set(SRC_DRIVER_NONE
  driver/none/none.cc
)
set(SRC_DRIVER_NVME
  driver/nvme/nvme.cc
  driver/nvme/prp.cc
  driver/nvme/queue.cc
)
set(SRC_IGL
  igl/block_io.cc
)
set(SRC_IGL_REQUEST
  igl/request/request_config.cc
  igl/request/request_generator.cc
)
set(SRC_IGL_TRACE
  igl/trace/trace_config.cc
  igl/trace/trace_replayer.cc
)
set(SRC_LIB_DRAMPOWER
  lib/drampower/src/CAHelpers.cc
  lib/drampower/src/CmdHandlers.cc
  lib/drampower/src/CmdScheduler.cc
  lib/drampower/src/CommandAnalysis.cc
  lib/drampower/src/MemArchitectureSpec.cc
  lib/drampower/src/MemBankWiseParams.cc
  lib/drampower/src/MemCommand.cc
  lib/drampower/src/MemoryPowerModel.cc
  lib/drampower/src/MemorySpecification.cc
  lib/drampower/src/MemPowerSpec.cc
  lib/drampower/src/MemTimingSpec.cc
  lib/drampower/src/Parameter.cc
  lib/drampower/src/Parametrisable.cc
  lib/drampower/src/TraceParser.cc
  lib/drampower/src/libdrampower/LibDRAMPower.cc
)
set(SRC_MAIN
  main/config_reader.cc
  main/config.cc
  main/engine.cc
  main/main.cc
  main/signal.cc
)
set(SRC_UTIL
  util/print.cc
  util/stopwatch.cc
)

# Source group for MSVC
SOURCE_GROUP("Source Files\\driver\\none" FILES ${SRC_DRIVER_NONE})
SOURCE_GROUP("Source Files\\driver\\nvme" FILES ${SRC_DRIVER_NVME})
SOURCE_GROUP("Source Files\\igl" FILES ${SRC_IGL})
SOURCE_GROUP("Source Files\\igl\\request" FILES ${SRC_IGL_REQUEST})
SOURCE_GROUP("Source Files\\igl\\trace" FILES ${SRC_IGL_TRACE})
SOURCE_GROUP("Source Files\\lib\\drampower" FILES ${SRC_LIB_DRAMPOWER})
SOURCE_GROUP("Source Files\\main" FILES ${SRC_MAIN})
SOURCE_GROUP("Source Files\\util" FILES ${SRC_UTIL})

# Define executable
add_executable(simplessd-standalone
  ${SRC_DRIVER_NVME}
  ${SRC_DRIVER_NONE}
  ${SRC_IGL}
  ${SRC_IGL_REQUEST}
  ${SRC_IGL_TRACE}
  ${SRC_LIB_DRAMPOWER}
  ${SRC_MAIN}
  ${SRC_UTIL}
  ${OUTPUT_VERSION_FILE}
)

# DRAMPower warning
if (NOT DRAMPOWER_VERBOSE)
  target_compile_definitions(simplessd-standalone PRIVATE -DDRAMPOWER_DISABLE_WARNING)
endif ()

# Platform specific settings
if (MSVC)
  target_compile_definitions(simplessd-standalone PRIVATE -D_CRT_SECURE_NO_WARNINGS)
  target_compile_options(simplessd-standalone PRIVATE /wd4819)  # Surpress unicode warning
else ()
  # Check libunwind/libelf
  include(CheckIncludeFiles)

  check_include_files("libunwind.h" HAVE_UNWIND_H)
  check_include_files("elfutils/libdwfl.h" HAVE_LIBDWFL_H)

  if (NOT HAVE_UNWIND_H)
    message(FATAL_ERROR "Please install libunwind-dev or libunwind-devel.")
  endif ()
  if (NOT HAVE_LIBDWFL_H)
    message(FATAL_ERROR "Please install libdw-dev or elfutils-devel.")
  endif ()

  target_compile_options(simplessd-standalone PRIVATE -g -Wall -Wextra -Werror)

  set(LD_FLAGS "-ldw -lunwind -lstdc++fs")
  set(CMAKE_CXX_FLAGS "-D__FILENAME__='\"$(subst ${PROJECT_SOURCE_DIR}/,,$(abspath $<))\"' ${CMAKE_CXX_FLAGS}")

  if (uppercase_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    target_compile_definitions(simplessd-standalone PRIVATE -DSIMPLESSD_STANDALONE_DEBUG)
    target_compile_options(simplessd-standalone PRIVATE -O0)
  else ()
    target_compile_options(simplessd-standalone PRIVATE -O2)
  endif ()
endif ()

# Force version checking in every build
add_dependencies(simplessd-standalone ${VERSION_TARGET})

# Link libraries
target_link_libraries(simplessd-standalone
  simplessd
  ${CMAKE_THREAD_LIBS_INIT}
  ${LD_FLAGS}
)
