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

#ifndef CPPEMACS_LITERALS_HPP_
#define CPPEMACS_LITERALS_HPP_

#include "core.hpp"
#include <cstring>
#include <ostream>

namespace cppemacs::literals {

/** ""_Estr Emacs string literal representation. */
struct estring_literal {
  const char *data; size_t len;
  estring_literal(const char *data, size_t len): data(data), len(len) {}
  estring_literal(const char *str): data(str), len(std::strlen(str)) {}

  estring_literal(const std::string &str): data(str.data()), len(str.length()) {}
  operator std::string() const { return std::string(data, len); }
#ifdef __cpp_lib_string_view
  estring_literal(const std::string_view &str): data(str.data()), len(str.length()) {}
  operator std::string_view() const { return std::string_view(data, len); }
#endif

  /** Convert a C++ ""_Estr string to an Emacs string. */
  friend value to_emacs(expected_type_t<estring_literal>, env nv, const estring_literal &str)
  { return nv.make_string(str.data, str.len); }

  friend std::ostream &operator<<(std::ostream &os, const estring_literal &str)
  { os.write(str.data, str.len); return os; }
};
/** ""_Estr Emacs string literal. */
inline estring_literal operator "" _Estr(const char *data, size_t len)
{ return estring_literal(data, len); }

/** A readable literal, with ""_Eread. */
struct eread_literal : estring_literal {
  template <typename...Args>
  eread_literal(Args &&...args): estring_literal(std::forward<Args>(args)...) {}

  /** Convert a C++ ""_Estr string to an Emacs string. */
  template <typename Env>
  friend value to_emacs(expected_type_t<eread_literal>, const Env &nv, const eread_literal &str)
  { return (nv->*"read")(nv.make_string(str.data, str.len)); }
};
/** ""_Eread literal, uses `read` for conversion to Emacs. */
inline eread_literal operator "" _Eread(const char *data) { return eread_literal(data); }
/** ""_Eread literal, uses `read` for conversion to Emacs. */
inline eread_literal operator "" _Eread(const char *data, size_t len) { return eread_literal(data, len); }

}

#endif /* CPPEMACS_LITERALS_HPP_ */
