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

#include "common.hpp"

SCENARIO("throwing exceptions") {
  GIVEN("definitions of `cppemacs-funN'") {
    cell defalias = envp->*"defalias";

    defalias("cppemacs-fun1", envp->*make_spreader_function<2>(
               "Equivalent to `signal'.",
               [](envw nv, value sym, value data) -> value
               { throw signal(sym, data); }));

    defalias("cppemacs-fun2", envp->*make_spreader_function<2>(
               "Equivalent to `throw'.",
               [](envw nv, value sym, value data) -> value
               { throw thrown(sym, data); }));

    defalias("cppemacs-fun3", envp->*make_spreader_function<1>(
               "Throw a C++ `runtime_error' with the argument.",
               [](envw nv, cell msg) -> value
               { throw std::runtime_error(msg.unwrap<std::string>()); }));

    defalias("cppemacs-fun4", envp->*make_spreader_function<0, true>(
               "Call `error'.",
               [](envw nv, std::pair<value *, size_t> rest) {
                 nv.funcall(nv->*"error", rest.second, rest.first);
                 nv.maybe_non_local_exit();
                 return false;
               }));

    auto [expr, sig_type] = GENERATE(
      std::make_pair(
        R"((signal 'error '("normal error")))"_Eread,
        funcall_exit::signal_
      ),
      std::make_pair(
        R"((throw 'hello 'world))"_Eread,
        funcall_exit::throw_
      ),
      std::make_pair(
        R"((+ 1 2))"_Eread,
        funcall_exit::return_
      ),

      std::make_pair(
        R"((cppemacs-fun1 'error '("C++ error")))"_Eread,
        funcall_exit::signal_
      ),
      std::make_pair(
        R"((cppemacs-fun2 'hello 'thrown-value))"_Eread,
        funcall_exit::throw_
      ),
      std::make_pair(
        R"((cppemacs-fun3 "C++ runtime error"))"_Eread,
        funcall_exit::signal_
      ),

      std::make_pair(
        R"((cppemacs-fun4 "%s: %s" "silly" "error"))"_Eread,
        funcall_exit::signal_
      ),

      std::make_pair(
        R"((cppemacs-fun4))"_Eread,
        funcall_exit::signal_
      )
    );
    WHEN("evaluating " << expr) {
      auto eval_expr = [e = expr]() { ((envp->*"eval")(e), envp.rethrow_non_local_exit()); };
      switch (sig_type) {
      case funcall_exit::signal_:
        THEN("a signal is raised") {
          REQUIRE_THROWS_AS(eval_expr(), signal);
        }
        break;
      case funcall_exit::throw_:
        THEN("a value is thrown") {
          REQUIRE_THROWS_AS(eval_expr(), thrown);
        }
        break;

      default:
        THEN("it returns normally") {
          REQUIRE_NOTHROW(eval_expr());
        }
        break;
      }
    }
  }
}
