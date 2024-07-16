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

extern "C" {
CPPEMACS_EXPORT int plugin_is_GPL_compatible;
}

namespace itf_with_emacs {
//! [Interfacing with Emacs]
using namespace cppemacs;
int emacs_module_init(emacs_runtime *rt) noexcept {
  envw env = rt->get_environment(rt);
  env.run_catching([&]() {
    (env->*"set-default-toplevel-value")("my-cool-constant", 12345);

    cell defalias = env->*"defalias";
    defalias(
      "my-cool-function",
      env->*make_spreader_function(
        spreader_arity<1>(),
        "Do something cool.",
        [](envw env, cell arg) {
          if (arg.extract<intmax_t>() < 0) {
            throw std::runtime_error("Argument must be non-negative");
          }

          return arg;
        }
      )
    );
  });
  return 0;
}
//! [Interfacing with Emacs]
}

#if (EMACS_MAJOR_VERSION >= 27) && defined(CPPEMACS_HAVE_STRING_VIEW)
namespace conversions {
using namespace cppemacs;
//! [Conversions]
void cpp_to_emacs_conversions(envw env) {
  // `defalias' symbol
  value defalias_sym = env->*"defalias";

  // create numbers
  env->*10; env->*12345L; env->*5.5;

  // create a string
  using namespace cppemacs::literals;
  env->*"This is a string."_Estr;

  // or with C++17 string_view
  using namespace std::string_view_literals;
  env->*"This is also a string."sv;

  // `t' and `nil'
  env->*true; env->*false;

  // inject()/->* return a cell directly
  cell defalias = env->*"defalias";
  // ->* with an existing value just wraps it in a cell
  defalias = env->*defalias_sym;

  // cell::operator()() automatically converts
  defalias("identity-alias", "identity"); // = (defalias 'identity-alias 'identity)
}

void cpp_from_emacs_conversions(envw env, value val) {
  // convert to an integer
  int x = env.extract<int>(val);

  // cell provides an even more convenient interface
  cell cel(env, val);
  int y = cel.extract<int>();
  double d = cel.extract<double>();
  std::string s = cel.extract<std::string>();
}
//! [Conversions]
}
#endif

extern "C" CPPEMACS_EXPORT int
emacs_module_init(emacs_runtime *rt) noexcept {
  itf_with_emacs::emacs_module_init(rt);
  return 0;
}
