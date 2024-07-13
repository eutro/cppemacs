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
  if (CMAKE_VERSION VERSION_LESS "3.14")
    FetchContent_GetProperties(DoxygenAwesome)
    if (NOT DoxygenAwesome_POPULATED)
      FetchContent_Populate(DoxygenAwesome)
    endif()
  else()
    FetchContent_MakeAvailable(DoxygenAwesome)
  endif()

  set(DOXYGEN_HTML_DYNAMIC_SECTIONS YES)
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
  set(DOXYGEN_LAYOUT_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/layout.xml")

  set(DOXYGEN_EXCLUDE_SYMBOLS
    CPPEMACS_HAVE_NOEXCEPT_TYPEDEFS
    CPPEMACS_HAVE_RETURN_TYPE_DEDUCTION
    CPPEMACS_HAVE_STRING_VIEW
    CPPEMACS_HAVE_IS_INVOCABLE)

  set(DOXYGEN_DOT_IMAGE_FORMAT svg)
  set(DOXYGEN_DOT_TRANSPARENT YES)

  set(DOXYGEN_PROJECT_BRIEF "C++11 Emacs module API wrapper")
  set(DOXYGEN_PREDEFINED
    EMACS_MAJOR_VERSION=29
    CPPEMACS_DOXYGEN_RUNNING)
  set(DOXYGEN_INLINE_SOURCES NO)
  set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
  set(DOXYGEN_GROUP_NESTED_COMPOUNDS YES)
  set(DOXYGEN_ALWAYS_DETAILED_SEC YES)

  # the manual is under GNU Free Documentation License
  # TODO: cppemacs documentation should probably be too
  option(CPPEMACS_Elisp_manual_embed
    "Embed the Emacs documentation HTML pages into the output."
    OFF)
  option(CPPEMACS_Elisp_manual_embed_source
    "URL to fetch the Elisp manual from, if embedding."
    "https://www.gnu.org/software/emacs/manual/elisp.html_node.tar.gz")

  if (CPPEMACS_Elisp_manual_embed)
    set(CPPEMACS_ELISP_MANUAL_LOCATION "elisp")
    include(ExternalProject)
    ExternalProject_Add(ElispManual
      URL "${CPPEMACS_Elisp_manual_embed_source}"
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND "")

    ExternalProject_Get_Property(ElispManual SOURCE_DIR)
    add_custom_command(OUTPUT copy_elisp_manual.stamp POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
      "${SOURCE_DIR}" "${CPPEMACS_DOCS_BUILD_DIR}/html/elisp"
      COMMAND ${CMAKE_COMMAND} -E touch copy_elisp_manual.stamp
      DEPENDS "${SOURCE_DIR}"
    )
    add_custom_target(copy_elisp_manual ALL
      DEPENDS copy_elisp_manual.stamp)
  else()
    set(CPPEMACS_ELISP_MANUAL_LOCATION
      "https://www.gnu.org/software/emacs/manual/html_node/elisp")
  endif()

  set(DOXYGEN_ALIASES
    "manlink{2}=[\\1](${CPPEMACS_ELISP_MANUAL_LOCATION}/\\2)"
    "manual{2}=See the manual: @manlink{\\1,\\2}."
    "manual{1}=See the @manlink{manual,\\1}."
  )

  set(CPPEMACS_EXAMPLES
    hello_world.cpp)

  list(TRANSFORM CPPEMACS_EXAMPLES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/examples/")

  # add a target for examples, but don't build them by default (this
  # brings them in for compile_commands.json and lets us build them if
  # we want to)
  add_library(${PROJECT_NAME}_examples OBJECT ${CPPEMACS_EXAMPLES})
  target_link_libraries(${PROJECT_NAME}_examples PRIVATE cppemacs)
  set_target_properties(${PROJECT_NAME}_examples PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    EXCLUDE_FROM_ALL ON
  )

  set(DOXYGEN_EXAMPLE_PATH "${CPPEMACS_EXAMPLES}")

  set(DOXYGEN_OUTPUT_DIRECTORY "${CPPEMACS_DOCS_BUILD_DIR}")
  set(DOXYGEN_STRIP_FROM_INC_PATH "${CPPEMACS_INCLUDE_BUILD_DIR}")

  doxygen_add_docs(${PROJECT_NAME}_docs
    "${CPPEMACS_HEADERS}" "${CPPEMACS_EXAMPLES}"
    ALL USE_STAMP_FILE
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/..")
else ()
  message(STATUS "Doxygen not found -- cannot build documentation")
endif ()
