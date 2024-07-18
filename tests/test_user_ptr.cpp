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

SCOPED_SCENARIO("unwrapping a user_ptr") {
  GIVEN("a single user_ptr") {
    std::shared_ptr<int> sptr(new int{0});
    auto uptr = make_user_ptr<decltype(sptr)>(sptr);

    WHEN("unwrapping it") {
      cell ptrv = envp->*uptr;
      auto unwrapped = ptrv.extract<user_ptr<decltype(sptr)>>();

      THEN("the result is the same pointer") {
        REQUIRE(unwrapped.get() == uptr.get());
      }
    }

    WHEN("garbage collecting it") {
      envp.run_scoped([&](envw env) {
        env->*make_user_ptr<decltype(sptr)>(sptr);
      });
      long old_count = sptr.use_count();

      if ((envp->*"garbage-collect")()) {
        THEN("it gets garbage collected") {
          REQUIRE(sptr.use_count() < old_count);
        }
      }
    }
  }
}

struct CommonType {
  int x;
  CommonType(int x): x(x) {}
  bool operator==(const CommonType &o) const { return x == o.x; }
};
struct Type1 : CommonType { Type1(int x) : CommonType(x) {} };
struct Type2 : CommonType { Type2(int x) : CommonType(x) {} };

SCOPED_SCENARIO("type-checking user pointers") {
  GIVEN("two user_ptr-s of different types") {
    cell ptr1 = envp->*make_user_ptr<Type1>(1);
    cell ptr2 = envp->*make_user_ptr<Type2>(2);
    envp.maybe_non_local_exit();

    WHEN("they are unwrapped") {
      THEN("the results are the same") {
        REQUIRE_NOTHROW(*ptr1.extract<user_ptr<Type1>>() == Type1(1));
        REQUIRE_NOTHROW(*ptr2.extract<user_ptr<Type2>>() == Type2(2));
      }
    }

    WHEN("they are extracted as the wrong type") {
      THEN("an exception is thrown") {
        REQUIRE_THROWS(ptr1.extract<user_ptr<Type2>>());
        REQUIRE_THROWS(ptr2.extract<user_ptr<Type1>>());
        REQUIRE_THROWS(ptr1.extract<user_ptr<CommonType>>());
        REQUIRE_THROWS(ptr2.extract<user_ptr<CommonType>>());
      }
    }
  }
}
