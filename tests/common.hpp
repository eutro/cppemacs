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

#ifndef CPPEMACS_TEST_COMMON_
#define CPPEMACS_TEST_COMMON_

#include <cppemacs/all.hpp>
#include <catch2/catch_all.hpp>
#include <string>

namespace cppemacs {
extern envw envp;

inline void test_run_scoped(void (*f)()) {
  envw env_saved = envp;
  env_saved.run_scoped([&](envw env) {
    envp = env;
    try {
      f();
      envp = env_saved;
    } catch(...) {
      envp = env_saved;
      throw;
    }
  });
}

}

#define CPPEMACS_TEST_UNIQUE_NAME2(NAME, COUNT) cppemacs_##NAME##_##COUNT
#define CPPEMACS_TEST_UNIQUE_NAME1(NAME, COUNT) CPPEMACS_TEST_UNIQUE_NAME2(NAME, COUNT)
#define CPPEMACS_TEST_UNIQUE_NAME(NAME) CPPEMACS_TEST_UNIQUE_NAME1(NAME, __LINE__)
#define TEST_SCOPED(TEST_CASE_EXPR)                             \
  static void CPPEMACS_TEST_UNIQUE_NAME(scoped_test)();         \
  TEST_CASE_EXPR { cppemacs::test_run_scoped(CPPEMACS_TEST_UNIQUE_NAME(scoped_test)); } \
  static void CPPEMACS_TEST_UNIQUE_NAME(scoped_test)()

using namespace cppemacs;
using namespace cppemacs::literals;
using namespace cppemacs::detail;

template <typename T, typename = void> struct type_name;
#define REGISTER_NAME(T) \
  template<> struct type_name<T> { static constexpr const char *value = #T; }
#define REGISTER_NAME2(A, B) REGISTER_NAME(A); REGISTER_NAME(B)
#define REGISTER_NAME4(A, B, C, D) REGISTER_NAME2(A, B); REGISTER_NAME2(C, D)

template <typename T> struct type_name<
  T, detail::enable_if_t<std::is_integral<T>::value>> {
  static constexpr const char *prefix =
    std::numeric_limits<T>::is_signed
    ? "int" : "uint";
  static constexpr const int bits =
    std::numeric_limits<T>::digits
    + std::numeric_limits<T>::is_signed;

  friend std::ostream &operator<<(std::ostream &os, const type_name &tn) {
    return os << tn.prefix << tn.bits << "_t";
  }
};

REGISTER_NAME2(bool, std::string);
REGISTER_NAME(estring_literal);
REGISTER_NAME(eread_literal);
#ifdef CPPEMACS_HAVE_STRING_VIEW
REGISTER_NAME(std::string_view);
#endif
#if CPPEMACS_ENABLE_GMPXX
REGISTER_NAME(mpz_class);
#endif

template <typename T>
std::ostream &operator<<(std::ostream &os, const type_name<T> &tn) { return os << tn.value; }

struct ReturnsNonNilOn : Catch::Matchers::MatcherGenericBase {
  cell function;
  std::vector<value> args;
  template <typename Func, typename...Args>
  ReturnsNonNilOn(Func func, Args &&...arg)
    : function(envp->*func),
      args({value(function->*std::forward<Args>(arg))...}) {}

  template <typename Arg>
  bool match(Arg &&arg) const {
    std::vector<value> argscpy = args;
    argscpy.push_back(function->*arg);
    return function.call(argscpy.size(), argscpy.data());
  }

  std::string describe() const override {
    envw env = function.env();
    return (env->*"format")(
      "returns non-nil on: %S%S"_Estr,
      function,
      (env->*"list").call(
        args.size(),
        const_cast<value*>(args.data())
      )
    ).extract<std::string>();
  }
};

struct LispEquals : ReturnsNonNilOn {
  template <typename Arg>
  LispEquals(Arg &&arg): ReturnsNonNilOn("equal", std::forward<Arg>(arg)) {}

  std::string describe() const override {
    return (function->*"format")("lisp-equals %S"_Estr, args[0]).extract<std::string>();
  }
};

#endif /* CPPEMACS_TEST_COMMON_ */
