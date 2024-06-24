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

namespace cppemacs {
extern envw envp;
}

using namespace cppemacs;
using namespace cppemacs::literals;

#ifdef __GNUC__
#include <cxxabi.h>
#endif

#ifdef __GNUC__
static inline std::string demangle(const char *name) {
  int status;
  std::unique_ptr<char[], decltype(&std::free)> s(
    abi::__cxa_demangle(name, 0, 0, &status),
    &std::free);
  return std::string(s ? s.get() : name);
}
#else
static inline std::string demangle(std::string &&s) { return s; }
#endif

template <typename T>
static inline std::string type_name() { return demangle(typeid(T).name()); }

namespace Catch {
template<>
struct StringMaker<std::type_info>
{ static std::string convert(std::type_info const &value) { return demangle(value.name()); } };
}

#endif /* CPPEMACS_TEST_COMMON_ */
