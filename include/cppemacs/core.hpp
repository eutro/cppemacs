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

#ifndef CPPEMACS_EMACS_HPP_
#define CPPEMACS_EMACS_HPP_

#include <cassert>
#include <emacs-module.h>

#include <exception>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <type_traits>

/**
 * @page GFDL-1.3-or-later GNU Free Documentation License
 *
 * @include{doc,raise=1} GFDL-1.3-or-later.md
 */

/**
 * @brief cppemacs is a C++11 API wrapper for writing Emacs
 * @ref dynamic_modules "Dynamic Modules".
 *
 * Features include:
 *
 * - A wrapper for the environment struct: @ref cppemacs::envw, which allows
 *   invoking all @manlink{Emacs API functions|Writing-Dynamic-Modules.html} as
 *   member functions.
 *
 * - A collection of easy-to-use @ref cppemacs_conversions "type conversions"
 *   that make creating and inspecting Lisp values a breeze.
 *
 * - Various @ref cppemacs_utilities "utilities", like @ref
 *   make_spreader_function() that make writing functions even easier.
 *
 * Here is a brief example:
 *
 * @snippet brief_example.cpp Brief Example
 *
 * @spoiler{Longer Example}
 * @snippet hello_world.cpp Full example
 * @endspoiler
 */
namespace cppemacs {}

/**
 * @mainpage
 *
 * Copyright (C) 2024 Eutro <https://eutro.dev>
 *
 * Permission is granted to copy, distribute and/or modify this document under
 * the terms of the GNU Free Documentation License, Version 1.3 or any later
 * version published by the Free Software Foundation; with no Invariant
 * Sections, no Front-Cover Texts, and no Back-Cover Texts. A copy of the
 * license is included in the section entitled \"@subpage "GFDL-1.3-or-later"\".
 *
 * # Introduction
 *
 * @copydoc cppemacs
 *
 * ## The Emacs Lisp Reference Manual
 *
 * This document is intended to be accompanied by the <a href="@manloc">GNU
 * Emacs Lisp Reference Manual</a>, in particular the section titled
 * @manlink{Writing Dynamic Modules|Writing-Dynamic-Modules.html|.} The user
 * is expected to be reasonably familiar with Emacs Lisp, though not necessarily
 * with @manlink{dynamic modules|Dynamic-Modules.html|,} and to reference the
 * manual and/or the source code of Emacs for the documentation and behavior of
 * functions that are not part of cppemacs itself.
 *
 * # Dynamic Modules
 * @anchor dynamic_modules
 *
 * An @manlink{dynamic Emacs module|Dynamic-Modules.html|} is any shared library
 * that exports the following two (`extern "C"`) symbols, as described in the
 * @manlink{manual|Module-Initialization.html|.}
 *
 * @code
 * // Include cppemacs headers
 * #include <cppemacs/all.hpp>
 *
 * // Indicate that the dynamic module's code is released under the GPL or compatible license.
 * int plugin_is_GPL_compatible;
 *
 * // Called when the module is loaded.
 * int emacs_module_function(emacs_runtime *rt) noexcept {
 *   // Perform module initialization, e.g. registering functions with `defalias`.
 * }
 * @endcode
 *
 * Such a module needs to be compiled to a shared library (`.so`, `.dll`,
 * `.dynlib`):
 *
 * @spoiler{Command Line}
 * @code{py}
 * cc -fPIC -I"/path/to/cppemacs/include" -c my_dynamic_module.c
 * cc -shared my_dynamic_module.o -o my_dynamic_module.so
 * @endcode
 * @endspoiler
 *
 * @par
 * @spoiler{CMake}
 * @code{cmake}
 * # See README.md for how to install/acquire
 * find_package(cppemacs REQUIRED)
 *
 * # Create a shared library `my_dynamic_module`.
 * add_library(my_dynamic_module SHARED my_dynamic_module.cpp)
 *
 * # Link against `cppemacs` (adds include paths).
 * target_link_libraries(my_dynamic_module cppemacs)
 * @endcode
 * @endspoiler
 *
 * It can then be loaded in Emacs using `require`, `load`, or the low-level
 * `module-load`:
 *
 * @spoiler{Emacs Lisp}
 * @code{lisp}
 * (add-to-list 'load-path "/path/to/containing/directory")
 * (load "my_dynamic_module")
 * ;; better as long as you (provide 'my_dynamic_module)
 * (require 'my_dynamic_module)
 *
 * ;; specify the full path including platform-specific extension
 * (module-load "/path/to/containing/directory/my_dynamic_module.so")
 * @endcode
 * @endspoiler
 *
 * # Interfacing with Emacs
 *
 * The Emacs module API is exposed through an @ref emacs_env "environment", a
 * set of functions as exposed by the wrapper @ref envw. These functions provide
 * ways to inspect and create Emacs lisp @ref value "values", handle error
 * conditions and non-local exits. More details about the C module API are
 * described in the @manlink{manual|Writing-Dynamic-Modules.html|.}
 *
 * cppemacs greatly simplifies the use of this interface, by providing a set of
 * @ref cppemacs_conversions "type-based conversions" to and from Emacs values,
 * a @ref cppemacs::envw::operator->*() "simple syntax" to perform them, @ref
 * cppemacs::cell::operator()() "function-call syntax", and
 * @ref cppemacs::envw::run_catching() "exception handling".
 *
 * @snippet core_examples.cpp Interfacing with Emacs
 */

/**
 * @defgroup cppemacs_core Core Types
 *
 * @brief Core wrapper type definitions for the Emacs module API.
 */

/**
 * @defgroup cppemacs_optional Optional Features
 *
 * @brief Optional features, which can be enabled with pre-defined macros.
 *
 * @addtogroup cppemacs_optional
 * @{
 */
#ifndef CPPEMACS_ENABLE_GMPXX
/**
 * @brief Define as 1 before including <@ref cppemacs/core.hpp> to enable @ref
 * cppemacs_conversions "conversions" for `mpz_class` from <a
 * href="https://gmplib.org/manual/C_002b_002b-Interface-General">gmpxx</a>.
 */
#  define CPPEMACS_ENABLE_GMPXX 0
#endif

#ifndef CPPEMACS_ENABLE_EXCEPTION_BOXING
/**
 * @brief Define as 1 before including <@ref cppemacs/core.hpp> to more tightly
 * integrate Emacs signals with C++ exceptions, at the cost of performance.
 *
 * Specifically, unless overridden with their `Unbox` or `Box` template
 * parameters:
 *
 * - @ref cppemacs::envw::maybe_non_local_exit() "envw::maybe_non_local_exit()"
 *   will @ref cppemacs::envw::rethrow_non_local_exit() "rethrow" the non-local
 *   exit, so it can be caught as @ref cppemacs::signal or @ref
 *   cppemacs::thrown, and without leaving the non-local exit pending. Most
 *   parts of cppemacs that fail using exceptions use `maybe_non_local_exit()`
 *   to raise the error.
 *
 * - @ref cppemacs::envw::run_catching() "envw::run_catching()" stores C++
 *   exceptions as `user-ptr`s of `std::exception_ptr`s, and @ref
 *   cppemacs::envw::rethrow_non_local_exit() "envw::rethrow_non_local_exit()"
 *   unboxes them.
 */
#  define CPPEMACS_ENABLE_EXCEPTION_BOXING 0
#endif

/**@}*/
/**
 * @addtogroup cppemacs_core
 * @{
 */
namespace cppemacs {

// emacs-module.h only defines it in/after Emacs 27
// (with CMake we could potentially pass it as part of the build?)
#ifndef EMACS_MAJOR_VERSION
#  ifdef CPPEMACS_DOXYGEN_RUNNING
/**
 * @brief The @e compile-time major version of Emacs.
 *
 * This is defined in `emacs-module.h` since Emacs 27, and falls back to 25 if
 * that is not found. The value shown in documentation is the highest that
 * cppemacs supports.
 *
 * If you might be on exactly Emacs 26 and desperately need @ref
 * cppemacs::envw::should_quit() "envw::should_quit()", define the secret
 * `CPPEMACS_MAJOR_VERSION_FALLBACK` as 25 or 26 before including this file, and
 * please tell me! I'd like to know what you're doing.
 *
 * @see @ref cppemacs::envw::is_compatible() "envw::is_compatible()" for
 * checking @e runtime version compatibility.
 */
#    define EMACS_MAJOR_VERSION 29
#  else
#    ifndef CPPEMACS_MAJOR_VERSION_FALLBACK
#      define CPPEMACS_MAJOR_VERSION_FALLBACK 25
#    endif
#    define EMACS_MAJOR_VERSION CPPEMACS_MAJOR_VERSION_FALLBACK
#  endif
#endif

namespace detail {
/** @brief C++14 @c std::enable_if_t */
template <bool B, class T = void> using enable_if_t = typename std::enable_if<B, T>::type;
/** @brief C++14 @c std::decay_t */
template <typename T> using decay_t = typename std::decay<T>::type;
/** @brief C++14 @c std::remove_reference_t */
template <typename T> using remove_reference_t = typename std::remove_reference<T>::type;
/** @brief C++14 @c std::void_t */
template <typename...T> using void_t = void;

#if (defined(__cpp_lib_is_invocable) || (__cplusplus >= 201703L)) || defined(CPPEMACS_DOXYGEN_RUNNING)
#define CPPEMACS_HAVE_IS_INVOCABLE 1
/** @brief C++17 @c std::invoke_result_t approximation. */
template <typename T, typename ...Args> using invoke_result_t = std::invoke_result_t<T, Args...>;
#else
template <typename T, typename ...Args> using invoke_result_t =
  decltype(std::declval<detail::decay_t<T>>()(std::declval<Args>()...));
#endif

#if (defined(__cpp_lib_integer_sequence) || (__cplusplus >= 201304L)) || defined(CPPEMACS_DOXYGEN_RUNNING)
/** @brief C++14 `std::index_sequence` */
template <size_t...Idx> using index_sequence = std::index_sequence<Idx...>;
/** @brief C++14 `std::make_index_sequence` */
template <size_t N> using make_index_sequence = std::make_index_sequence<N>;
#else
template <size_t...Idx> struct index_sequence {};

template <size_t N, typename> struct index_sequence_snoc;
template <size_t N, size_t...Idx> struct index_sequence_snoc<N, index_sequence<Idx...>>
{ using type = index_sequence<Idx..., N>; };
template <size_t N> struct make_index_sequence_ :
    index_sequence_snoc<N, typename make_index_sequence_<N - 1>::type> {};
template <> struct make_index_sequence_<0> { using type = index_sequence<>; };

template <size_t N> using make_index_sequence = typename make_index_sequence_<N>::type;
#endif
};

// aliases to core types
/**
 * @brief Raw Emacs environment. You should use @ref envw in most cases.
 *
 * @code
 * value some_module_function(emacs_env *raw_env, ptrdiff_t nargs, value *args, void *) noexcept {
 *   envw env = raw_env;
 *   // ...
 * }
 * @endcode
 */
using emacs_env = ::emacs_env;

/** @brief Synonymous with @ref value. */
using emacs_value = ::emacs_value;
/**
 * @brief Raw opaque pointer representing an Emacs value. See @ref cell for the wrapper,
 * and @ref envw for functions that operate on these.
 *
 * @code
 * value some_module_function(emacs_env *env, ptrdiff_t nargs, value *args, void *) noexcept {
 *   cell arg1(env, args[0]);
 *   // ...
 * }
 * @endcode
 */
using value = emacs_value;

/**
 * @brief Struct passed to emacs_module_init().
 *
 * This struct has two (public) fields:
 *
 * - `ptrdiff_t size`, the size, in bytes, of @ref emacs_runtime in the running
 * Emacs binary.
 *
 * - <code>@ref cppemacs::emacs_env "emacs_env" *(*get_environment)(@ref
 * cppemacs::emacs_runtime "emacs_runtime" *runtime)</code>, which returns an
 * @ref emacs_env that can be used until emacs_module_init() returns.
 *
 * @code
 * extern "C" int emacs_module_init(emacs_runtime *rt) noexcept {
 *   if (rt->size < sizeof(*rt)) return 1; // check size (though it is unlikely to change)
 *   envw env = rt->get_environment(rt);
 *   // ...
 * }
 * @endcode
 */
using emacs_runtime = ::emacs_runtime;

}

/**
 * @brief The main entrypoint of the dynamic module.
 * @manual{Module-Initialization.html#index-emacs_005fmodule_005finit-1}
 *
 * @param runtime The @ref cppemacs::emacs_runtime "emacs_runtime", which can be
 * used to obtain an @ref cppemacs::emacs_env "emacs_env" to access the module API.
 *
 * @return The exit status. If this is non-zero, Emacs will @manlink{raise a
 * signal|Signaling-Errors.html#index-signal} `module-init-failed` with it.
 *
 * @code
 * using namespace cppemacs;
 * extern "C" int emacs_module_init(emacs_runtime *rt) noexcept {
 *   // check size (though it is unlikely to change)
 *   if (rt->size < sizeof(*rt)) return 1;
 *   envw env = rt->get_environment(rt);
 *
 *   // check the size of the environment
 *   if (!env.is_compatible<27>()) return 2;
 *   // ...
 * }
 * @endcode
 */
extern "C" int emacs_module_init(cppemacs::emacs_runtime *runtime) noexcept;

namespace cppemacs {

/** @brief Possible function call outcomes. @manual{Module-Nonlocal.html} */
using emacs_funcall_exit = ::emacs_funcall_exit;
/** @brief envw::process_input() outcomes. */
using emacs_process_input_result = ::emacs_process_input_result;

/** @brief MAX_ARITY for variadic functions. */
static constexpr ptrdiff_t emacs_variadic_function = ::emacs_variadic_function;

#if (defined(__cpp_lib_string_view) || (__cplusplus >= 201606L)) || defined(CPPEMACS_DOXYGEN_RUNNING)
#define CPPEMACS_HAVE_STRING_VIEW 1
#endif

// before C++17, the standard forbids noexcept in typedefs, but not in function parameters,
// so when using these as function parameters they should (conditionally) be expanded
#if (defined(__cpp_noexcept_function_type) || (__cplusplus >= 201703L)) || defined(CPPEMACS_DOXYGEN_RUNNING)
#define CPPEMACS_HAVE_NOEXCEPT_TYPEDEFS 1
/**
 * @brief A raw module function, for envw::make_function().
 *
 * @warning Type aliases like this one and emacs_finalizer can only be
 * `noexcept` in C++17 and above
 * ([P0012R1](https://wg21.link/P0012R1)), but function parameters can
 * be `noexcept` even in C++11. Be careful using this as a type that
 * is forwarded to envw::make_function(), as it will not typecheck on
 * certain compilers. Write out the full function type instead, and
 * surround it with @ref CPPEMACS_SUPPRESS_WCOMPAT_MANGLING_BEGIN.
 *
 * @see make_module_function() and make_spreader_function() for convenience
 * functions that produce @ref emacs_function "emacs_functions" from C++
 * closures.
 */
using emacs_function = value (*)(emacs_env *env, ptrdiff_t nargs, value *args, void *data) noexcept;
/** @brief A finalizer for envw::make_user_ptr().
 * @see emacs_function for a note on the `noexcept` specifier. */
using emacs_finalizer = void (*)(void *data) noexcept;

#else
#define CPPEMACS_HAVE_NOEXCEPT_TYPEDEFS 0
using emacs_function = value (*)(emacs_env *env, ptrdiff_t nargs, value *args, void *data);
using emacs_finalizer = void (*)(void *data);

#ifdef __clang__
#  define CPPEMACS_SUPPRESS_WCOMPAT_MANGLING_BEGIN \
  _Pragma("clang diagnostic push") \
  _Pragma("clang diagnostic ignored \"-Wc++17-compat-mangling\"")
#  define CPPEMACS_SUPPRESS_WCOMPAT_MANGLING_END \
  _Pragma("clang diagnostic pop")
#endif

#endif

#ifndef CPPEMACS_SUPPRESS_WCOMPAT_MANGLING_BEGIN
/**
 * @brief On Clang, begin suppressing `-Wc++17-compat-mangling`.
 *
 * Procedures which take @ref cppemacs::emacs_function "emacs_function" or @ref
 * cppemacs::emacs_finalizer "emacs_finalizer" as parameters should have them
 * annotated with noexcept in order to match the C types, but before C++17 these
 * are not (in the standard) part of the type system, which breaks name
 * mangling.
 *
 * This warning can safely be ignored, since all affected procedures
 * are inline, and no attempt is made to preserve binary compatibility.
 *
 * This must be paired with a matching @ref
 * CPPEMACS_SUPPRESS_WCOMPAT_MANGLING_END.
 */
#  define CPPEMACS_SUPPRESS_WCOMPAT_MANGLING_BEGIN
/** @brief On Clang, end suppressing `-Wc++17-compat-mangling`. */
#  define CPPEMACS_SUPPRESS_WCOMPAT_MANGLING_END
#endif

#if (EMACS_MAJOR_VERSION >= 27)
/** @brief An element of big integer magnitude arrays. @manual{Module-Values.html#index-emacs_005flimb_005ft} */
using emacs_limb_t = ::emacs_limb_t;
#endif

/** @brief Enum wrapper for @ref emacs_funcall_exit.
 *
 * When @ref operator bool() "converted to bool", this is true if there is a
 * @ref signal_ "signal" or @ref throw_ "throw" pending.
 *
 * @see envw::non_local_exit_check() and envw::non_local_exit_get() for
 * obtaining the current function exit status.
 *
 * @manual{Module-Nonlocal.html}
 */
struct funcall_exit {
  /** @brief The underlying @ref emacs_funcall_exit. */
  emacs_funcall_exit raw;
  /** @brief Construct the wrapper. */
  constexpr funcall_exit(emacs_funcall_exit raw) noexcept: raw(raw) {}
  /** @brief Get the underlying @ref emacs_funcall_exit. */
  constexpr operator emacs_funcall_exit() const noexcept { return raw; }
  /** @brief Get the underlying @ref emacs_funcall_exit. */
  constexpr emacs_funcall_exit get() const noexcept { return raw; }

  /** @brief Function has returned normally.  */
  static constexpr emacs_funcall_exit return_ = emacs_funcall_exit_return;
  /** @brief Function has signaled an error using `signal`.  */
  static constexpr emacs_funcall_exit signal_ = emacs_funcall_exit_signal;
  /** @brief Function has exited using `throw`.  */
  static constexpr emacs_funcall_exit throw_ = emacs_funcall_exit_throw;

  /** @brief True if this is not equal to @ref return_. */
  constexpr operator bool() const { return raw != return_; }
};

/** @brief Enum wrapper for @ref emacs_process_input_result.
 *
 * @manual{Module-Misc.html#index-process_005finput} */
struct process_input_result {
  /** @brief The underlying @ref emacs_process_input_result. */
  emacs_process_input_result raw;
  /** @brief Construct the wrapper. */
  constexpr process_input_result(emacs_process_input_result raw) noexcept: raw(raw) {}
  /** @brief Get the underlying @ref emacs_process_input_result. */
  constexpr operator emacs_process_input_result() const noexcept { return raw; }
  /** @brief Get the underlying @ref emacs_process_input_result. */
  constexpr emacs_process_input_result get() const noexcept { return raw; }

  /** @brief Module code may continue  */
  static constexpr emacs_process_input_result continue_ = emacs_process_input_continue;
  /** @brief Module code should return control to Emacs as soon as possible.  */
  static constexpr emacs_process_input_result quit_ = emacs_process_input_quit;

  /** @brief True if this is not equal to @ref continue_. */
  constexpr operator bool() const { return raw != continue_; }
};

/** @brief A valueless exception indicating a pending non-local exit.
 *
 * @warning The specific non-local exit should @b not be cleared if this is
 * thrown.
 *
 * @see
 * envw::maybe_non_local_exit(), which raises this exception.
 *
 * envw::non_local_exit_check(), which indicates if there is a pending non-local exit.
 */
struct non_local_exit {};

/**
 * @brief An exception representing an Emacs `signal` exit.
 *
 * @warning This should not be constructed or thrown if there is already a
 * pending non-local exit, as Emacs module API functions will not return
 * meaningful values.
 *
 * @see
 * envw::run_catching(), which raises an Emacs signal if this is caught.
 *
 * envw::rethrow_non_local_exit(), which throws this if an Emacs signal is pending.
 */
struct signal {
  /** @brief The ERROR-SYMBOL of the signal. */
  value symbol;
  /** @brief The associated DATA of the signal. */
  value data;
  /** @brief Construct a signal exception. */
  signal(value symbol, value data) noexcept
    : symbol(symbol), data(data) {}
};

/**
 * @brief An exception representing an Emacs `throw` exit.
 *
 * @warning This should not be constructed or thrown if there is already a
 * pending non-local exit, as Emacs module API functions will not return
 * meaningful values.
 *
 * @see
 * envw::run_catching(), which throws in Emacs if this is caught.
 *
 * envw::rethrow_non_local_exit(), which throws this if an Emacs throw is pending.
 */
struct thrown {
  /** @brief The TAG which is being throw to. */
  value symbol;
  /** @brief The VALUE which is being thrown. */
  value data;
  /** @brief Construct a thrown exception. */
  thrown(value symbol, value data) noexcept
    : symbol(symbol), data(data) {}
};

struct cell;

/**@}*/
/**
 * @addtogroup cppemacs_conversions
 * @{
 */

/**
 * @brief Marker type for @link cppemacs_conversions Emacs type
 * conversions @endlink.
 *
 * When converting from C++ to Emacs, using @ref to_emacs(), this type denotes the
 * expected argument type. This makes overload resolution easier to reason about,
 * since implicit conversions will not be performed unless the overload permits it.
 *
 * When converting from Emacs to C++, using @ref from_emacs(), this type denotes the
 * expected return type of the overload, allowing the overload to be resolved
 * without explicit template parameters.
 *
 * @tparam T The exact decayed (see `std::decay`) type expected by the
 * conversion function.
 */
template <typename T> struct expected_type_t {};

#ifndef CPPEMACS_DOXYGEN_RUNNING
#if defined(__cpp_if_constexpr) || (__cplusplus >= 201606L)
/** @brief Defined if `if constexpr` is available */
#  define CPPEMACS_HAVE_IF_CONSTEXPR 1
#  define CPPEMACS_MAYBE_IF_CONSTEXPR if constexpr
#else
#  define CPPEMACS_MAYBE_IF_CONSTEXPR if
#endif
#endif

/* type conversion concepts, we can declare these now so we can use
   them in `envw` */
#if defined(__cpp_concepts) || defined(CPPEMACS_DOXYGEN_RUNNING)
/** @brief Defined if concepts are available. */
#  define CPPEMACS_HAVE_CONCEPTS 1

/**
 * @brief Specifies that values of type `T` can be
 * @ref cppemacs_conversions "converted" to Emacs values.
 *
 * Any type that has an overload for `to_emacs(expected_type_t<decay_t<T>>,
 * emacs_env *env, T &&t)` using ADL, with a return type convertible to @ref
 * cppemacs::value "value", satisfies this concept.
 *
 * @note There are no other restrictions. In particular, such functions may
 * throw exceptions, though many (particularly those with complicated GC
 * hand-off) may try not to.
 *
 * @code
 * struct my_cool_struct {
 *   int x, y;
 *
 *   // can be defined in the containing namespace, or as a friend, thanks to ADL
 *   friend value to_emacs(expected_type_t<my_cool_struct>, envw env, const my_cool_struct &s) {
 *     return (env->*"cons")(s.x, s.y);
 *   }
 * };
 *
 * static_assert(to_emacs_convertible<my_cool_struct>, "Must be to-Emacs-convertible!");
 * @endcode
 *
 * @see @ref cppemacs::envw::inject() "envw::inject()" and @ref
 * cppemacs::envw::operator->*() "->*", which perform the conversion.
 *
 * @ref cppemacs::from_emacs_convertible "from_emacs_convertible" for the
 * opposite direction.
 *
 * @ref TO_EMACS_TYPE which is provided even before C++20.
 *
 * The @ref cppemacs_conversions "top-level documentation for type conversions".
 */
template <typename T>
concept to_emacs_convertible = requires(T &&a, emacs_env *env) {
  value(to_emacs(
          expected_type_t<detail::decay_t<T>>{},
          env, std::forward<T>(a)));
};

/**
 * @brief Type constraint for @ref cppemacs::to_emacs_convertible "to-Emacs-convertible" types.
 *
 * For use in type template parameter declarations. This expands to
 * `typename` if concepts are unavailable.
 *
 * @code
 * template <TO_EMACS_TYPE T>
 * void use_emacs_convertible(envw env, T &&f) {
 *   value x = env->*std::forward<T>(f);
 *   // ...
 * }
 * @endcode
 */
#define TO_EMACS_TYPE cppemacs::to_emacs_convertible

/**
 * @brief Specifies that Emacs values can be
 * @ref cppemacs_conversions "converted" to values of type T.
 *
 * Any type that has an overload for `from_emacs(expected_type_t<T>, emacs_env
 * *env, value x)` using ADL, with a return type convertible to `T` satisfies
 * this concept.
 *
 * @note There are no other restrictions. In particular, implementations may
 * throw exceptions, and are @e encouraged to, so that they do not return
 * nonsensical data.
 *
 * @code
 * struct my_cool_struct {
 *   int x, y;
 *
 *   // can be defined in the containing namespace, or as a friend, thanks to ADL
 *   friend value from_emacs(expected_type_t<my_cool_struct>, envw env, value x) {
 *     return my_cool_struct{(env->*"car")(x), (env->*"cdr")(x)};
 *   }
 * };
 *
 * static_assert(from_emacs_convertible<my_cool_struct>, "Must be from-Emacs-convertible!");
 * @endcode
 *
 * @see @ref cppemacs::envw::extract() "envw::extract()" and @ref
 * cppemacs::cell::extract() "cell::extract()", which perform the conversion.
 *
 * @ref cppemacs::to_emacs_convertible "to_emacs_convertible" for the
 * opposite direction.
 *
 * @ref FROM_EMACS_TYPE which is provided even before C++20.
 *
 * The @ref cppemacs_conversions "top-level documentation for type conversions".
 */
template <typename T>
concept from_emacs_convertible = requires(value x, emacs_env *env) {
  T(from_emacs(expected_type_t<T>{}, env, x));
};

/**
 * @brief Type constraint for @ref cppemacs::from_emacs_convertible
 * "from-Emacs-convertible" types.
 *
 * For use in type template parameter declarations. This expands to
 * `typename` if concepts are unavailable.
 *
 * @code
 * template <FROM_EMACS_TYPE T>
 * void use_emacs_convertible(envw env, value x) {
 *   T x = env.extract<T>(x);
 *   // ...
 * }
 * @endcode
 */
#define FROM_EMACS_TYPE cppemacs::from_emacs_convertible

#else
#define TO_EMACS_TYPE typename
#define FROM_EMACS_TYPE typename
#endif

#ifndef CPPEMACS_DOXYGEN_RUNNING
inline value from_emacs(expected_type_t<value>, emacs_env *, value x) { return x; }
inline value to_emacs(expected_type_t<value>, emacs_env *, value x) { return x; }

inline std::string from_emacs(expected_type_t<std::string>, emacs_env *raw_env, value val) noexcept;
#endif

/**@}*/
/** @addtogroup cppemacs_core
 * @{ */

/**
 * @brief General Emacs environment wrapper.
 *
 * This type wraps the raw @ref emacs_env, providing additional funcionality:
 *
 * - Member functions for invoking all function pointers in @ref emacs_env.
 *   Where C has to do `env->function_name(env, ...)`, C++ can do
 *   `env.function_name(...)` instead.
 *
 * - Conversions @ref inject() "from C++ to Emacs" and @ref extract()
 *   "vice-versa". Most Lisp value creation/inspection can be done this
 *   way. See @ref cppemacs_conversions.
 *
 * - Idiomatic non-local exit handling, via run_catching(),
 *   maybe_non_local_exit(), and rethrow_non_local_exit().
 *
 * @important An environment and most @ref value "values" obtained from it are
 * invalidated when the function it was first obtained in returns (either @ref
 * emacs_module_init() or a @ref make_function() "module function" to which it
 * is a parameter).  Emacs environment functions cannot be used outside of the
 * dynamic extent of these functions and @ref value "values" (except for @ref
 * make_global_ref()), cannot be shared between invocations either.
 *
 * @warning If a `noexcept` method on this class fails, or is called while a
 * @ref non_local_exit_check() "non-local exit is pending", it returns a
 * meaningless result instead of throwing an exception. This is usually
 * harmless, but the program should check and try to return to Emacs as soon as
 * possible.
 *
 * @warning For better integration with C++ exception handling, run_catching()
 * and maybe_non_local_exit() can be used.
 *
 */
CPPEMACS_SUPPRESS_WCOMPAT_MANGLING_BEGIN
struct envw {
private:
  emacs_env *raw;
public:
  /** @brief Construct from a raw @ref emacs_env. */
  constexpr envw(emacs_env *raw) noexcept: raw(raw) {}
  /** @brief Convert to a raw @ref emacs_env. */
  constexpr operator emacs_env *() const noexcept { return raw; }
  /** @brief Reference the raw @ref emacs_env. */
  constexpr emacs_env *operator->() const noexcept { return raw; }
  /** @brief Reference the raw @ref emacs_env. */
  constexpr emacs_env &operator*() const noexcept { return *raw; }

  /**
   * @brief The size, in bytes, of the @ref emacs_env in the running Emacs binary.
   *
   * This can be used to check if the running Emacs version is compatible with
   * the module, since newer Emacs versions have more fields and thus a larger
   * size.
   */
  ptrdiff_t size() const noexcept { return raw->size; }

  /**
   * @brief Check if this environment supports the module API of the given Emacs
   * major version. A check occurs at both compile time and runtime.
   *
   * - At compile time, this fails if `MajorVersion` is greater than @ref
   * EMACS_MAJOR_VERSION or the maximum supported by cppemacs.
   *
   * - At runtime, returns <code>size() >= sizeof(emacs_env_NN)</code>. This
   * cannot distinguish between major versions with no new functions
   * (e.g. is_compatible<29>() will return true even when running Emacs 28).
   *
   * @code
   * int emacs_module_init(emacs_runtime *rt) {
   *   envw env = rt->get_environment(rt);
   *   if (!env.is_compatible<27>()) return 1;
   * }
   * @endcode
   *
   * @see check_compatible() which throws an exception if the runtime check
   * fails.
   */
  template <size_t MajorVersion> bool is_compatible() const noexcept {
    static_assert(MajorVersion <= 29, "This version of cppemacs only supports up to Emacs 29");
    static_assert(MajorVersion <= EMACS_MAJOR_VERSION,
                  "The Emacs version we are compiling against is too low");
    static_assert(MajorVersion >= 25, "The Emacs module API only exists since Emacs 25");
    static_assert(!MajorVersion, "There should be a specialization for is_compatible, this is a bug in cppemacs");
    return false;
  }

private:
#ifndef CPPEMACS_DOXYGEN_RUNNING
  template <size_t MajorVersion>
  struct major_error_message {
    template <char ...C>
    struct inner {
      static constexpr const char value[] = { C..., 0 };
    };
    static constexpr const char *value = inner<
      'E','m','a','c','s',' ','m','a','j','o','r',' ','v','e','r','s','i','o','n',' ',
      '0' + ((MajorVersion / 10) % 10),
      '0' + (MajorVersion % 10),
      ' ','r','e','q','u','i','r','e','d'
      >::value;
  };
#endif
public:

  /**
   * @brief Throw an exception with the error message `"Emacs major version
   * <MajorVersion> required"`.
   *
   * @see is_compatible() and check_compatible()
   */
  template <size_t MajorVersion> [[noreturn]] static void version_check_error() noexcept(false) {
    throw std::runtime_error(major_error_message<MajorVersion>::value);
  }

  /**
   * @brief Check if this environment supports the given major version's module API,
   * and raises an error otherwise.
   *
   * @code
   * int emacs_module_init(emacs_runtime *rt) {
   *   envw env = rt->get_environment(rt);
   *   env.run_catching([&]() {
   *     env.check_compatible<27>();
   *   });
   * }
   * @endcode
   *
   * @see is_compatible() and version_check_error()
   */
  template <size_t MajorVersion> void check_compatible() const noexcept(false) {
    if (!is_compatible<MajorVersion>()) {
      version_check_error<MajorVersion>();
    }
  }

  static_assert(EMACS_MAJOR_VERSION >= 25, "Bad Emacs version");
  /** @name Emacs 25 */
  /**@{*/
  /* Memory management. */
  /**
   * @brief Make a global reference to a value. @manual{Module-Values.html#index-make_005fglobal_005fref}
   *
   * @note This value must be freed exactly once with @ref free_global_ref().
   *
   * @return A @ref value that can be used beyond the lifetime of this environment.
   */
  value make_global_ref(value val) const noexcept { return raw->make_global_ref(raw, val); }
  /**
   * @brief Free a global reference obtained with make_global_ref(). @manual{Module-Values.html#index-free_005fglobal_005fref}
   *
   * @param global_value A global reference obtained with make_global_ref().
   */
  void free_global_ref(value global_value) const noexcept { return raw->free_global_ref(raw, global_value); }

  /* Non-local exit handling. */
  /**
   * @brief Check if there is a non-local exit pending.
   *
   * Emacs lisp has two types of non-local exit: `signal` and `throw`.
   * The semantics of both of these are that they unwind to the
   * nearest enclosing handler that matches (`condition-case` or
   * `throw`). A function can also simply return normally. These
   * different statuses are described by the @ref funcall_exit type.
   *
   * When a non-local exit (`signal` or `throw`) is pending, to ensure
   * correct semantics, module code is expected to yield back to Emacs
   * without performing other work. This method is used to check
   * whether control should return to Emacs quickly. See also the
   * @manlink{manual|Module-Nonlocal.html#index-non_005flocal_005fexit_005fcheck|.}
   *
   * Most code may be better served by maybe_non_local_exit(), which
   * throws C++ exceptions to handle non-local exits. @e This method
   * is best used by code that has intricate error handling, or must
   * be `noexcept`.
   *
   * @code
   * void critical_noexcept_function(envw env) noexcept {
   *   do_something_that_might_fail(env);
   *   if (env.non_local_exit_check()) {
   *     // maybe perform cleanup that wouldn't be better served by a destructor
   *     return;
   *   }
   *   // ...
   * }
   * @endcode
   *
   * @warning If this returns anything other than @ref funcall_exit::return_,
   * then most other methods return meaningless values, unless specified
   * otherwise. Functions that require meaningful values should either return as
   * soon as possible, or use maybe_non_local_exit() to throw an exception.
   *
   * @return The current @ref funcall_exit "function exit status".
   */
  funcall_exit non_local_exit_check() const noexcept { return raw->non_local_exit_check(raw); }
  /** @brief Clear a pending non-local exit. @manual{Module-Nonlocal.html#index-non_005flocal_005fexit_005fclear}
   * @details This can be invoked even with a @ref non_local_exit_check() "non-local exit pending". */
  void non_local_exit_clear() const noexcept { return raw->non_local_exit_clear(raw); }
  /** @brief Get the data of a non-local exit (signal/throw). @manual{Module-Nonlocal.html#index-non_005flocal_005fexit_005fget}
   * @details This can be invoked even with a @ref non_local_exit_check() "non-local exit pending". */
  funcall_exit non_local_exit_get(value &symbol, value &data) const noexcept { return raw->non_local_exit_get(raw, &symbol, &data); }
  /** @brief Raise a signal. @manual{Module-Nonlocal.html#index-non_005flocal_005fexit_005fsignal} */
  void non_local_exit_signal(value symbol, value data) const noexcept { return raw->non_local_exit_signal(raw, symbol, data); }
  /** @brief Throw a value with a tag. @manual{Module-Nonlocal.html#index-non_005flocal_005fexit_005fthrow} */
  void non_local_exit_throw(value symbol, value data) const noexcept { return raw->non_local_exit_throw(raw, symbol, data); }

  /**
   * @brief Check for @ref non_local_exit_check() "non-local exit", and throw it
   * as a C++ exception (@ref signal or @ref thrown) if there is one.
   *
   * Unlike @ref maybe_non_local_exit(), this method @b always clears the
   * pending non-local exit, and does capture the type and its data.
   *
   * @tparam Unbox Whether `cppemacs--exception` signals
   * should be re-raised as their captured C++ exceptions.
   *
   * @throws signal If a signal is pending.
   * @throws thrown If a throw is pending.
   * @throws ... Any C++ exception, if `Unbox` is true and a boxed one is pending
   */
  template <bool Unbox = CPPEMACS_ENABLE_EXCEPTION_BOXING>
  void rethrow_non_local_exit() const noexcept(false) {
    value symbol, data;
    funcall_exit kind = non_local_exit_get(symbol, data);
    if (!kind) return;
    non_local_exit_clear();
    if (kind == funcall_exit::signal_) {
      CPPEMACS_MAYBE_IF_CONSTEXPR (Unbox) {
        if (eq(symbol, intern("error"))) {
          value msg = funcall(intern("car"), {data});
          std::string msg_string = from_emacs(expected_type_t<std::string>{}, *this, msg);
          if (non_local_exit_check()) {
            non_local_exit_clear();
          } else {
            throw std::runtime_error(std::move(msg_string));
          }
        } if (eq(symbol, intern("cppemacs--exception"))) {
          auto eptr = reinterpret_cast<std::exception_ptr *>(get_user_ptr(data));
          if (non_local_exit_check()) {
            non_local_exit_clear();
          } else if (eptr && *eptr) {
            std::rethrow_exception(*eptr);
          }
        }
      }
      throw signal(symbol, data);
    } else {
      throw thrown(symbol, data);
    }
  }

  /**
   * @brief Check for @ref non_local_exit_check() "non-local exit", and throw an
   * exception if there is one.
   *
   * This allows using C++ stack unwinding to return control to Emacs
   * naturally.
   *
   * @tparam Unbox Determines how the non-local exit should be performed:
   * @parblock
   * - If false, this method does @b not clear the pending non-local exit, and
   *   does not capture the type (signal/throw) or its data, it merely throws
   *   @ref non_local_exit.
   *
   * - If true, this method uses rethrow_non_local_exit() (which see) to
   *   re-raise the exception.
   * @endparblock
   */
  template <bool Unbox = CPPEMACS_ENABLE_EXCEPTION_BOXING>
  void maybe_non_local_exit() const noexcept(false) {
    if (non_local_exit_check()) {
      CPPEMACS_MAYBE_IF_CONSTEXPR (Unbox) {
        rethrow_non_local_exit<Unbox>();
      } else {
        throw non_local_exit{};
      }
    }
  }

  /* Function registration. */

  /**
   * @brief Make an Emacs <i>module function</i> from a function pointer and
   * associated data. @manual{Module-Functions.html#index-make_005ffunction}
   *
   * @param min_arity The minimum number of arguments the function can be
   * invoked with.
   *
   * @param max_arity The maximum number of arguments the function can be
   * invoked with.
   *
   * @param func The implementation of the function, which will be called with
   * a new environment, the arguments, and the associated @p data.
   *
   * @param docstring The documentation string of the
   * function. @manual{Function-Documentation.html} In particular, a `(fn
   * arglist)` line at the end is helpful to provide argument names, since they
   * cannot be inferred from the source code.
   *
   * @param data The data to associate with the function, which will be passed
   * to @p func.
   *
   * @see make_module_function() and make_spreader_function() for convenience
   * functions that produce @ref emacs_function "emacs_functions" from C++
   * closures.
   *
   * @b Example
   * @code
   * static value some_module_function(emacs_env *raw_env, ptrdiff_t nargs, value *args, void *data) noexcept {
   *   envw env = raw_env;
   *   value arg1 = args[0];
   *   value arg2 = nargs >= 2 ? args[1] : env.intern("10");
   *   return (env->*"+")(arg1, arg2);
   * }
   *
   * extern "C" int emacs_module_init(emacs_runtime *rt) noexcept {
   *   envw env = rt->get_environment(rt);
   *
   *   value fun = env->make_function(
   *     1, 2, some_module_function,
   *     "R(Add X to Y, or 10 if unspecified.
   *
   * (fn X &optional Y))", nullptr);
   *
   *   (env->*"defalias")("some-module-function", fun);
   *
   *   return 0;
   * }
   * @endcode
   */
  value make_function(ptrdiff_t min_arity, ptrdiff_t max_arity,
#if CPPEMACS_HAVE_NOEXCEPT_TYPEDEFS
                      emacs_function func,
#else
                      value (*func)(emacs_env*, ptrdiff_t, value*, void*) noexcept,
#endif
                      const char *docstring,
                      void *data) const noexcept
  { return raw->make_function(raw, min_arity, max_arity, func, docstring, data); }

  /** @brief Call func with the provided arguments. @manual{Module-Misc.html#index-funcall-1} */
  value funcall(value func, ptrdiff_t nargs, value *args) const noexcept { return raw->funcall(raw, func, nargs, args); }

  /** @brief Call func with the provided arguments. @manual{Module-Misc.html#index-funcall-1} */
  value funcall(value func, std::initializer_list<value> args) const noexcept {
    return funcall(func, args.size(), const_cast<value*>(args.begin()));
  }

  /** @brief Get the interned Emacs symbol with the given null-terminated ASCII
   * name. @manual{Module-Misc.html#index-intern-1}. */
  value intern(const char *name) const noexcept { return raw->intern(raw, name); }

  /* Type conversion. */
  /** @brief Get the type of @e arg as a symbol like `string` or `integer`. @manual{Module-Misc.html#index-type_005fof} */
  value type_of(value arg) const noexcept { return raw->type_of(raw, arg); }
  /** @brief Returns whether @e arg is non-nil. @manual{Module-Misc.html#index-is_005fnot_005fnil} */
  bool is_not_nil(value arg) const noexcept { return raw->is_not_nil(raw, arg); }
  /** @brief Returns whether @e a is the same object as @e b. @manual{Module-Misc.html#index-eq-1} */
  bool eq(value a, value b) const noexcept { return raw->eq(raw, a, b); }
  /** @brief Get the value of an Emacs integer @e arg. @manual{Module-Values.html#index-extract_005finteger} */
  intmax_t extract_integer(value arg) const noexcept { return raw->extract_integer(raw, arg); }
  /** @brief Create an Emacs integer with value @e n. @manual{Module-Values.html#index-make_005finteger} */
  value make_integer(intmax_t n) const noexcept { return raw->make_integer(raw, n); }
  /** @brief Get the value of an Emacs float @e arg. @manual{Module-Values.html#index-extract_005ffloat} */
  double extract_float(value arg) const noexcept { return raw->extract_float(raw, arg); }
  /** @brief Create an Emacs float with value @e d. @manual{Module-Values.html#index-make_005ffloat} */
  value make_float(double d) const noexcept { return raw->make_float(raw, d); }

  /**
   * @brief Copy the content of the Lisp string @e VALUE to @e BUFFER as an utf8
   * null-terminated string.
   *
   * @e SIZE must point to the total size of the buffer.  If @e BUFFER is NULL
   * or if @e SIZE is not big enough, write the required buffer size to SIZE and
   * return true.
   *
   * Note that @e SIZE must include the last null byte (e.g. "abc" needs a
   * buffer of size 4).
   *
   * Return true if the string was successfully copied.
   *
   * @manual{Module-Values.html#index-copy_005fstring_005fcontents}
   */
  bool copy_string_contents(value value, char *buffer, ptrdiff_t &size) const noexcept {
    return raw->copy_string_contents(raw, value, buffer, &size);
  }

  /** @brief Create a Lisp string from a utf8 encoded string. @manual{Module-Values.html#index-make_005fstring} */
  value make_string(const char *str, ptrdiff_t len) const noexcept { return raw->make_string(raw, str, len); }
  /** @brief Create a Lisp string from a null-terminated utf8 string. */
  value make_string(const char *str) const noexcept { return raw->make_string(raw, str, std::char_traits<char>::length(str)); }
  /** @brief Create a Lisp string from a utf8 std::string. */
  value make_string(const std::string &str) const noexcept { return make_string(str.data(), str.length()); }

  /* Embedded pointer type. */
#if CPPEMACS_HAVE_NOEXCEPT_TYPEDEFS
  /** @brief Create a `user-ptr` object which wraps @e ptr. @manual{Module-Values.html#index-make_005fuser_005fptr} */
  value make_user_ptr(emacs_finalizer fin, void *ptr) const noexcept { return raw->make_user_ptr(raw, fin, ptr); }
#else
  value make_user_ptr(void (*fin)(void*) noexcept, void *ptr) const noexcept { return raw->make_user_ptr(raw, fin, ptr); }
#endif

  /** @brief Extract the C pointer from a `user-ptr` object. @manual{Module-Values.html#index-get_005fuser_005fptr} */
  void *get_user_ptr(value arg) const noexcept { return raw->get_user_ptr(raw, arg); }
  /** @brief Set the C pointer of a `user-ptr` object. @manual{Module-Values.html#index-set_005fuser_005fptr} */
  void set_user_ptr(value arg, void *ptr) const noexcept { return raw->set_user_ptr(raw, arg, ptr); }

#if CPPEMACS_HAVE_NOEXCEPT_TYPEDEFS
  /** @brief Extract the finalizer from a `user-ptr` object. @manual{Module-Values.html#index-get_005fuser_005ffinalizer} */
  emacs_finalizer get_user_finalizer(value uptr) const noexcept { return raw->get_user_finalizer(raw, uptr); }
  /** @brief Set the finalizer of a `user-ptr` object. @manual{Module-Values.html#index-set_005fuser_005ffinalizer} */
  void set_user_finalizer(value uptr, emacs_finalizer fin) const noexcept { return raw->set_user_finalizer(raw, uptr, fin); }
#else
  void (*get_user_finalizer(value uptr) const noexcept)(void*) noexcept { return raw->get_user_finalizer(raw, uptr); }
  void set_user_finalizer(value uptr, void (*fin)(void*) noexcept) const noexcept { return raw->set_user_finalizer(raw, uptr, fin); }
#endif

  /* Vector functions. */
  /** @brief Get the @e index-th element of @e vector. @manual{Module-Values.html#index-vec_005fget} */
  value vec_get(value vector, ptrdiff_t index) const noexcept { return raw->vec_get(raw, vector, index); }
  /** @brief Set the @e index-th element of @e vector to @e value. @manual{Module-Values.html#index-vec_005fset} */
  void vec_set(value vector, ptrdiff_t index, value value) const noexcept { return raw->vec_set(raw, vector, index, value); }
  /** @brief Get the size of @e vector. @manual{Module-Values.html#index-vec_005fsize} */
  ptrdiff_t vec_size(value vector) const noexcept { return raw->vec_size(raw, vector); }

  /**@}*/
#if (EMACS_MAJOR_VERSION >= 26)
  /** @name Emacs 26 */
  /**@{*/
  /** @brief Returns whether a quit is pending. @manual{Module-Misc.html#index-should_005fquit} */
  bool should_quit() const noexcept { return raw->should_quit(raw); }

  /**@}*/
#endif // >= 26
#if (EMACS_MAJOR_VERSION >= 27)
  /** @name Emacs 27 */
  /**@{*/
  /** @brief Processes pending input events and returns whether the module function
   * should quit. @manual{Module-Misc.html#index-process_005finput} */
  process_input_result process_input() const noexcept { return raw->process_input(raw); }

  /** @brief Interpret @e arg as a Lisp time value and convert it to a timespec.
   * @manual{Module-Values.html#index-extract_005ftime} */
  struct timespec extract_time(value arg) const noexcept { return raw->extract_time(raw, arg); }
  /** @brief Convert @e time to a Lisp time value. @manual{Module-Values.html#index-make_005ftime} */
  value make_time(struct timespec time) const noexcept { return raw->make_time(raw, time); }

  /**
   * @brief Extract the arbitrary-precision integer value of @e arg, which must
   * be an integer (fixnum or bignum).
   * @manual{Module-Values.html#index-extract_005fbig_005finteger}
   */
  bool extract_big_integer(value arg, int *sign, ptrdiff_t *count, emacs_limb_t *magnitude) const noexcept
  { return raw->extract_big_integer(raw, arg, sign, count, magnitude); }
  /**
   * @brief Make an arbitrary-precision Lisp integer with the provided @e sign and @e magnitude.
   * @manual{Module-Values.html#index-extract_005fbig_005finteger}
   */
  value make_big_integer(int sign, ptrdiff_t count, const emacs_limb_t *magnitude) const noexcept
  { return raw->make_big_integer(raw, sign, count, magnitude); }

  /**@}*/
#endif // >= 27
#if (EMACS_MAJOR_VERSION >= 28)
  /** @name Emacs 28 */
  /**@{*/

#if CPPEMACS_HAVE_NOEXCEPT_TYPEDEFS
  /** @brief Get the finalizer of a module function. @manual{Module-Functions.html#index-get_005ffunction_005ffinalizer} */
  emacs_finalizer get_function_finalizer(value arg) const noexcept { return raw->get_function_finalizer(raw, arg); }
  /** @brief Set the finalizer of a module function. @manual{Module-Functions.html#index-set_005ffunction_005ffinalizer} */
  void set_function_finalizer(value arg, emacs_finalizer fin) const noexcept { return raw->set_function_finalizer(raw, arg, fin); }
#else
  void (*get_function_finalizer (value arg) const noexcept)(void*) noexcept { return raw->get_function_finalizer(raw, arg); }
  void set_function_finalizer(value arg, void (*fin)(void*) noexcept) const noexcept { return raw->set_function_finalizer(raw, arg, fin); }
#endif

  /** @brief Open a file descriptor to a pipe process from `make-pipe-process`.
   * @manual{Module-Misc.html#index-open_005fchannel}. */
  int open_channel(value pipe_process) const noexcept { return raw->open_channel(raw, pipe_process); }
  /** @brief Make a module function interactive with `interactive` specification @e spec.
   * @manual{Module-Functions.html#index-make_005finteractive} */
  void make_interactive(value function, value spec) const noexcept { return raw->make_interactive(raw, function, spec); }
  /** @brief Make a unibyte Lisp string from binary data.
   * @manual{Module-Values.html#index-make_005funibyte_005fstring}. */
  value make_unibyte_string(const char *str, ptrdiff_t len) const noexcept { return raw->make_unibyte_string(raw, str, len); }

  /**@}*/
#endif // >= 28
#if (EMACS_MAJOR_VERSION >= 29)
  /** @name Emacs 29 */
  /**@{*/

  /**@}*/
#endif

  /**
   * @brief Perform an arbitrary conversion to Emacs.
   *
   * @warning This may thrown an exception, depending on the type, which is not
   * allowed in the top-level emacs_module_init() function. If you want to be
   * safe, use @ref run_catching().
   *
   * @see
   * to_emacs_convertible for what values can be converted.
   *
   * operator->*() for a terser alias.
   */
  template <TO_EMACS_TYPE T> cell inject(T &&arg) const;

  /** @brief Perform an arbitrary conversion to Emacs. Synonym for inject(). */
  template <TO_EMACS_TYPE T> cell operator->*(T &&arg) const;

  /**
   * @brief Perform an arbitrary conversion from Emacs.
   *
   * @warning This throws an exception (via maybe_non_local_exit()) if Emacs is
   * in a non-local exit state, since the return value is typically meaningless.
   * Conversions themselves are also encouraged to throw exceptions for this
   * reason.
   *
   * @see
   * from_emacs_convertible for what values can be converted.
   *
   * cell::extract() which is often more convenient.
   */
  template <FROM_EMACS_TYPE T> T extract(value val) const noexcept(false)
  {
    auto ret = from_emacs(expected_type_t<T>{}, raw, val);
    maybe_non_local_exit();
    return ret;
  }

private:
  // F doesn't throw
  template <typename BoxV, typename F, detail::enable_if_t<noexcept(std::declval<F>()()), bool> = true>
  auto run_catching_internal(BoxV, F &&f) const noexcept -> detail::invoke_result_t<F> {
    return std::forward<F>(f)();
  }

  /* Extracted to a non-generic (over F) function, to reduce code size, and
   * for the (not correctness-bearing) `static bool is_init;`. */
  template <bool Box>
  void run_catching_handle_current(std::integral_constant<bool, Box>) const noexcept {
    if (non_local_exit_check()) return;
    try {
      throw;
    } catch (const signal &s) {
      non_local_exit_signal(s.symbol, s.data);
    } catch (const thrown &s) {
      non_local_exit_throw(s.symbol, s.data);
    } catch (const non_local_exit &) {
      CPPEMACS_MAYBE_IF_CONSTEXPR(Box) {
        throw;
      } else {
        funcall(
          intern("error"),
          {make_string("Expected non-local exit")}
        );
      }
    } catch (const std::exception &err) {
      CPPEMACS_MAYBE_IF_CONSTEXPR(Box) {
        // std::runtime_error is thrown directly as error
        if (typeid(err) != typeid(std::runtime_error)) {
          throw;
        }
      }
      const char *str = err.what();
      funcall(
        intern("error"),
        {make_string(str, std::char_traits<char>::length(str))}
      );
    } catch (...) {
      CPPEMACS_MAYBE_IF_CONSTEXPR(Box) {
        void (*fin)(void *) noexcept = [](void *v) noexcept
        { delete reinterpret_cast<std::exception_ptr*>(v); };
        auto uptr_ptr = new std::exception_ptr(std::current_exception());

        static bool is_init = false;
        value tag = intern("cppemacs--exception");
        if (!is_init) {
          funcall(intern("define-error"), {
              tag,
              make_string("Opaque C++ exception")
            });
          is_init = true;
        }

        value uptr = make_user_ptr(fin, uptr_ptr);
        funcall(intern("signal"), {tag, uptr});
      } else {
        funcall(
          intern("error"),
          {make_string("Unrecognised exception")}
        );
      }
    }
    assert(non_local_exit_check());
  }

  // F might throw
  template <typename BoxV, typename F, detail::enable_if_t<!noexcept(std::declval<F>()()), bool> = true>
  auto run_catching_internal(BoxV, F &&f) const noexcept -> detail::invoke_result_t<F> {
    using return_type = detail::invoke_result_t<F>;
    static_assert(
      std::is_void<return_type>::value ||
      std::is_nothrow_default_constructible<return_type>::value,
      "Return type must be void or nothrow default constructible");

    try {
      return std::forward<F>(f)();
    } catch(...) {
      run_catching_handle_current(BoxV{});
    }
    return return_type();
  }
public:

  /**
   * @brief Run `f()` and catch all exceptions, propagating them to Emacs via
   * `signal` or `throw`.
   *
   * @note This should be used in procedures which have to be
   * `noexcept`, like emacs_module_init() which is directly called
   * from `module-load`. Some forms, like make_module_function() and
   * make_spreader_function() implicitly wrap their function argument
   * with this, so you might not need to worry about this.
   *
   * If `noexcept(f())` is true, then this simply calls the function directly,
   * and no exception handling is done. Otherwise the return type must be void,
   * or default-constructible, so that something can be returned when an
   * exception is thrown.
   *
   * Caught exceptions are converted as follows:
   *
   * - If a non-local exit is pending when the exception is caught, do nothing.
   *
   * - If it is @ref signal or @ref thrown, raise it with @ref
   *   non_local_exit_signal() or @ref non_local_exit_throw(), respectively.
   *
   * - If it is @ref non_local_exit, and `Box` is false, signal an unspecified
   *   `error`.
   *
   * - If it is a `std::exception` and `Box` is false, or if `Box` is true and
   *   it is exactly `std::runtime_error`, signal an `error` with the
   *   exception's `what()` as its message.
   *
   * - If it is anything else:
   *
   *   - If `Box` is true, signal a `cppemacs--exception` with the exception boxed as a
   *     `user-ptr` of the `std::exception_ptr`.
   *
   *   - If `Box` is false, signal an unspecified `error`.
   *
   * In all cases, the @link cppemacs::envw::non_local_exit_check() non-local
   * exit status @endlink will be `signal` or `throw` if an exception was
   * caught.
   *
   * @code
   * envw env = ..;
   * env.run_catching([]() {
   *   throw std::runtime_error("This will be an Emacs error");
   * });
   * @endcode
   *
   * @tparam Box Controls the behavior when catching general exceptions.
   */
  template <bool Box = CPPEMACS_ENABLE_EXCEPTION_BOXING, typename F>
  auto run_catching(F &&f) const noexcept -> detail::invoke_result_t<F> {
    return run_catching_internal(
      std::integral_constant<bool, Box>{},
      std::forward<F>(f));
  }

  /**
   * @brief Call `f` with a new @ref envw.
   *
   * This can be used to permit the garbage collector to collect values used in
   * `f`, even before the caller has to return. `f` may capture @ref value
   * "values" (or indeed, any variables) and use them, but may not propagate any
   * of its own env's @ref value "values" to the caller, since its env will go
   * out of scope once it returns: it can use the enclosing env for values that
   * must be retained.
   *
   * If `f` throws an exception, it is not propagated to Emacs, but caught and
   * passed back to C++ (via `std::exception_ptr`).
   *
   * @param f The function to call. The return value is ignored.
   */
  template <typename F>
#ifdef CPPEMACS_HAVE_CONCEPTS
  requires std::invocable<F, envw>
#endif
  void run_scoped(F &&f) const noexcept(noexcept(std::declval<F>()(std::declval<envw>()))) {
    constexpr bool is_noexcept = noexcept(std::declval<F>()(std::declval<envw>()));
    // otherwise it must be captured in the lambda, for some reason
    using IsNoexcept = std::integral_constant<bool, is_noexcept>;
    using FPtrType = typename std::remove_reference<F>::type *;

    if (non_local_exit_check()) return;

    struct fun_data {
      FPtrType fptr;
      std::exception_ptr exn;
    };
    fun_data fdata{&f, nullptr};
    value func = make_function(1, 1, [](emacs_env *raw_env, ptrdiff_t, value *args, void *data) noexcept {
      envw env = raw_env;
      fun_data &fdata = *reinterpret_cast<fun_data*>(data);
      try {
        std::forward<F>(*fdata.fptr)(env);
      } catch (...) {
        fdata.exn = std::current_exception();
      }
      return args[0];
    }, nullptr, reinterpret_cast<void *>(&fdata));

    funcall(func, 1, &func);
    if (!is_noexcept) {
      if (fdata.exn) {
        std::rethrow_exception(std::move(fdata.exn));
      }

      maybe_non_local_exit();
    }
  }
};

#ifndef CPPEMACS_DOXYGEN_RUNNING
// specializations for is_compatible, these must be defined out of line, GCC complains otherwise
#  if (EMACS_MAJOR_VERSION >= 25)
template <> inline bool envw::is_compatible<25>() const noexcept { return size() >= sizeof(emacs_env_26); }
#  endif
#  if (EMACS_MAJOR_VERSION >= 26)
template <> inline bool envw::is_compatible<26>() const noexcept { return size() >= sizeof(emacs_env_26); }
#  endif
#  if (EMACS_MAJOR_VERSION >= 27)
template <> inline bool envw::is_compatible<27>() const noexcept { return size() >= sizeof(emacs_env_27); }
#  endif
#  if (EMACS_MAJOR_VERSION >= 28)
template <> inline bool envw::is_compatible<28>() const noexcept { return size() >= sizeof(emacs_env_28); }
#  endif
#  if (EMACS_MAJOR_VERSION >= 29)
template <> inline bool envw::is_compatible<29>() const noexcept { return size() >= sizeof(emacs_env_29); }
#  endif
#endif

/**
 * @brief General Emacs value wrapper.
 *
 * A @ref cell is like a simple @ref value, but additionally carries around the
 * @ref envw it is part of. This provides a few benefits:
 *
 * - Cells can be @ref operator()() "called as functions", performing @ref
 *   cppemacs_conversions "type conversions" on the arguments.
 *
 * - @ref operator==() and @ref operator bool() can be used to idiomatically
 *   check (identity) equality and nil-ness.
 *
 * - They can be @ref extract() "extracted" to C++ values easily.
 *
 * @code
 * envw env = ...;
 * value val = ...;
 *
 * // envw::inject()/->* return a cell
 * cell cel = env->*val;
 *
 * // cells can be called and perform conversions
 * cel(1, 2, 3);
 * // code should check for errors
 * env.maybe_non_local_exit();
 *
 * // cells make it easy to call Emacs functions
 * (cel->*"message")("Hello!"_Estr);
 * @endcode
 *
 * @warning Much like @ref envw, most methods on this type do not
 * throw exceptions if Emacs code fails. It is up to the caller to
 * check for non-local exits with @ref envw::non_local_exit_check()
 * or @ref envw::maybe_non_local_exit().
 */
struct cell {
private:
  envw nv;
  value val;

public:
  /** @brief Construct a cell from an environment and value. */
  cell(envw nv, value val) noexcept: nv(nv), val(val) {}

  /** @brief Get the referenced environment. */
  const envw *operator->() const noexcept { return &nv; }
  /** @brief Get the referenced environment. */
  const envw env() const noexcept { return nv; }

  /**
   * @brief Convert a C++ value to a cell. See @ref cppemacs_conversions.
   *
   * @warning This may throw exceptions. See envw::inject().
   */
  template <TO_EMACS_TYPE T> cell operator->*(T &&arg) const
    noexcept(noexcept(nv->*std::forward<T>(arg)))
  { return nv->*std::forward<T>(arg); }

  /** @brief Extract the underlying raw Emacs value. */
  operator value() const noexcept { return val; }
  /** @brief Assign from a raw Emacs value. */
  cell &operator=(value new_val) noexcept { val = new_val; return *this; }

  /**
   * @brief Call this value as a function.
   *
   * This automatically performs @ref cppemacs_conversions
   * "conversions" on the arguments.
   *
   * @warning This throws exceptions if the conversions fail, but not
   * if the function call fails, which may be unintuitive. See @ref
   * envw::non_local_exit_check().
   */
  template <TO_EMACS_TYPE ...Args>
  cell operator()(Args&&...args) const
    noexcept(noexcept(std::initializer_list<value>{nv->*std::forward<Args>(args)...})) {
    value argv[] = {nv->*std::forward<Args>(args)...};
    return call(sizeof...(Args), argv);
  }

  /**
   * @brief Call this value as a function on the given arguments.
   *
   * @warning This returns a meaningless value, rather than throwing
   * an exception, if the function call fails. See @ref
   * envw::non_local_exit_check().
   */
  cell call(ptrdiff_t nargs, value *args) const noexcept {
    return nv->*nv.funcall(val, nargs, args);
  }

  /** @brief Get the type of this cell, via @ref envw::type_of(). */
  cell type() const noexcept { return nv->*nv.type_of(val); }

  /**
   * @brief Get the size of this value as a vector.
   *
   * @warning If this is not a vector, this method returns a
   * meaningless value, rather than throwing an exception. See @ref
   * envw::non_local_exit_check().
   */
  ptrdiff_t vec_size() const noexcept { return nv.vec_size(val); }

  /** @brief Get the `index`th element of this value as a vector. */
  cell vec_get(ptrdiff_t index) const noexcept { return nv->*nv.vec_get(val, index); }
  /** @brief Set the `index`th element of this value as a vector. */
  void vec_set(ptrdiff_t index, value new_value) const noexcept { nv.vec_set(val, index, new_value); }

  /** @brief Return `true` if the value is non-nil. */
  operator bool() const noexcept { return nv.is_not_nil(val); }

  /** @brief Return `true` if this value is `eq` to some other. */
  bool operator==(value o) const noexcept { return nv.eq(val, o); }

  /**
   * @brief Convert this cell to the given C++ type. See @ref cppemacs_conversions.
   *
   * @warning This may throw exceptions. See envw::extract().
   *
   * @code
   * cell x = ...;
   * int x_int = x.extract<int>();
   * double x_double = x.extract<double>();
   * @endcode
   */
  template <FROM_EMACS_TYPE T>
  T extract() const noexcept(false) { return nv.extract<T>(*this); }

  /**
   * @brief Set this cell from the given C++ value. See @ref cppemacs_conversions.
   *
   * @warning This may throw exceptions. See the warning on envw::extract().
   */
  template <TO_EMACS_TYPE T>
  void set(T &&new_val) noexcept(false) { *this = nv->*std::forward<T>(new_val); }
};
CPPEMACS_SUPPRESS_WCOMPAT_MANGLING_END

#ifndef CPPEMACS_DOXYGEN_RUNNING
// the out-of-line definitions confuse doxygen
template <TO_EMACS_TYPE T> cell envw::operator->*(T &&arg) const
{ return cell(*this, to_emacs(expected_type_t<detail::decay_t<T>>{}, raw, std::forward<T>(arg))); }
template <TO_EMACS_TYPE T> cell envw::inject(T &&arg) const
{ return *this->*std::forward<T>(arg); }

inline cell from_emacs(expected_type_t<cell>, envw nv, value x) { return cell(nv, x); }
inline value to_emacs(expected_type_t<cell>, envw, cell x) { return x; }
#endif

/** @} */

/**
 * @addtogroup cppemacs_conversions
 * @{ */
/**
 * @brief Convert an Emacs string to a C++ string.
 */
inline std::string from_emacs(expected_type_t<std::string>, emacs_env *raw_env, value val) noexcept {
  envw nv = raw_env;
  ptrdiff_t len = 0;
  if (nv.copy_string_contents(val, nullptr, len)) {
    std::string ret(len - 1, '\0');
    if (nv.copy_string_contents(val, &ret[0], len)) {
      return ret;
    }
  }
  return "";
}
/**@}*/

}

#endif /* CPPEMACS_EMACS_HPP_ */
