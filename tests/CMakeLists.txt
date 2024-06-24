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

message(STATUS "Building tests")

set(BUILD_SHARED_LIBS ON)
FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG v3.6.0
  # do not use FindPackage, we need the shared library, which we
  # likely don't have on the system
)

FetchContent_GetProperties(Catch2)
if(NOT Catch2_POPULATED)
  FetchContent_Populate(Catch2)
endif()
add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

include(Catch)

set(CPPEMACS_TEST_TARGET ${PROJECT_NAME}_test)
set(CPPEMACS_TEST2_TARGET ${PROJECT_NAME}_test_cxx_11)

add_library(${CPPEMACS_TEST_TARGET} SHARED
  main.cpp
  listeners.cpp
  test_conversions.cpp
  test_user_ptr.cpp
  test_vector.cpp
  test_exceptions.cpp
)

if (NOT CMAKE_VERSION VERSION_LESS "3.16")
  target_precompile_headers(${CPPEMACS_TEST_TARGET}
    PRIVATE common.hpp)
endif()

target_link_libraries(${CPPEMACS_TEST_TARGET} PRIVATE
  cppemacs Catch2::Catch2)

# test with strict C++11 compliance
add_executable(${CPPEMACS_TEST2_TARGET} test_inst_all.cpp)
target_link_libraries(${CPPEMACS_TEST2_TARGET} PRIVATE cppemacs)
set_target_properties(${CPPEMACS_TEST2_TARGET} PROPERTIES CXX_STANDARD 11)
set_target_properties(${CPPEMACS_TEST2_TARGET} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${CPPEMACS_TEST2_TARGET} PROPERTIES CXX_EXTENSIONS OFF)

add_test(NAME ${CPPEMACS_TEST2_TARGET}
  COMMAND ${CPPEMACS_TEST2_TARGET})

if (NOT Emacs_FOUND)
  message(FATAL_ERROR "Emacs not found! Aborting...")
endif ()

add_test(NAME ${CPPEMACS_TEST_TARGET}
  COMMAND Emacs::emacs -Q --batch --script
  "${CMAKE_CURRENT_SOURCE_DIR}/test.el"
  "$<TARGET_FILE:${CPPEMACS_TEST_TARGET}>")

if (CPPEMACS_Coverage)
  include(CodeCoverage)
  message(STATUS "Testing with code coverage")
  append_coverage_compiler_flags_to_target(${CPPEMACS_TEST_TARGET})
  set(CPPEMACS_SETUP_COVERAGE_ARGS
    EXECUTABLE ctest
    DEPENDENCIES ${CPPEMACS_TEST_TARGET}
    EXCLUDE /usr ${CMAKE_BINARY_DIR})

  if (NOT ((LCOV_PATH AND GENHTML_PATH)
        OR GCOVR_PATH
        OR FASTCOV_PATH))
    message(FATAL_ERROR "No coverage frontend (lcov + genhtml, gcovr, or fastcov)")
  endif ()

  if (LCOV_PATH AND GENHTML_PATH)
    message(STATUS "Coverage: have lcov")
    setup_target_for_coverage_lcov(NAME cppemacs_coverage_lcov
      ${CPPEMACS_SETUP_COVERAGE_ARGS})
  endif ()
  if (GCOVR_PATH)
    message(STATUS "Coverage: have gcovr")
    setup_target_for_coverage_gcovr_xml(NAME cppemacs_coverage_gcovr_xml
      ${CPPEMACS_SETUP_COVERAGE_ARGS})
    setup_target_for_coverage_gcovr_html(NAME cppemacs_coverage_gcovr_html
      ${CPPEMACS_SETUP_COVERAGE_ARGS})
  endif ()
  if (FASTCOV_PATH)
    message(STATUS "Coverage: have fastcov")
    if (GENHTML_PATH)
      set(CPPEMACS_FASTCOV_ARGS)
    else ()
      set(CPPEMACS_FASTCOV_ARGS SKIP_HTML)
    endif ()
    setup_target_for_coverage_fastcov(NAME cppemacs_coverage_fastcov
      ${CPPEMACS_SETUP_COVERAGE_ARGS}
      ${CPPEMACS_FASTCOV_ARGS})
  endif ()
  
endif ()
