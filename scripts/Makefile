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

.PHONY:
clean:
	rm -rf *.tmp.*

.PHONY:
add-tmp-extension:
	@find .. -name "$(NAME)" -execdir cp "$(NAME)" "$(NAME).tmp.$(EXT)" \;

.PHONY:
remove-tmp-extension:
	@find .. -name "$(NAME).tmp.$(EXT)" -execdir mv "$(NAME).tmp.$(EXT)" "$(NAME)" \;

CHECK_HEADER_FLAGS?=
.PHONY:
check-headers:
	emacs --batch --quick --script check-license.el --eval '(check-license-main)' -- \
	  --color auto -s check-license-spec.el $(CHECK_HEADER_FLAGS) -C ..

.PHONY:
all:
	$(MAKE) check-headers
	$(MAKE) clean
