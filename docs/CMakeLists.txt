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

if (${CMAKE_VERSION} VERSION_LESS 3.13)
  message(WARNING "CMake >= 3.13 required to build documentation")
  return()
endif()

find_package(Doxygen
  COMPONENTS dot)

if (Doxygen_FOUND)
  message(STATUS "Doxygen found -- building documentation")

  FetchContent_Declare(DoxygenAwesome
    GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css
    GIT_TAG v2.3.3)
  FetchContent_GetProperties(DoxygenAwesome)
  if(NOT DoxygenAwesome_POPULATED)
    FetchContent_Populate(DoxygenAwesome)
  endif()

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
    "${CMAKE_CURRENT_SOURCE_DIR}/header.html")

  set(DOXYGEN_DOT_IMAGE_FORMAT svg)
  set(DOXYGEN_DOT_TRANSPARENT YES)

  set(DOXYGEN_PROJECT_BRIEF "C++11 Emacs module API wrapper")
  set(DOXYGEN_INCLUDE_PATH "${CPPEMACS_BUILD_INDCLUDE_DIR}")
  set(DOXYGEN_PREDEFINED
    EMACS_MAJOR_VERSION=29
    CPPEMACS_DOXYGEN_RUNNING)
  set(DOXYGEN_INLINE_SOURCES NO)

  set(DOXYGEN_OUTPUT_DIRECTORY "${CPPEMACS_DOCS_BUILD_DIR}")

  doxygen_add_docs(${PROJECT_NAME}_docs
    ${CPPEMACS_HEADERS}
    ALL USE_STAMP_FILE
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/..")
else ()
  message(STATUS "Doxygen not found -- cannot build documentation")
endif ()
