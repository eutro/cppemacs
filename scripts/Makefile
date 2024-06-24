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
ALL_LICENSE_TYPES=c el py html
ALL_LICENSE_HEADERS=$(addprefix license-header-,$(addsuffix .txt,$(ALL_LICENSE_TYPES)))
LICENSE_CHECK_IGNORES?=build,.cache

.PHONY:
clean:
	rm -rf license-header-*.txt *.tmp.*

.PHONY:
add-tmp-extension:
	@find .. -name "$(NAME)" -execdir cp "$(NAME)" "$(NAME).tmp.$(EXT)" \;

.PHONY:
remove-tmp-extension:
	@find .. -name "$(NAME).tmp.$(EXT)" -execdir mv "$(NAME).tmp.$(EXT)" "$(NAME)" \;

CHECK_HEADER_FLAGS?=
.PHONY:
check-headers: $(ALL_LICENSE_HEADERS)
	license-header-checker $(CHECK_HEADER_FLAGS) -i $(LICENSE_CHECK_IGNORES) license-header-c.txt .. hpp cpp
	license-header-checker $(CHECK_HEADER_FLAGS) -i $(LICENSE_CHECK_IGNORES) -e '(;[^\n]*\n)*' license-header-el.txt .. el
	@$(MAKE) --no-print-directory add-tmp-extension NAME=CMakeLists.txt EXT=cmake
	@$(MAKE) --no-print-directory add-tmp-extension NAME=Makefile EXT=make
	license-header-checker $(CHECK_HEADER_FLAGS) -i $(LICENSE_CHECK_IGNORES),cmake/vendored -e '(#[^\n]*\n)*' license-header-py.txt .. make cmake gitignore
	@$(MAKE) --no-print-directory remove-tmp-extension NAME=CMakeLists.txt EXT=cmake
	@$(MAKE) --no-print-directory remove-tmp-extension NAME=Makefile EXT=make
	license-header-checker $(CHECK_HEADER_FLAGS) -i $(LICENSE_CHECK_IGNORES) license-header-html.txt .. html md

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
license-header-html.txt: license-header.txt
	@(echo '<!--' && \
	 sed 's/.*/-\0/' $< && \
	 echo '-->') > $@

.PHONY:
all:
	$(MAKE) check-headers
	$(MAKE) clean
