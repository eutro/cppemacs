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
#include <vector>

SCOPED_CASE("cell.vec_*") {
  GIVEN("a heterogenous vector") {
    cell vec = (envp->*"vector")(1, 1.5, "hello"_Estr, "some-symbol");

    WHEN("iterating over the vector") {
      THEN("all the elements are correct") {
        cell format = envp->*"format";
        std::vector<std::string> elts;
        for (ptrdiff_t ii = 0, end = vec.vec_size(); ii < end; ++ii) {
          elts.push_back(format("%S"_Estr, vec.vec_get(ii)).extract<std::string>());
        }

        std::vector<std::string> expected{"1", "1.5", "\"hello\"", "some-symbol"};
        REQUIRE(elts == expected);
      }
    }
  }
}
