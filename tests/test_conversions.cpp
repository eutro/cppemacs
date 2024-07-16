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
  void operator()(expected_type_t<Res>, const Lhs &lhs, const cell &val) const {
    THEN("is the same") {
      REQUIRE(lhs == val.extract<Res>());
    }
  }
};

struct throws_an_exception {
  template <typename Lhs, typename Res>
  void operator()(expected_type_t<Res>, Lhs &&, const cell &val) const {
    THEN("throws an exception") {
      REQUIRE_THROWS(val.extract<Res>());
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

TEST_SCOPED(SCENARIO("round trip converting values")) {
  WHEN("reading malformed input") {
    THEN("an exception is thrown") {
      REQUIRE_THROWS((envp->*")"_Eread, envp.maybe_non_local_exit())); // mismatched delimiter
    }
  }

  checkRoundTrip<std::string>("abcd");

  // check Catch config as well otherwise it might not link
#if defined(CPPEMACS_HAVE_STRING_VIEW)
  checkRoundTrip<std::string_view, std::string>("abcdefgh");
#endif

  checkRoundTrip<bool>(true);
  checkRoundTrip<bool>(false);

  checkRoundTrip<int>(123);
  checkRoundTrip<int>(INT_MIN);
  checkRoundTrip<intmax_t, short int, throws_an_exception>(static_cast<intmax_t>(INT_MIN) - 1);
  checkRoundTrip<intmax_t, short int, throws_an_exception>(static_cast<intmax_t>(INT_MAX) + 1);

#if (EMACS_MAJOR_VERSION >= 27)
  if (envp.is_compatible<27>()) {
    // these are too big for fixnums, which are the only integer type before Emacs 27
    checkRoundTrip<intmax_t>(INTMAX_MAX);
    checkRoundTrip<intmax_t>(INTMAX_MIN);

    checkRoundTrip<uintmax_t>(UINTMAX_MAX);
    checkRoundTrip<uintmax_t>(static_cast<uintmax_t>(INTMAX_MAX) + 15);
    checkRoundTrip<uintmax_t>(INTMAX_MAX);
    checkRoundTrip<std::string, uintmax_t, throws_an_exception>("ab");

    checkRoundTrip<intmax_t, uintmax_t, throws_an_exception>(INTMAX_MIN);
    checkRoundTrip<intmax_t, uintmax_t, throws_an_exception>(-10);
    checkRoundTrip<eread_literal, uintmax_t, throws_an_exception>(
      eread_literal(std::to_string(UINTMAX_MAX) += "0"));

#  ifdef CPPEMACS_ENABLE_GMPXX
    checkRoundTrip<mpz_class>(mpz_class(0));
    checkRoundTrip<mpz_class>(mpz_class(-99));
    checkRoundTrip<mpz_class>(mpz_class(99));
    checkRoundTrip<mpz_class>(mpz_class(LONG_MAX) * LONG_MAX * LONG_MAX - 999);
    checkRoundTrip<mpz_class>(mpz_class(LONG_MIN) * LONG_MAX * LONG_MAX + 999);

    checkRoundTrip<estring_literal, mpz_class, throws_an_exception>("not an integer"_Estr);
#  endif
  }
#endif

  checkRoundTrip<int, std::string, throws_an_exception>(1);
  checkRoundTrip<std::string, int, throws_an_exception>("abcd");
}

TEST_SCOPED(SCENARIO("constructing functions")) {
  std::shared_ptr<int> sptr{new int{0}};
  REQUIRE(sptr.use_count() == 1);

  GIVEN("a lambda that captures many things") {
    long long w = 0, x = 0, y = 0, z = 0;
    using llcell = cell_extracted<long long>;
    auto func = make_spreader_function(
      spreader_arity<1, 4>(),
      "Update the internal state.",
      [w, x, y, z, sptr](envw env, llcell wv, llcell xv, llcell yv, llcell zv) mutable {
        CHECK(sptr.use_count() >= 1);
        value ret = (env->*"list")(w, x, y, z);
        std::tie(w, x, y, z) = std::make_tuple(wv, xv, yv, zv);
        return ret;
      });

    WHEN("it is converted") {
      unsigned old_count;
      envp.run_scoped([&](envw env) {
        cell f = env->*std::move(func);
        CHECK(sptr.use_count() == 2);

        THEN("it updates its state correctly") {
          cell equal = env->*"equal";
          cell list = env->*"list";

          REQUIRE_THAT(f(1, 2, 3, 4), LispEquals(list(0, 0, 0, 0)));
          REQUIRE_THAT(f(4, 5, 6),    LispEquals(list(1, 2, 3, 4)));
          REQUIRE_THAT(f(6, 7),       LispEquals(list(4, 5, 6, 0)));
          REQUIRE_THAT(f(7),          LispEquals(list(6, 7, 0, 0)));
        }

        CHECK(sptr.use_count() == 2);
      });

      THEN("it gets destroyed after garbage collection") {
        cell garbage_collect = envp->*"garbage-collect";
        if (garbage_collect() && garbage_collect()) {
          if (envp.is_compatible_relaxed<28>()) {
            REQUIRE(sptr.use_count() == 1);
          } else {
            CHECK_NOFAIL(sptr.use_count() == 1);
          }
        }
      }
    }
  }
}
