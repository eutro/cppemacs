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

#include <vector>
SCENARIO("using vec_cell") {
  GIVEN("a heterogenous vector") {
    vec_cell vec = (envp->*"vector")(1, 1.5, "hello"_Estr, "some-symbol");

    THEN("all the elements are correct") {
      cell format = envp->*"format";
      std::vector<std::string> elts;
      for (cell elt : vec) {
        elts.push_back(format("%S"_Estr, elt).unwrap<std::string>());
      }

      std::vector<std::string> expected{"1", "1.5", "\"hello\"", "some-symbol"};
      REQUIRE(elts == expected);
    }
  }
}
