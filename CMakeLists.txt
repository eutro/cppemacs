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

cmake_minimum_required(VERSION 3.23) # for FILE_SET install

project(cppemacs
  VERSION 0.1.0
  LANGUAGES CXX)

# Determine if we are the main project
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MAIN_PROJECT ON)
endif()

list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/cmake"
  "${CMAKE_CURRENT_LIST_DIR}/cmake/vendored")

include(FetchContent)

# Options
include(GNUInstallDirs)

option(CPPEMACS_Test "Run cppemacs tests" ${MAIN_PROJECT})
option(CPPEMACS_Coverage "Test with code coverage" OFF)
option(CPPEMACS_Documentation "Generate documentation" ${MAIN_PROJECT})
option(CPPEMACS_Install "Add cppemacs as an install target" ${MAIN_PROJECT})

set(CPPEMACS_TARGET_NAME ${PROJECT_NAME})
set(CPPEMACS_LICENSE_FILE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPPEMACS_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}" CACHE INTERNAL "")
set(CPPEMACS_TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(CPPEMACS_PROJECT_CONFIG_IN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.in.cmake")
set(CPPEMACS_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
set(CPPEMACS_PROJECT_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Version.cmake")
set(CPPEMACS_PROJECT_TARGETS_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")

# cppemacs target
set(CPPEMACS_HEADERS
  cppemacs/all.hpp
  cppemacs/core.hpp
  cppemacs/conversions.hpp
  cppemacs/utils.hpp
  cppemacs/literals.hpp)
add_library(${CPPEMACS_TARGET_NAME} INTERFACE)
target_sources(${CPPEMACS_TARGET_NAME} PUBLIC
  FILE_SET HEADERS TYPE HEADERS
  BASE_DIRS .
  FILES ${CPPEMACS_HEADERS}
)
target_compile_features(${CPPEMACS_TARGET_NAME} INTERFACE cxx_std_11)

# Export target for other projects to use
export(TARGETS ${CPPEMACS_TARGET_NAME}
  NAMESPACE ${PROJECT_NAME}::
  FILE ${CPPEMACS_PROJECT_TARGETS_FILE}
)

if (CPPEMACS_Documentation)
  find_package(Doxygen
    COMPONENTS dot)
  if (Doxygen_FOUND)
    message(STATUS "Doxygen found")

    FetchContent_Declare(DoxygenAwesome
      GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css
      GIT_TAG v2.3.3)
    FetchContent_MakeAvailable(DoxygenAwesome)

    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_DISABLE_INDEX NO)
    set(DOXYGEN_FULL_SIDEBAR NO)
    set(DOXYGEN_HTML_COLORSTYLE LIGHT)
    set(DOXYGEN_HTML_EXTRA_STYLESHEET
      "${doxygenawesome_SOURCE_DIR}/doxygen-awesome.css"
      "${doxygenawesome_SOURCE_DIR}/doxygen-awesome-sidebar-only.css"
      "${doxygenawesome_SOURCE_DIR}/doxygen-awesome-sidebar-only-darkmode-toggle.css")
    set(DOXYGEN_HTML_EXTRA_FILES
      "${doxygenawesome_SOURCE_DIR}/doxygen-awesome-darkmode-toggle.js"
      "${doxygenawesome_SOURCE_DIR}/doxygen-awesome-fragment-copy-button.js"
      "${doxygenawesome_SOURCE_DIR}/doxygen-awesome-paragraph-link.js"
      "${doxygenawesome_SOURCE_DIR}/doxygen-awesome-interactive-toc.js")
    set(DOXYGEN_HTML_HEADER
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/header.html")

    set(DOXYGEN_DOT_IMAGE_FORMAT svg)
    set(DOXYGEN_DOT_TRANSPARENT YES)

    set(DOXYGEN_PROJECT_BRIEF "C++11 Emacs module API wrapper")
    set(DOXYGEN_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
    set(DOXYGEN_PREDEFINED
      EMACS_MAJOR_VERSION=29
      CPPEMACS_DOXYGEN_RUNNING)
    set(DOXYGEN_INLINE_SOURCES NO)

    doxygen_add_docs(${PROJECT_NAME}_docs
      ${CPPEMACS_HEADERS}
      ALL USE_STAMP_FILE
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif ()
endif ()

if (CPPEMACS_Test)
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v3.6.0
    # do not use FindPackage, we need the shared library, which we
    # likely don't have on the system
  )
  set(BUILD_SHARED_LIBS ON)
  FetchContent_MakeAvailable(Catch2)
  if(NOT Catch2_FOUND)
    message(STATUS "Catch2: (Fetched)")
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
  else()
    message(STATUS "Catch2: (System)")
  endif()

  include(CTest)
  include(Catch)

  set(CPPEMACS_TEST_TARGET ${PROJECT_NAME}_test)
  set(CPPEMACS_TEST2_TARGET ${PROJECT_NAME}_test_cxx_11)

  add_library(${CPPEMACS_TEST_TARGET} SHARED
    tests/main.cpp
    tests/listeners.cpp
    tests/test_conversions.cpp
    tests/test_user_ptr.cpp
    tests/test_vector.cpp
    tests/test_exceptions.cpp
  )
  target_precompile_headers(${CPPEMACS_TEST_TARGET}
    PRIVATE tests/common.hpp)
  target_link_libraries(${CPPEMACS_TEST_TARGET} PRIVATE
    cppemacs Catch2::Catch2)

  # test with strict C++11 compliance
  add_executable(${CPPEMACS_TEST2_TARGET} tests/test_inst_all.cpp)
  target_link_libraries(${CPPEMACS_TEST2_TARGET} PRIVATE cppemacs)
  set_target_properties(${CPPEMACS_TEST2_TARGET} PROPERTIES CXX_STANDARD 11)
  set_target_properties(${CPPEMACS_TEST2_TARGET} PROPERTIES CXX_STANDARD_REQUIRED ON)
  set_target_properties(${CPPEMACS_TEST2_TARGET} PROPERTIES CXX_EXTENSIONS OFF)
  add_test(NAME ${CPPEMACS_TEST2_TARGET}
    COMMAND ${CPPEMACS_TEST2_TARGET})

  find_program(EMACS_PATH emacs)
  if (NOT EMACS_PATH)
    message(FATAL_ERROR "Emacs not found! Aborting...")
  endif ()

  add_test(NAME ${CPPEMACS_TEST_TARGET}
    COMMAND ${EMACS_PATH} -Q --batch --script
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test.el"
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

endif()

if (CPPEMACS_Install)
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "${CPPEMACS_PROJECT_CONFIG_IN_FILE}"
    "${CPPEMACS_PROJECT_CONFIG_FILE}"
    INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/pkgconfig"
  )
  write_basic_package_version_file(
    "${CPPEMACS_PROJECT_VERSION_FILE}"
    COMPATIBILITY AnyNewerVersion
    ARCH_INDEPENDENT
  )

  # Install headers
  install(TARGETS ${CPPEMACS_TARGET_NAME}
    EXPORT ${CPPEMACS_TARGETS_EXPORT_NAME}
    FILE_SET HEADERS
  )
  # Install *Config.cmake and *Version.cmake
  install(FILES ${CPPEMACS_PROJECT_CONFIG_FILE} ${CPPEMACS_PROJECT_VERSION_FILE}
    DESTINATION ${CPPEMACS_CONFIG_INSTALL_DIR}
  )
  # Install license file with headers
  install(FILES ${CPPEMACS_LICENSE_FILE}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cppemacs")

  # Install export file
  install(EXPORT ${CPPEMACS_TARGETS_EXPORT_NAME}
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CPPEMACS_CONFIG_INSTALL_DIR}
  )

  # CPack
  set(CPACK_PACKAGE_VENDOR "Eutro")
  set(CPACK_RESOURCE_FILE_LICENSE "${CPPEMACS_LICENSE_FILE}")
  include(CPack)
endif()
