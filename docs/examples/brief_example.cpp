/*
 * Copyright (C) 2024 Eutro <https://eutro.dev>
 *
 * This file is part of cppemacs.
 *
 * cppemacs is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cppemacs is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cppemacs. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-FileCopyrightText: 2024 Eutro <https://eutro.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

//! [Brief Example]
#include <cppemacs/all.hpp>
using namespace cppemacs;

extern "C" {

// must be compatible with Emacs' license
CPPEMACS_EXPORT int plugin_is_GPL_compatible;

// the entrypoint of the module
CPPEMACS_EXPORT int emacs_module_init(emacs_runtime *rt) noexcept {
  envw env = rt->get_environment(rt);

  // ... define module functions
  cell defalias = env->*"defalias";
  defalias("example-function", env->*make_spreader_function(
             spreader_thunk(), "Do something useful.",
             [](envw env) {
               // ... do something useful
               return nullptr; // return nil
             }));

  return 0;
}

}
//! [Brief Example]
