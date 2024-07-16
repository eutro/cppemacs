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

if("${CMAKE_MAJOR_VERSION}" LESS 3)
  message(FATAL_ERROR "CMake >= 3.0.0 required")
endif()

cmake_policy(PUSH)
cmake_policy(VERSION 3.0...3.27)

if(NOT CMAKE_VERSION VERSION_LESS "3.19.0")
  set(_Emacs_HAVE_PACKAGE_VERSION_RANGE TRUE)
  list(APPEND _Emacs_FIND_PACKAGE_EXTRA_ARGS
    HANDLE_VERSION_RANGE)
endif()

include(FindPackageHandleStandardArgs)

function(_Emacs_get_version emacs_version result_var emacs_path)
  execute_process(
    COMMAND "${emacs_path}" --quick --batch --eval [[(princ emacs-version)]]
    OUTPUT_VARIABLE full_emacs_version
    ERROR_VARIABLE error_msg
    RESULT_VARIABLE version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  set(${result_var} ${version_result} PARENT_SCOPE)
  set(${emacs_version} ${full_emacs_version} PARENT_SCOPE)
endfunction()

function(_Emacs_version_validator version_match emacs_path)
  if (NOT DEFINED Emacs_FIND_VERSION)
    set(${version_match} TRUE PARENT_SCOPE)
  else()
    _Emacs_get_version(candidate_version version_result "${emacs_path}")

    if (${version_result} EQUAL 0)
      if (_Emacs_HAVE_PACKAGE_VERSION_RANGE)
        find_package_check_version("${candidate_version}" valid_emacs_version
          HANDLE_VERSION_RANGE)
      else()
        if ((NOT DEFINED Emacs_VERSION)
            OR ("${PACKAGE_FIND_VERSION}" VERSION_LESS "${candidate_version}"))
          set(valid_emacs_version TRUE)
        else()
          set(valid_emacs_version FALSE)
        endif()
      endif()

      set(${version_match} "${valid_emacs_version}" PARENT_SCOPE)
    else()
      set(${version_match} FALSE PARENT_SCOPE)
    endif()
  endif()
endfunction()

macro(_Emacs_find_emacs)
  find_program(
    Emacs_EXECUTABLE NAMES emacs
    DOC "Emacs text editor"
    HINTS "${_Emacs_SEARCH_PATHS_HINT}"
    PATHS "${_Emacs_SEARCH_PATHS}"
    VALIDATOR _Emacs_version_validator
  )
  mark_as_advanced(Emacs_EXECUTABLE)

  if(Emacs_EXECUTABLE)
    _Emacs_get_version(Emacs_VERSION _Emacs_version_result "${Emacs_EXECUTABLE}")

    if(NOT (${_Emacs_version_result} EQUAL 0))
      if(NOT Emacs_FIND_QUIETLY)
        message(WARNING "Emacs executable failed unexpectedly while determining version (exit status: ${_Emacs_version_result}). Disabling Emacs.")
      endif()
      set(Emacs_EXECUTABLE "${Emacs_EXECUTABLE}-FAILED_EXECUTION-NOTFOUND")
    else()
      # Create an imported target for Emacs
      if(NOT TARGET Emacs::emacs)
        add_executable(Emacs::emacs IMPORTED GLOBAL)
        set_target_properties(Emacs::emacs PROPERTIES
          IMPORTED_LOCATION "${Emacs_EXECUTABLE}"
        )
      endif()

      set(_Emacs_HINT_PATH_FROM_EXE "${Emacs_EXECUTABLE}/../..")
      get_filename_component(_Emacs_HINT_PATH_FROM_EXE "${_Emacs_HINT_PATH_FROM_EXE}" ABSOLUTE)
      list(APPEND _Emacs_SEARCH_PATHS_HINT "${_Emacs_HINT_PATH_FROM_EXE}")
    endif()
  endif()
endmacro()

macro(_Emacs_find_emacs_headers)
  find_path(Emacs_INCLUDE_DIR emacs-module.h
    DOC "Path to Emacs module header"
    PATH_SUFFIXES include emacs/include ../include
    HINTS ${_Emacs_SEARCH_PATHS_HINT}
    PATHS ${_Emacs_SEARCH_PATHS}
  )

  if(Emacs_INCLUDE_DIR)
    if (NOT TARGET Emacs::emacs_module)
      add_library(Emacs::emacs_module INTERFACE IMPORTED GLOBAL)
      set_target_properties(Emacs::emacs_module PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Emacs_INCLUDE_DIR}"
      )
    endif()
  endif()
endmacro()

set(_Emacs_SEARCH_PATHS
  /opt /usr /usr/local # FHS
  /sw /opt/sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  ~/.guix-profile # Guix
  ~/.nix-profile # Nix
  ${Emacs_PATH})
set(_Emacs_SEARCH_PATHS_HINT)

_Emacs_find_emacs()
_Emacs_find_emacs_headers()

find_package_handle_standard_args(
  Emacs
  REQUIRED_VARS Emacs_EXECUTABLE Emacs_INCLUDE_DIR
  VERSION_VAR Emacs_VERSION
  ${_Emacs_FIND_PACKAGE_EXTRA_ARGS}
)

cmake_policy(POP)
