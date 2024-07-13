# Copyright (C) 2024 Eutro <https://eutro.dev>
#
# This file is part of cppemacs.
#
# cppemacs is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cppemacs is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cppemacs. If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-FileCopyrightText: 2024 Eutro <https://eutro.dev>
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.0...3.27)

project(cppemacs
  VERSION 0.1.0
  LANGUAGES CXX)

### Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
  message(FATAL_ERROR "You cannot build in a source directory (or any directory with "
    "CMakeLists.txt file). Please make a build subdirectory. Feel free to "
    "remove CMakeCache.txt and CMakeFiles.")
endif()

# Determine if we are the main project
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MAIN_PROJECT ON)
endif()

list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/cmake"
  "${CMAKE_CURRENT_LIST_DIR}/cmake/vendored")

# Options
include(GNUInstallDirs)

option(CPPEMACS_Test "Build cppemacs tests" ${MAIN_PROJECT})
option(CPPEMACS_Coverage "Test with code coverage" OFF)
option(CPPEMACS_Documentation "Generate documentation" OFF)
option(CPPEMACS_Install "Add cppemacs installation rules" ${MAIN_PROJECT})

set(CPPEMACS_TARGET_NAME ${PROJECT_NAME})
set(CPPEMACS_LICENSE_FILE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPPEMACS_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}" CACHE INTERNAL "")
set(CPPEMACS_TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(CPPEMACS_PROJECT_CONFIG_IN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.in.cmake")
set(CPPEMACS_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
set(CPPEMACS_PROJECT_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Version.cmake")
set(CPPEMACS_PROJECT_TARGETS_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")

set(CPPEMACS_INCLUDE_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(CPPEMACS_INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}")
set(CPPEMACS_DOCS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs")

# cppemacs target
set(CPPEMACS_HEADERS
  include/cppemacs/all.hpp
  include/cppemacs/core.hpp
  include/cppemacs/conversions.hpp
  include/cppemacs/utils.hpp
  include/cppemacs/literals.hpp)

add_library(${CPPEMACS_TARGET_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${CPPEMACS_TARGET_NAME} ALIAS ${CPPEMACS_TARGET_NAME})
if (${CMAKE_VERSION} VERSION_LESS "3.8")
  target_compile_features(${CPPEMACS_TARGET_NAME} INTERFACE cxx_range_for)
else()
  target_compile_features(${CPPEMACS_TARGET_NAME} INTERFACE cxx_std_11)
endif()

target_include_directories(${CPPEMACS_TARGET_NAME} INTERFACE
  $<BUILD_INTERFACE:${CPPEMACS_INCLUDE_BUILD_DIR}>
  $<INSTALL_INTERFACE:${CPPEMACS_INCLUDE_INSTALL_DIR}>
)

find_package(Emacs)
if (Emacs_FOUND)
  target_link_libraries(${CPPEMACS_TARGET_NAME} INTERFACE Emacs::emacs_module)
else ()
  message(WARN "Emacs not found on this machine")
endif ()

# Export target for other projects to use
export(TARGETS ${CPPEMACS_TARGET_NAME}
  NAMESPACE ${PROJECT_NAME}::
  FILE ${CPPEMACS_PROJECT_TARGETS_FILE}
)

if (CPPEMACS_Documentation OR CPPEMACS_Test)
  if (${CMAKE_VERSION} VERSION_LESS 3.11)
    message(WARNING "CMake >= 3.11 required to build tests or documentation")
    return()
  endif()

  include(FetchContent)
endif()

if (CPPEMACS_Documentation)
  add_subdirectory(docs)
endif()
if (CPPEMACS_Test)
  include(CTest)
  add_subdirectory(tests)
endif()
if (CPPEMACS_Install)
  include(install.cmake)
endif()
