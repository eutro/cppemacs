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
#include <ostream>

namespace cppemacs {

/**
 * @defgroup cppemacs_literals Literals
 * @brief Custom string literals for Emacs values.
 *
 * @addtogroup cppemacs_literals
 * @{
 */

namespace literals {

/** @brief A reference to a string, with `""_Estr`. */
struct estring_literal {
  /** @brief The utf8 data of the string. */
  const char *data;
  /** @brief The number of characters in the string. */
  size_t len;

  /** @brief Construct from data and length. */
  constexpr estring_literal(const char *data, size_t len): data(data), len(len) {}
  /** @brief Construct from a null-terminated string. */
  estring_literal(const char *str): data(str), len(std::char_traits<char>::length(str)) {}

  /** @brief Construct from a std::string. */
  estring_literal(const std::string &str): data(str.data()), len(str.length()) {}
  /** @brief Convert to a std::string. */
  operator std::string() const { return std::string(data, len); }
#ifdef CPPEMACS_HAVE_STRING_VIEW
  /** @brief Construct from a std::string_view. */
  constexpr estring_literal(const std::string_view &str): data(str.data()), len(str.length()) {}
  /** @brief Convert to a std::string_view. */
  constexpr operator std::string_view() const { return std::string_view(data, len); }
#endif

  /** @brief Convert a C++ ""_Estr string to an Emacs string. */
  friend value to_emacs(expected_type_t<estring_literal>, envw nv, const estring_literal &str)
  { return nv.make_string(str.data, str.len); }

  /** @brief Write this value to an output stream. */
  friend std::ostream &operator<<(std::ostream &os, const estring_literal &str)
  { os.write(str.data, str.len); return os; }
};

/** @brief `""_Estr` string literal. */
inline constexpr estring_literal operator "" _Estr(const char *data, size_t len)
{ return estring_literal(data, len); }

/** @brief A readable literal, with `""_Eread`. */
struct eread_literal : estring_literal {
  /** @brief Constructor which forwards to `estring_literal` */
  template <typename...Args>
  constexpr eread_literal(Args &&...args): estring_literal(std::forward<Args>(args)...) {}

  /** @brief Read the string. */
  friend value to_emacs(expected_type_t<eread_literal>, envw nv, const eread_literal &str)
  { return (nv->*"read")(nv.make_string(str.data, str.len)); }
};
/** @brief `""_Eread` literal, uses `read` for conversion to Emacs. */
inline eread_literal operator "" _Eread(const char *data) { return eread_literal(data); }
/** @brief `""_Eread` literal, uses `read` for conversion to Emacs. */
inline constexpr eread_literal operator "" _Eread(const char *data, size_t len) { return eread_literal(data, len); }

/** @} */

}

}

#endif /* CPPEMACS_LITERALS_HPP_ */
