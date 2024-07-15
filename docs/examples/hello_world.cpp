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

#include <cppemacs/all.hpp>

#if (EMACS_MAJOR_VERSION < 28)
extern "C" {
int plugin_is_GPL_compatible;

int emacs_module_init(emacs_runtime *rt) noexcept {
  return 0;
}
}
#else

//! [Full example]
#include <cppemacs/all.hpp>
using namespace cppemacs;
using namespace cppemacs::literals;

extern "C" {

int plugin_is_GPL_compatible;

int emacs_module_init(emacs_runtime *rt) noexcept {
  // make sure rt has all the members we know of
  if (rt->size < sizeof(*rt)) return 1;

  // wrap the environment
  envw env = rt->get_environment(rt);

  // check that certain features are available
  if (!env.is_compatible<28>()) return 2;

  // propagate C++ exceptions and Emacs non-local exits
  env.run_catching([&]() {
    // call a function
    (env->*"message")("Hello, world!"_Estr);

    // create functions with any arity
    cell cppemacs_hello_world = env->*make_spreader_function(
      spreader_thunk(),
      "Run Hello, world!",
      [](envw env) {
        // (message "Hello, %s!" (read-string "What is your name?"))
        (env->*"message")(
          "Hello, %s!"_Estr,
          (env->*"read-string")("What is your name? "_Estr)
        );

        return false; // -> nil
      });
    // make the function interactive (Emacs 28+)
    env.make_interactive(cppemacs_hello_world, env->*""_Estr);

    // expose them to Emacs
    (env->*"defalias")("cppemacs-hello-world", cppemacs_hello_world);
  });

  return 0;
}

}
//! [Full example]

#endif
