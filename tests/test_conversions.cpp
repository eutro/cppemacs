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

#include <cstdint>

struct is_the_same {
  template <typename Lhs, typename Res>
  void operator()(expected_type_t<Res>, Lhs &&lhs, const cell &val) const {
    THEN("is the same") {
      REQUIRE(lhs == val.unwrap<Res>());
    }
  }
};

struct throws_an_exception {
  template <typename Lhs, typename Res>
  void operator()(expected_type_t<Res>, Lhs &&, const cell &val) const {
    THEN("throws an exception") {
      REQUIRE_THROWS(val.unwrap<Res>());
    }
  }
};

template <typename Arg, typename Res = Arg, typename F = is_the_same>
static void checkRoundTrip(const Arg &value, F f = {}) {
  GIVEN("a value " << value << " of type " << type_name<Arg>()) {
    cell val = envp->*value;
    WHEN("round-trip converting to " << type_name<Res>()) {
      f(expected_type_t<Res>{}, value, val);
    }
  }
}

SCENARIO("round trip converting values") {
  WHEN("reading malformed input") {
    THEN("an exception is thrown") {
      REQUIRE_THROWS((envp->*")"_Eread, envp.maybe_non_local_exit())); // mismatched delimiter
    }
  }

  checkRoundTrip<std::string>("abcd");
#ifdef __cpp_lib_string_view
  checkRoundTrip<std::string_view, std::string>("abcdefgh");
#endif

  checkRoundTrip<bool>(true);
  checkRoundTrip<bool>(false);

  checkRoundTrip<int>(123);
  checkRoundTrip<int>(INT_MIN);
  checkRoundTrip<intmax_t, short int, throws_an_exception>(static_cast<intmax_t>(INT_MIN) - 1);
  checkRoundTrip<intmax_t, short int, throws_an_exception>(static_cast<intmax_t>(INT_MAX) + 1);

  checkRoundTrip<intmax_t>(INTMAX_MAX);
  checkRoundTrip<intmax_t>(INTMAX_MIN);

#ifdef CPPEMACS_HAS_UINTMAX_CONVERSION
  if (envp->size >= sizeof(emacs_env_27)) {
    checkRoundTrip<uintmax_t>(UINTMAX_MAX);
    checkRoundTrip<uintmax_t>(static_cast<uintmax_t>(INTMAX_MAX) + 15);
    checkRoundTrip<uintmax_t>(INTMAX_MAX);
    checkRoundTrip<std::string, uintmax_t, throws_an_exception>("ab");

    checkRoundTrip<intmax_t, uintmax_t, throws_an_exception>(INTMAX_MIN);
    checkRoundTrip<intmax_t, uintmax_t, throws_an_exception>(-10);
    checkRoundTrip<eread_literal, uintmax_t, throws_an_exception>(
      eread_literal(std::to_string(UINTMAX_MAX) += "0"));
  }
#endif

  checkRoundTrip<int, std::string, throws_an_exception>(1);
  checkRoundTrip<std::string, int, throws_an_exception>("abcd");
}
