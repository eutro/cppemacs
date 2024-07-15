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
#include <limits>

/**
 * @defgroup cppemacs_conversions Type Conversions
 * @brief Conversion functions between C++ and Emacs values.
 *
 * Almost all interfaces with Emacs will want to perform some
 * conversion of values. cppemacs has a unified way to define global
 * conversion methods, and has convenient syntaxes for performing
 * these conversions.
 *
 * The easiest way to convert between C++ and Emacs values is with
 * envw::extract()/@ref cell::extract() (Emacs to C++) and envw::inject()/@ref
 * envw::operator->*() "->*" (C++ to Emacs). The procedures by which
 * values are converted are described by @ref
 * cppemacs::from_emacs_convertible "from_emacs_convertible" and @ref
 * cppemacs::to_emacs_convertible "to_emacs_convertible",
 * respectively.
 *
 * @note String literals are converted to @e symbols instead of Emacs
 * @e strings, because they are far more common. The types
 * `std::string`, C++17 `std::string_view`, and @ref cppemacs::literals::estring_literal can
 * be used to make Emacs strings instead.
 *
 * Some example conversions:
 * @snippet core_examples.cpp Conversions
 *
 * @addtogroup cppemacs_conversions
 * @{
 */

#if CPPEMACS_ENABLE_GMPXX
#  include <gmpxx.h>
#  include <memory>
#endif

namespace cppemacs {

namespace detail {

/** @brief true if T is an integral and its range of values is a subseet of intmax_t */
template <typename T>
struct is_integral_smaller_than_intmax : std::integral_constant<bool, (
  std::is_integral<T>::value &&
  (std::numeric_limits<T>::digits <= std::numeric_limits<intmax_t>::digits)
)> {};

#if (EMACS_MAJOR_VERSION >= 27) && !defined(CPPEMACS_DOXYGEN_RUNNING)
/** @brief number of `emacs_limb_t`s required to represent a `uintmax_t` */
static constexpr ptrdiff_t uintmax_limb_count =
  1 + // ceil division
  (std::numeric_limits<emacs_limb_t>::digits - 1) /
  std::numeric_limits<uintmax_t>::digits;
#define CPPEMACS_UINTMAX_ONE_LIMB (EMACS_LIMB_MAX >= UINTMAX_MAX)
static_assert(CPPEMACS_UINTMAX_ONE_LIMB == (detail::uintmax_limb_count == 1), "Inconsistent limb size detection");
#endif

}

/**
 * @brief Convert a string constant to a symbol.
 */
inline value to_emacs(expected_type_t<const char *>, envw nv, const char *name)
{ return nv.intern(name); }

/** @brief Convert a C++ string to an Emacs string. */
inline value to_emacs(expected_type_t<std::string>, envw nv, const std::string &str)
{ return nv.make_string(str.data(), str.length()); }

#ifdef CPPEMACS_HAVE_STRING_VIEW
/** @brief Convert a C++ string view to an Emacs string. */
inline value to_emacs(expected_type_t<std::string_view>, envw nv, const std::string_view &str)
{ return nv.make_string(str.data(), str.length()); }
#endif

/** @brief Return Emacs `nil`. */
inline value to_emacs(expected_type_t<std::nullptr_t>, envw nv, std::nullptr_t) { return nv.intern("nil"); }

/** @brief Convert x to Emacs `t` or `nil`. */
inline value to_emacs(expected_type_t<bool>, envw nv, bool x) { return nv.intern(x ? "t" : "nil"); }
/** @brief Convert x to bool, true if non-nil. */
inline bool from_emacs(expected_type_t<bool>, envw nv, value x) { return nv.is_not_nil(x); }

/** @brief Convert a C++ integer to an Emacs integer.
 *
 * This only works with integral types that are no larger than
 * `intmax_t`. From Emacs 27, `uintmax_t` is also supported via
 * biginteger conversions.
 */
template <typename Int, detail::enable_if_t<detail::is_integral_smaller_than_intmax<Int>::value, bool> = true>
value to_emacs(expected_type_t<Int>, envw nv, Int n)
{ return nv.make_integer(static_cast<intmax_t>(n)); }

#if (EMACS_MAJOR_VERSION >= 27)

/** @brief Convert a C++ `uintmax_t` to an Emacs integer. Emacs 27+ only. */
inline value to_emacs(expected_type_t<uintmax_t>, envw nv, uintmax_t n) {
  nv.check_compatible<27>();
  using namespace detail;
  // uintmax_t may overflow intmax_t (bad), so fallback to the bigint version
  if (n > static_cast<uintmax_t>(std::numeric_limits<intmax_t>::max())) {
    emacs_limb_t magnitude[uintmax_limb_count] = {0};
#if CPPEMACS_UINTMAX_ONE_LIMB
    magnitude[0] = static_cast<emacs_limb_t>(n);
#else
    for (int ii = 0; ii < uintmax_limb_count && n > 0;
         ++ii, n >>= std::numeric_limits<emacs_limb_t>::digits) {
      magnitude[ii] = n & std::numeric_limits<emacs_limb_t>::max();
    }
#endif
    return nv.make_big_integer(1, uintmax_limb_count, magnitude);
  } else {
    return to_emacs(expected_type_t<intmax_t>{}, nv, static_cast<intmax_t>(n));
  }
}
#endif

namespace detail {
template <typename Int>
[[noreturn]] inline void throw_out_of_range(envw nv, value val) {
  throw signal(
    (nv->*"args-out-of-range"),
    nv.funcall(nv->*"list", {
        val,
        nv->*std::numeric_limits<Int>::min(),
        nv->*std::numeric_limits<Int>::max()
      }));
}
}

/** @brief Convert an Emacs integer to a C++ integer.
 *
 * This only works with integral types that are no larger than
 * `intmax_t`. From Emacs 27, `uintmax_t` is also supported via
 * biginteger conversions.
 */
template <typename Int, detail::enable_if_t<detail::is_integral_smaller_than_intmax<Int>::value, bool> = true>
inline Int from_emacs(expected_type_t<Int>, envw nv, value val) {
  intmax_t int_val = nv.extract_integer(val);
  nv.maybe_non_local_exit();
  if (int_val < static_cast<intmax_t>(std::numeric_limits<Int>::min())
      || int_val > static_cast<intmax_t>(std::numeric_limits<Int>::max())) {
    detail::throw_out_of_range<Int>(nv, val);
  }
  return static_cast<Int>(int_val);
}

#if (EMACS_MAJOR_VERSION >= 27)

/** @brief Convert an Emacs integer to a C++ `uintmax_t`. Emacs 27+ only. */
inline uintmax_t from_emacs(expected_type_t<uintmax_t>, envw nv, value val) {
  nv.check_compatible<27>();
  using namespace detail;
  // always use bigint conversion
  int sign = 0;
  ptrdiff_t count = uintmax_limb_count;
  emacs_limb_t magnitude[uintmax_limb_count] = {0};
  if (!nv.extract_big_integer(val, &sign, &count, magnitude)
      || sign < 0) {
    nv.maybe_non_local_exit();
    detail::throw_out_of_range<uintmax_t>(nv, val);
  }
  constexpr size_t extra_digits =
    std::numeric_limits<uintmax_t>::digits
    % std::numeric_limits<emacs_limb_t>::digits;
  // uintmax_t is (by definition) the biggest integer type, and it
  // would be bizarre for this not to be a multiple of size_t (which
  // is what emacs_limb_t) is
  static_assert(extra_digits == 0,
                "UINTMAX_DIGITS must be a multiple of EMACS_LIMB_DIGITS\n"
                "Please tell me what platform you are using!");
#if CPPEMACS_UINTMAX_ONE_LIMB
  return static_cast<uintmax_t>(magnitude[0]);
#else
  uintmax_t ret = 0;
  for (int ii = uintmax_limb_count - 1; ii >= 0; --ii) {
    ret <<= std::numeric_limits<emacs_limb_t>::digits;
    ret |= static_cast<uintmax_t>(magnitude[ii]);
  }
  return ret;
#endif
}
#endif

/** @brief Convert a C++ float to an Emacs float.
 *
 * This always goes through `double` first, regardless of `Float`.
 */
template <typename Float, detail::enable_if_t<std::is_floating_point<Float>::value, bool> = true>
inline value to_emacs(expected_type_t<Float>, envw nv, Float d)
{ return nv.make_float(static_cast<double>(d)); }

/** @brief Convert an Emacs float to a C++ float.
 *
 * This always goes through `double` first, regardless of `Float`.
 */
template <typename Float, detail::enable_if_t<std::is_floating_point<Float>::value, bool> = true>
inline Float from_emacs(expected_type_t<Float>, envw nv, value val)
{ return static_cast<Float>(nv.extract_float(val)); }

#if (EMACS_MAJOR_VERSION >= 27)
/** @brief Convert a C timespec to an Emacs timespec. Emacs 27+ only. */
inline value to_emacs(expected_type_t<struct timespec>, envw nv, struct timespec time)
{ nv.check_compatible<27>(); return nv.make_time(time); }
/** @brief Convert a C timespec to an Emacs timespec. Emacs 27+ only. */
inline struct timespec from_emacs(expected_type_t<struct timespec>, envw nv, value val)
{ nv.check_compatible<27>(); return nv.extract_time(val); }
#endif

#if ((EMACS_MAJOR_VERSION >= 27) && CPPEMACS_ENABLE_GMPXX) || defined(CPPEMACS_DOXYGEN_RUNNING)

/**
 * @brief Convert a GMP integer to an Emacs integer.
 *
 * Emacs 27+ only. Requires `CPPEMACS_ENABLE_GMPXX` to be defined
 * before including this file.
 */
inline value to_emacs(expected_type_t<mpz_class>, envw nv, const mpz_class &zc) {
  nv.check_compatible<27>();
  nv.maybe_non_local_exit();

  mpz_srcptr z = zc.get_mpz_t();
  if (mpz_fits_slong_p(z)) {
    return nv.make_integer(mpz_get_si(z));
  }

  int sign = mpz_sgn(z);
  size_t count = 0;
  size_t nwords = mpz_sizeinbase(z, 2) / std::numeric_limits<emacs_limb_t>::digits;
  std::unique_ptr<emacs_limb_t[]> magnitude(new emacs_limb_t[nwords]);
  mpz_export(
    magnitude.get(), &count,
    -1, sizeof(emacs_limb_t), 0, 0, // LE limbs, native byte order, full words
    z
  );

  return nv.make_big_integer(sign, count, magnitude.get());
}

/**
 * @brief Convert an Emacs integer to a GMP integer.
 *
 * Emacs 27+ only. Requires `CPPEMACS_ENABLE_GMPXX` to be defined
 * before including this file.
 */
inline mpz_class from_emacs(expected_type_t<mpz_class>, envw nv, value x) {
  nv.check_compatible<27>();

  int sign = 0;
  ptrdiff_t count = 0;
  if (nv.extract_big_integer(x, &sign, &count, nullptr)) {
    if (sign == 0) {
      return 0;
    }

    std::unique_ptr<emacs_limb_t[]> magnitude(new emacs_limb_t[count]);
    if (nv.extract_big_integer(x, &sign, &count, magnitude.get())) {
      mpz_class ret;

      mpz_ptr z = ret.get_mpz_t();
      mpz_import(
        z, count,
        -1, sizeof(emacs_limb_t), 0, 0, // LE limbs, native byte order, full words
        magnitude.get()
      );

      if (sign == -1) {
        mpz_neg(z, z);
      }

      return ret;
    }
  }

  nv.maybe_non_local_exit();
  throw std::runtime_error("Bigint conversion failed");
}
#endif

/** @} */

}

#endif /* CPPEMACS_CONVERSIONS_HPP_ */
