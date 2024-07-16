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

FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG v3.6.0
  # do not use FindPackage, we need the shared library, which we
  # likely don't have on the system
)

if (CMAKE_VERSION VERSION_LESS "3.14")
  FetchContent_GetProperties(Catch2)
  if (NOT Catch2_POPULATED)
    FetchContent_Populate(Catch2)
  endif()
else()
  FetchContent_MakeAvailable(Catch2)
endif()

#add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
#list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

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
set_target_properties(${CPPEMACS_TEST_TARGET} PROPERTIES
  CXX_STANDARD 20 # attempt to use C++20
)
set_target_properties(Catch2 PROPERTIES
  CXX_STANDARD 20 # ditto for Catch2
  POSITION_INDEPENDENT_CODE ON # needed to link into shared library
)

if (NOT CMAKE_VERSION VERSION_LESS "3.16")
  target_precompile_headers(${CPPEMACS_TEST_TARGET}
    PRIVATE common.hpp)
endif()

target_link_libraries(${CPPEMACS_TEST_TARGET} PRIVATE
  cppemacs Catch2::Catch2)

find_package(GMP) # optional GMP dependency
if (GMP_FOUND)
  message(STATUS "Testing with GMP")
  target_link_libraries(${CPPEMACS_TEST_TARGET} PRIVATE
    GMP::GMP GMP::GMPXX)
  target_compile_definitions(${CPPEMACS_TEST_TARGET} PRIVATE
    CPPEMACS_ENABLE_GMPXX=1)
endif()

set(CPPEMACS_TEST_TARGETS
  ${CPPEMACS_TEST_TARGET}
  ${CPPEMACS_TEST2_TARGET}
)

# test with strict C++11 compliance
add_executable(${CPPEMACS_TEST2_TARGET} test_inst_all.cpp)
target_link_libraries(${CPPEMACS_TEST2_TARGET} PRIVATE cppemacs)
set_target_properties(${CPPEMACS_TEST2_TARGET} PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF)

add_test(NAME ${CPPEMACS_TEST2_TARGET}
  COMMAND ${CPPEMACS_TEST2_TARGET})

if (NOT Emacs_FOUND)
  message(FATAL_ERROR "Emacs not found! Aborting...")
endif ()

add_test(NAME ${CPPEMACS_TEST_TARGET}
  COMMAND Emacs::emacs -Q --batch --script
  "${CMAKE_CURRENT_SOURCE_DIR}/test.el"
  "$<TARGET_FILE:${CPPEMACS_TEST_TARGET}>")

if (CPPEMACS_Examples)
  list(APPEND CPPEMACS_TEST_TARGETS "${PROJECT_NAME}_examples")
  add_test(
    NAME ${PROJECT_NAME}_doctest
    COMMAND "${CMAKE_COMMAND}"
    --build ${CMAKE_BINARY_DIR}
    --target "${PROJECT_NAME}_examples_load_all"
  )
endif()

if (CPPEMACS_Coverage)
  include(CodeCoverage)
  message(STATUS "Testing with code coverage")
  append_coverage_compiler_flags_to_target(${CPPEMACS_TEST_TARGET})
  set(CPPEMACS_SETUP_COVERAGE_ARGS
    EXECUTABLE ctest
    DEPENDENCIES ${CPPEMACS_TEST_TARGETS}
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
