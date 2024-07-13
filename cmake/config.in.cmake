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

include(FindPackageHandleStandardArgs)
set(${CMAKE_FIND_PACKAGE_NAME}_CONFIG ${CMAKE_CURRENT_LIST_FILE})
find_package_handle_standard_args(@PROJECT_NAME@ CONFIG_MODE)

if (NOT TARGET @PROJECT_NAME@::@CPPEMACS_TARGET_NAME@)
  include("${CMAKE_CURRENT_LIST_DIR}/@CPPEMACS_TARGETS_EXPORT_NAME@.cmake")

  if (NOT TARGET Emacs::emacs_module)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/modules")
    find_package(Emacs)
  endif()
  if (Emacs_FOUND)
    set_target_properties(@PROJECT_NAME@::@CPPEMACS_TARGET_NAME@ PROPERTIES
      INTERFACE_LINK_LIBRARIES Emacs::emacs_module
    )
  elseif(NOT @PROJECT_NAME@_FIND_QUIETLY)
    message(WARN "Emacs not found on this machine")
  endif()

  if (NOT TARGET @CPPEMACS_TARGET_NAME@)
    add_library(@CPPEMACS_TARGET_NAME@ INTERFACE IMPORTED)
    set_target_properties(@CPPEMACS_TARGET_NAME@ PROPERTIES
      INTERFACE_LINK_LIBRARIES @PROJECT_NAME@::@CPPEMACS_TARGET_NAME@
    )
  endif()
endif()
