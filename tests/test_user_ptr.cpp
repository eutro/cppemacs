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

class Type1 {};
class Type2 {};

extern "C" void cppemacs_this_should_show_up_in_valgrind() {
  make_user_ptr<int>(10);
}

TEST_SCOPED(SCENARIO("using user_ptr")) {
  cppemacs_this_should_show_up_in_valgrind();

  GIVEN("a single user_ptr") {
    auto uptr = make_user_ptr<int>(10);
    cell ptrv = envp->*uptr;

    THEN("unwrapping it yields the same pointer") {
      auto unwrapped = ptrv.extract<user_ptr<int>>();
      REQUIRE(unwrapped.get() == uptr.get()); envp.maybe_non_local_exit();
    }
  }

  GIVEN("a user_ptr<shared_ptr>") {
    std::shared_ptr<int> sptr(new int{0});
    envp.run_scoped([&](envw env) {
      env->*make_user_ptr<decltype(sptr)>(sptr);
    });
    CHECK(sptr.use_count() == 2);

    THEN("garbage collection releases it") {
      if ((envp->*"garbage-collect")()) {
        REQUIRE(sptr.use_count() == 1);
      }
    }
  }

  GIVEN("two user_ptr-s of different types") {
    cell ptr1 = envp->*make_user_ptr<Type1>(); envp.maybe_non_local_exit();
    cell ptr2 = envp->*make_user_ptr<Type2>(); envp.maybe_non_local_exit();

    THEN("they can be unwrapped") {
      REQUIRE_NOTHROW(ptr1.extract<user_ptr<Type1>>());
      REQUIRE_NOTHROW(ptr2.extract<user_ptr<Type2>>());
    }

    THEN("they cannot be converted to each other") {
      REQUIRE_THROWS(ptr1.extract<user_ptr<Type2>>());
      REQUIRE_THROWS(ptr2.extract<user_ptr<Type1>>());
    }
  }
}
