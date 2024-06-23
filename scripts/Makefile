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

.PHONY:
default: all

# using https://github.com/lluissm/license-header-checker
ALL_LICENSE_HEADERS=license-header-c.txt license-header-el.txt license-header-py.txt
LICENSE_CHECK_IGNORES?=build,.cache

clean:
	rm -rf license-header-*.txt *.tmp.*

CHECK_HEADER_FLAGS?=
.PHONY:
check-headers: $(ALL_LICENSE_HEADERS)
	license-header-checker $(CHECK_HEADER_FLAGS) -i $(LICENSE_CHECK_IGNORES) license-header-c.txt .. hpp cpp
	license-header-checker $(CHECK_HEADER_FLAGS) -i $(LICENSE_CHECK_IGNORES) -e '(;[^\n]*\n)*' license-header-el.txt .. el
	@cp ../CMakeLists.txt CMakeLists.tmp.cmake
	@cp Makefile Makefile.tmp.make
	license-header-checker $(CHECK_HEADER_FLAGS) -i $(LICENSE_CHECK_IGNORES),cmake/vendored -e '(#[^\n]*\n)*' license-header-py.txt .. make cmake gitignore
	@mv CMakeLists.tmp.cmake ../CMakeLists.txt
	@mv Makefile.tmp.make Makefile

license-header-c.txt: license-header.txt
	@(echo '/*' && \
	 sed 's/.*/ *\0/' $< && \
	 echo ' */') > $@
license-header-el.txt: license-header.txt
	@(echo ';; -*- lexical-binding: t -*-' && \
	 echo ';' && \
	 sed 's/.*/;;\0/' $<) > $@
license-header-py.txt: license-header.txt
	@(sed 's/.*/#\0/' $<) > $@

.PHONY:
all:
	$(MAKE) check-headers
	$(MAKE) clean
