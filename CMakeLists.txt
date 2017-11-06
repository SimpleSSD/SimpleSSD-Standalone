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
#
# Authors: Donghyun Gouk <kukdh1@camelab.org>

# Header
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (SimpleSSD-Standalone)
SET_PROPERTY (GLOBAL PROPERTY USE_FOLDERS ON)

# Set parameters
SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
ADD_DEFINITIONS (-DSIMPLESSD_STANDALONE)

# Include Directories
INCLUDE_DIRECTORIES (
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/simplessd"
)

# Platform specific
IF (MSVC)
  ADD_DEFINITIONS (-D_CRT_SECURE_NO_WARNINGS)
  OPTION (USE_STATIC_RUNTIME "Link with static runtime libraries" ON)
  IF (USE_STATIC_RUNTIME)
    FOREACH (flag_var
      CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      IF (${flag_var} MATCHES "/MD")
        STRING (REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      ENDIF ()
    ENDFOREACH ()
  ENDIF ()
ELSE ()
  SET (CMAKE_CXX_FLAGS "-O2 ${CMAKE_CXX_FLAGS}")
  IF (CMAKE_VERSION VERSION_LESS "3.1")
    IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      SET (CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
    ENDIF ()
  ELSE ()
    SET (CMAKE_CXX_STANDARD 11)
    set (CMAKE_CXX_STANDARD_REQUIRED ON)
  ENDIF ()
ENDIF ()

# Source files
SET (GEM5_COMPATIBLE_SOURCE_FILES
  base/types.cc
)

SET (GEM5_COMPATIBLE_INCLUDE_FILES
  base/types.hh
  base/trace.hh
  base/misc.hh
)

SET (STANDALONE_SOURCE_FILES
  ssd_sim.cpp
  Simulator.cpp
  config.cc
  hil.cpp
  trace.cc
  rgen.cc
)

SET (STANDALONE_INCLUDE_FILES
  ssd_sim.h
  Simulator.h
  config.hh
  hil.h
  trace.hh
  rgen.hh
)

SET (SIMPLESSD_SOURCE_FILES
  simplessd/ini.c
  simplessd/base_config.cc
  simplessd/ftl_block.cc
  simplessd/ftl_hybridmapping.cc
  simplessd/ftl_mappingtable.cc
  simplessd/ftl_statistics.cc
  simplessd/ftl.cc
  simplessd/Latency.cc
  simplessd/LatencySLC.cc
  simplessd/LatencyMLC.cc
  simplessd/LatencyTLC.cc
  simplessd/PAL2_TimeSlot.cc
  simplessd/PAL2.cc
  simplessd/PALStatistics.cc
)

SET (SIMPLESSD_INCLUDE_FILES
  simplessd/ini.h
  simplessd/base_config.hh
  simplessd/ftl_block.hh
  simplessd/ftl_command.hh
  simplessd/ftl_defs.hh
  simplessd/ftl_hybridmapping.hh
  simplessd/ftl_mappingtable.hh
  simplessd/ftl_statistics.hh
  simplessd/ftl.hh
  simplessd/Latency.h
  simplessd/LatencySLC.h
  simplessd/LatencyMLC.h
  simplessd/LatencyTLC.h
  simplessd/PAL2_TimeSlot.h
  simplessd/PAL2.h
  simplessd/PALStatistics.h
  simplessd/SimpleSSD_types.h
)

SOURCE_GROUP("Header Files\\Standalone" FILES ${STANDALONE_INCLUDE_FILES})
SOURCE_GROUP("Header Files\\Standalone\\base" FILES ${GEM5_COMPATIBLE_INCLUDE_FILES})
SOURCE_GROUP("Header Files\\SimpleSSD" FILES ${SIMPLESSD_INCLUDE_FILES})
SOURCE_GROUP("Source Files\\Standalone" FILES ${STANDALONE_SOURCE_FILES})
SOURCE_GROUP("Source Files\\Standalone\\base" FILES ${GEM5_COMPATIBLE_SOURCE_FILES})
SOURCE_GROUP("Source Files\\SimpleSSD" FILES ${SIMPLESSD_SOURCE_FILES})
ADD_EXECUTABLE (
  SimpleSSD-Standalone
  ${STANDALONE_INCLUDE_FILES}
  ${GEM5_COMPATIBLE_INCLUDE_FILES}
  ${SIMPLESSD_INCLUDE_FILES}
  ${STANDALONE_SOURCE_FILES}
  ${GEM5_COMPATIBLE_SOURCE_FILES}
  ${SIMPLESSD_SOURCE_FILES}
)
