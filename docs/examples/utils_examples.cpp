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

#if (EMACS_MAJOR_VERSION < 27)
extern "C" {
int plugin_is_GPL_compatible;

int emacs_module_init(emacs_runtime *rt) noexcept {
  return 0;
}
}
#else

#include <cppemacs/all.hpp>
#include <vector>

#if defined(__cpp_lib_optional) || (__cplusplus > 201606L)
#  include <optional>
#endif
#if defined(__cpp_lib_span) || (__cplusplus > 202002L)
#  include <span>
#endif

using namespace cppemacs;
using namespace cppemacs::literals;

static void module_init(envw env);

extern "C" {

int plugin_is_GPL_compatible;

int emacs_module_init(emacs_runtime *rt) noexcept {
  if (rt->size < sizeof(*rt)) return 1;
  envw env = rt->get_environment(rt);
  if (!env.is_compatible<27>()) return 2;

  env.run_catching([&]() { module_init(env); });

  return 0;
}

}

//! [Spreader Functions]
static void spreader_function_examples(envw env) {
  env->*make_spreader_function(
    spreader_thunk(),
    "Do something with zero arguments.",
    [](envw env) {
      // ...
      return nullptr;
    });

  env->*make_spreader_function(
    spreader_arity<1, 2>(),
    "Do something with X and maybe Y.\n\n(fn X &optional Y)",
    [](envw env, value x, value y) {
      if (x) {
        // invoked with two arguments
      } else {
        // invoked with one argument
      }
      return nullptr;
    });

  // variadic functions
  //! [VA Spreader Functions]
  env->*make_spreader_function(
    spreader_variadic<1>(),
    "Do something with X and REST\n\n(fn X &rest REST).",
    [](envw env, value x, spreader_restargs rest) {
      for (value val : rest) {
        // ...
      }
      return nullptr;
    });

  env->*make_spreader_function(
    spreader_variadic<1>(),
    "Do something with X and REST\n\n(fn X &rest REST).",
    [](envw env, value x, std::vector<value> rest) {
      // ...
      return nullptr;
    });

#if defined(__cpp_lib_span)
  env->*make_spreader_function(
    spreader_variadic<1>(),
    "Do something with X and REST\n\n(fn X &rest REST).",
    [](envw env, value x, std::span<value> rest) {
      // ...
      return nullptr;
    });
#endif
  //! [VA Spreader Functions]
}
//! [Spreader Functions]

//! [Cell Extracted]
using intcell = cell_extracted<int>;
static void cell_extracted_examples(envw env) {
  env->*make_spreader_function(
    spreader_arity<2>(), "Add X and Y.\n\n(fn X Y)",
    [](envw env, intcell x, intcell y) -> int {
      return x.get() + y.get();
    });
  // equivalent to:
  env->*make_spreader_function(
    spreader_arity<2>(), "Add X and Y.\n\n(fn X Y)",
    [](envw env, cell x, cell y) -> int {
      return x.extract<int>() + y.extract<int>();
    });
}

#if defined(__cpp_lib_optional) || (__cplusplus > 201606L)
// optional arguments via C++17 optional
template <typename T> using optcell = cell_extracted<T, std::optional<T>>;
static void cell_extracted_optcell(envw env) {
  env->*make_spreader_function(
    spreader_arity<1, 2>(),
    "Add X to Y, or 10 if not provided.\n\n(fn X &optional Y)",
    [](envw env, intcell x, optcell<int> y) -> int {
      return x.get() + y->value_or(10);
    }
  );
}
#endif
//! [Cell Extracted]

static void module_init(envw env) {
  cell_extracted_examples(env);
#if defined(__cpp_lib_optional) || (__cplusplus > 201606L)
  cell_extracted_optcell(env);
#endif
}

#endif
