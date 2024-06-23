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

#ifndef CPPEMACS_CONVERSIONS_HPP_
#define CPPEMACS_CONVERSIONS_HPP_

#include "core.hpp"

#include <stdexcept>
#include <string>

namespace cppemacs {

/** Convert a string constant to a symbol. */
inline value to_emacs(expected_type_t<const char *>, env nv, const char *name)
{ return nv.intern(name); }

/** Convert a C++ string to an Emacs string. */
inline value to_emacs(expected_type_t<std::string>, env nv, const std::string &str)
{ return nv.make_string(str.data(), str.length()); }

#ifdef __cpp_lib_string_view
/** Convert a C++ string view to an Emacs string. */
inline value to_emacs(expected_type_t<std::string_view>, env nv, const std::string_view &str)
{ return nv.make_string(str.data(), str.length()); }
#endif

/** Convert an Emacs string to a C++ string. */
inline std::string from_emacs(expected_type_t<std::string>, env nv, value val) {
  ptrdiff_t len = 0;
  if (nv.copy_string_contents(val, nullptr, len)) {
    std::string ret(len - 1, '\0');
    if (nv.copy_string_contents(val, &ret[0], len)) {
      return ret;
    }
  }
  nv.maybe_non_local_exit();
  throw std::runtime_error("String conversion failed");
}

inline value to_emacs(expected_type_t<bool>, env nv, bool x) { return nv.intern(x ? "t" : "nil"); }
inline bool from_emacs(expected_type_t<bool>, env nv, value x) { return nv.is_not_nil(x); }

/** Convert a C++ integer to an Emacs integer. */
template <typename T> inline std::enable_if_t<
  std::is_integral_v<T> && (std::numeric_limits<T>::digits <= std::numeric_limits<intmax_t>::digits),
  value> to_emacs(expected_type_t<T>, env nv, T n)
{ return nv.make_integer(static_cast<intmax_t>(n)); }

static constexpr ptrdiff_t uintmax_limb_count =
      1 + // ceil division
      (std::numeric_limits<emacs_limb_t>::digits - 1) /
      std::numeric_limits<uintmax_t>::digits;

#if (EMACS_MAJOR_VERSION >= 27) && defined(__cpp_if_constexpr)
#define CPPEMACS_HAS_UINTMAX_CONVERSION 1

/** Convert a C++ integer to an Emacs integer. */
inline value to_emacs(expected_type_t<uintmax_t>, env nv, uintmax_t n) {
  CPPEMACS_CHECK_VERSION(nv, 27);
  // uintmax_t may overflow intmax_t (bad), so fallback to the bigint version
  if (n > static_cast<uintmax_t>(std::numeric_limits<intmax_t>::max())) {
    emacs_limb_t magnitude[uintmax_limb_count] = {0};
    if constexpr (uintmax_limb_count == 1) {
      magnitude[0] = static_cast<emacs_limb_t>(n);
    } else {
      for (int ii = 0; ii < uintmax_limb_count && n > 0;
           ++ii, n >>= std::numeric_limits<emacs_limb_t>::digits) {
        magnitude[ii] = n & std::numeric_limits<emacs_limb_t>::max();
      }
    }
    return nv.make_big_integer(1, uintmax_limb_count, magnitude);
  } else {
    return to_emacs(expected_type_t<intmax_t>{}, nv, static_cast<intmax_t>(n));
  }
}
#endif

/** Convert an Emacs integer to a C++ integer. */
template <typename T> inline std::enable_if_t<
  std::is_integral_v<T> && (std::numeric_limits<T>::digits <= std::numeric_limits<intmax_t>::digits),
  T>
from_emacs(expected_type_t<T>, env nv, value val) {
  intmax_t int_val = nv.extract_integer(val);
  nv.maybe_non_local_exit();
  if (int_val < static_cast<intmax_t>(std::numeric_limits<T>::min())
      || int_val > static_cast<intmax_t>(std::numeric_limits<T>::max())) {
    throw std::runtime_error("Integer out of range");
  }
  return static_cast<T>(int_val);
}

#ifdef CPPEMACS_HAS_UINTMAX_CONVERSION
inline uintmax_t from_emacs(expected_type_t<uintmax_t>, env nv, value val) {
  CPPEMACS_CHECK_VERSION(nv, 27);
  // always use bigint conversion
  int sign = 0;
  ptrdiff_t count = uintmax_limb_count;
  emacs_limb_t magnitude[uintmax_limb_count] = {0};
  if (!nv.extract_big_integer(val, &sign, &count, magnitude)
      || sign < 0) {
    nv.maybe_non_local_exit();
    throw std::runtime_error("Integer out of range");
  }
  if constexpr (std::is_same_v<emacs_limb_t, uintmax_t>) {
    return magnitude[0];
  } else {
    constexpr size_t ndigits =
      std::numeric_limits<uintmax_t>::digits
      % std::numeric_limits<emacs_limb_t>::digits;
    if (magnitude[uintmax_limb_count - 1] >= (1 << ndigits)) {
      throw std::runtime_error("Integer out of range");
    }
    uintmax_t ret = 0;
    for (int ii = uintmax_limb_count - 1; ii >= 0; --ii) {
      ret <<= std::numeric_limits<emacs_limb_t>::digits;
      ret |= static_cast<uintmax_t>(magnitude[ii]);
    }
    return ret;
  }
}
#endif

/** Convert a C++ float to an Emacs float. */
template <typename T> inline std::enable_if_t<std::is_floating_point_v<T>, value>
to_emacs(expected_type_t<T>, env nv, T d)
{ return nv.make_float(d); }

/** Convert an Emacs float to a C++ float. */
template <typename T> inline std::enable_if_t<std::is_floating_point_v<T>, T>
from_emacs(expected_type_t<T>, env nv, value val)
{ return static_cast<T>(nv.extract_float(val)); }

#if (EMACS_MAJOR_VERSION >= 27)
/** Convert a C timespec to an Emacs timespec */
inline value to_emacs(expected_type_t<struct timespec>, env nv, struct timespec time)
{ CPPEMACS_CHECK_VERSION(nv, 27); return nv.make_time(time); }
/** Convert a C timespec to an Emacs timespec */
inline struct timespec from_emacs(expected_type_t<struct timespec>, env nv, value val)
{ CPPEMACS_CHECK_VERSION(nv, 27); return nv.extract_time(val); }
#endif

}

#endif /* CPPEMACS_CONVERSIONS_HPP_ */
