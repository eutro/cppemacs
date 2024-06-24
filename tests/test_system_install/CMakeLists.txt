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

cmake_minimum_required(VERSION 3.13)

project(cppemacs_test_subproject_include)

include(FetchContent)

find_package(cppemacs REQUIRED)

if (NOT TARGET cppemacs::cppemacs)
  message(FATAL_ERROR "Test failed: cppemacs not available")
endif()

if (NOT TARGET Emacs::emacs_module)
  message(FATAL_ERROR "Test failed: Emacs::emacs_module not available")
endif()
