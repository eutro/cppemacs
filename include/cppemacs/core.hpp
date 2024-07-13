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

#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <type_traits>

/**
 * @brief cppemacs is a C++11 API wrapper for writing Emacs @manlink{dynamic modules.,Dynamic-Modules.html}
 *
 * Features include:
 *
 * - A wrapper for the environment struct: @ref cppemacs::envw, which allows
 *   invoking all @manlink{Emacs API functions,Writing-Dynamic-Modules.html} as
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
 * @code
 * #include <cppemacs/all.hpp>
 * using namespace cppemacs;
 *
 * // must be compatible with Emacs' license
 * extern "C" int plugin_is_GPL_compatible;
 *
 * // the entrypoint of the module
 * extern "C" int emacs_module_init(runtime *rt) noexcept {
 *   envw env(rt->get_environment());
 *
 *   // ... define module functions
 *   cell defalias = env->*"defalias";
 *   defalias("example-function", env->*make_spreader_function<0>("example", [](envw env) -> value {
 *     // ... do something useful
 *     return env->*false; // return nil
 *   }));
 *
 *   return 0;
 * }
 * @endcode
 *
 * Here is a longer illustrative example:
 *
 * @snippet hello_world.cpp Full example
 */
namespace cppemacs {

/** @mainpage
 * @copydoc cppemacs
 */

// emacs-module.h only defines it in/after Emacs 27
// (with CMake we could potentially pass it as part of the build?)
#ifndef EMACS_MAJOR_VERSION
#define EMACS_MAJOR_VERSION 25
#endif

namespace detail {
/** @brief C++14 @c std::enable_if_t */
template <bool B, class T = void> using enable_if_t = typename std::enable_if<B, T>::type;
/** @brief C++14 @c std::decay_t */
template <typename T> using decay_t = typename std::decay<T>::type;
/** @brief C++14 @c std::remove_reference_t */
template <typename T> using remove_reference_t = typename std::remove_reference<T>::type;

#if (defined(__cpp_lib_is_invocable) || (__cplusplus >= 201703L)) || defined(CPPEMACS_DOXYGEN_RUNNING)
#define CPPEMACS_HAVE_IS_INVOCABLE 1
/** @brief C++17 @c std::invoke_result_t approximation. */
template <typename T, typename ...Args> using invoke_result_t = std::invoke_result_t<T, Args...>;
#else
template <typename T, typename ...Args> using invoke_result_t =
  decltype(std::declval<detail::decay_t<T>>()(std::declval<Args>()...));
#endif
};

/**
 * @defgroup cppemacs_core Core Types
 * @brief Core wrapper type definitions for the Emacs module API.
 *
 * @addtogroup cppemacs_core
 * @{
 */

// aliases to core types
/** @brief Raw Emacs environment. You should use @ref envw in most cases. */
using emacs_env = ::emacs_env;

/** @brief Alias to the type in `emacs-module.h`. */
using emacs_value = ::emacs_value;
/** @brief Raw opaque pointer representing an Emacs value. See @ref cell for the wrapper. */
using value = emacs_value;

/** @brief Struct passed to module init function.
 * @manual{emacs_module_init(),Module-Initialization.html#index-emacs_005fmodule_005finit-1} */
using emacs_runtime = ::emacs_runtime;

/**
 * @brief Memberless subtype of @ref cppemacs::emacs_runtime "emacs_runtime".
 *
 * Can be used directly in the module init function:
 *
 * @code
 * using namespace cppemacs;
 * extern "C" int emacs_module_init(runtime *rt) noexcept {
 *   if (rt->size < sizeof(*rt)) return 1;
 *   envw env(rt->get_environment());
 *   // ...
 * }
 * @endcode
 */
struct runtime : emacs_runtime {
  /** @brief Get the environment. @manual{Module-Initialization.html#index-emacs_005fmodule_005finit-1} */
  emacs_env *get_environment() noexcept { return emacs_runtime::get_environment(static_cast<emacs_runtime*>(this)); }
};

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
/** @brief A raw module function, for envw::make_function().
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

/** @} */
/**
 * @addtogroup cppemacs_conversions
 * @{
 */

/**
 * @brief Marker type for converting between Emacs and C++ values.
 *
 * When converting from C++ to Emacs, using @ref to_emacs(), this type denotes the
 * expected argument type. This makes overload resolution easier to reason about,
 * since implicit conversions will not be performed unless the overload permits it.
 *
 * When converting from Emacs to C++, using @ref from_emacs(), this type denotes the
 * expected return type of the overload, allowing the overload to be resolved
 * without explicit template parameters.
 *
 * In both cases, ADL is used, allowing the functions to be looked up in the
 * namespace of @p T.
 */
template <typename T> struct expected_type_t {};

/* type conversion concepts, we can declare these now so we can use
   them in `envw` */
#if defined(__cpp_concepts) || defined(CPPEMACS_DOXYGEN_RUNNING)
/** @brief Defined if concepts are available. */
#  define CPPEMACS_HAVE_CONCEPTS 1

/**
 * @brief Specifies that values of type T can be converted to Emacs values.
 *
 * Any type that has an overload for `to_emacs(expected_type_t<decay_t<T>>,
 * emacs_env *env, T &&t)` using ADL, with a return type convertible to @ref
 * cppemacs::value "value", satisfies this concept. There are no other
 * restrictions. In particular, such functions may throw exceptions (and are
 * encouraged to).
 *
 * This can portably be used in template parameters with @ref TO_EMACS_TYPE,
 * which is provided even before C++20.
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
#define TO_EMACS_TYPE to_emacs_convertible

/**
 * @brief Specifies that Emacs values can be converted to values of
 * type T.
 *
 * Any type that has an overload for `from_emacs(expected_type_t<T>, emacs_env
 * *env, value x)` using ADL, with a return type convertible to `T` satisfies
 * this concept. There are no other restrictions. In particular, such functions
 * may throw exceptions, though those with complicated GC hand-off might try not
 * to.
 *
 * This can portably be used in template parameters with @ref FROM_EMACS_TYPE,
 * which is provided even before C++20.
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
#define FROM_EMACS_TYPE from_emacs_convertible

#else
#define TO_EMACS_TYPE typename
#define FROM_EMACS_TYPE typename
#endif

#ifndef CPPEMACS_DOXYGEN_RUNNING
inline value from_emacs(expected_type_t<value>, emacs_env *, value x) { return x; }
inline value to_emacs(expected_type_t<value>, emacs_env *, value x) { return x; }
#endif

/** @} */
/** @addtogroup cppemacs_core
 * @{ */

/** @brief Throw an error indicating that Emacs version N is required. */
#define CPPEMACS_VERSION_FAIL(N) throw std::runtime_error("Emacs " #N "+ required");

/**
 * @brief Return whether the environment supports Emacs major version
 * N.
 *
 * This compares the size field of the environment to the size of
 * emacs_env_N, so this does not support Emacs versions greater than
 * the one we were compiled against.
 */
#define CPPEMACS_HAS_VERSION(env, N) ((env)->size < sizeof(emacs_env_##N))
/**
 * @brief Check if the environment supports Emacs major version N, and
 * throw an exception if it doesn't.
 *
 * @see CPPEMACS_HAS_VERSION
 */
#define CPPEMACS_CHECK_VERSION(env, N)                                  \
  do { if (CPPEMACS_HAS_VERSION(env, N)) CPPEMACS_VERSION_FAIL(N); } while(0) \

/**
 * @brief General Emacs environment wrapper.
 *
 * This type wraps the raw @ref emacs_env, providing additional funcionality:
 *
 * - Member functions for invoking all function pointers in @ref emacs_env,
 *   without having to use @c env->function_name(env, ...).
 *
 * - Conversions @ref inject() "from C++ to Emacs" and @ref extract()
 *   "vice-versa". Most Lisp value creation/inspection can be done this
 *   way. See @ref cppemacs_conversions.
 *
 * - Idiomatic non-local exit handling, via run_catching(),
 *   maybe_non_local_exit(), and rethrow_non_local_exit().
 *
 * @important An environment and most @ref value "values" obtained from it are
 * invalidated when the function it was first obtained in returns (either
 * @manlink{emacs_module_init(),Module-Initialization.html#index-emacs_005fmodule_005finit-1}
 * or a @ref make_function() "module function" to which it is a parameter).
 * Emacs environment functions cannot be used outside of the dynamic extent of
 * these functions and @ref value "values" (except for @ref make_global_ref()),
 * cannot be shared between invocations either.
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
   * @brief Check if there is a non-local exit
   * pending. @manual{Module-Nonlocal.html#index-non_005flocal_005fexit_005fcheck}
   *
   * @warning If this returns anything other than @ref funcall_exit::return_,
   * then most other methods return meaningless values, unless specified
   * otherwise.
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
   * @brief Check for non-local exit, and throw @ref non_local_exit if there is one.
   *
   * This allows using C++ stack unwinding to return control to Emacs
   * naturally.
   *
   * This method does @b not clear the pending non-local exit, and
   * does not capture the type (signal/throw) or its data. For that,
   * use @ref rethrow_non_local_exit() instead.
   */
  void maybe_non_local_exit() const noexcept(false) {
    if (non_local_exit_check())
      throw non_local_exit{};
  }

  /**
   * @brief Check for non-local exit, and throw it as a C++ exception
   * (@ref signal or @ref thrown) if there is one.
   *
   * Unlike @ref maybe_non_local_exit(), this method @b does clear the
   * pending non-local exit, and does capture the type and its data.
   *
   * @throws signal If a signal is pending.
   * @throws thrown If a throw is pending.
   */
  void rethrow_non_local_exit() const noexcept(false) {
    value symbol, data;
    funcall_exit kind = non_local_exit_get(symbol, data);
    if (!kind) return;
    non_local_exit_clear();
    if (kind == funcall_exit::signal_) {
      throw signal(symbol, data);
    } else {
      throw thrown(symbol, data);
    }
  }

  /* Function registration. */

  /** @brief Make an Emacs <i>module function</i> from a C++ function.
   * @manual{Module-Functions.html#index-make_005ffunction}
   *
   * @see make_module_function() and make_spreader_function() for convenience
   * functions that produce @ref emacs_function "emacs_functions" from C++
   * closures.
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
  /**
   * @brief Perform an arbitrary conversion to Emacs.
   *
   * @see to_emacs_convertible for what values can be converted.
   *
   * @see operator->*() for a terser alias.
   */
  template <TO_EMACS_TYPE T> cell inject(T &&arg) const;

  /** @brief Perform an arbitrary conversion to Emacs. Synonym for inject(). */
  template <TO_EMACS_TYPE T> cell operator->*(T &&arg) const;

  /**
   * @brief Perform an arbitrary conversion from Emacs.
   *
   * @see from_emacs_convertible for what values can be converted.
   */
  template <FROM_EMACS_TYPE T> T extract(value val) const noexcept(false)
  {
    auto ret = from_emacs(expected_type_t<T>{}, raw, val);
    maybe_non_local_exit();
    return ret;
  }

  /** @brief Run @p f and catch all exceptions, propagating them to Emacs. */
  template <typename F>
  auto run_catching(F f) const noexcept -> detail::invoke_result_t<F> {
    using return_type = detail::invoke_result_t<F>;
    static_assert(
      std::is_void<return_type>::value ||
      std::is_nothrow_default_constructible<return_type>::value,
      "Return type must be void or nothrow default constructible");
    try {
      return f();
    } catch (const signal &s) {
      if (!non_local_exit_check())
        non_local_exit_signal(s.symbol, s.data);
    } catch (const thrown &s) {
      if (!non_local_exit_check())
        non_local_exit_throw(s.symbol, s.data);
    } catch (const non_local_exit &) {
      if (!non_local_exit_check()) {
        funcall(
          intern("error"),
          {make_string("Expected non-local exit")}
        );
      }
    } catch (std::runtime_error &err) {
      non_local_exit_clear();
      const char *str = err.what();
      funcall(
        intern("error"),
        {make_string(str, std::char_traits<char>::length(str))}
      );
    } catch (...) {
      non_local_exit_clear();
      funcall(
        intern("error"),
        {make_string("Unrecognised exception")}
      );
    }
    assert(non_local_exit_check());
    return return_type();
  }
};

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
 */
struct cell {
private:
  envw nv;
  value val;

public:
  /** @brief Construct a cell from an environment and value. */
  cell(envw nv, value val) noexcept: nv(nv), val(val) {}

  /** @brief Get the referenced environment. */
  const envw *operator->() const { return &nv; }
  /** @brief Convert a C++ value to a cell. See @ref cppemacs_conversions. */
  template <TO_EMACS_TYPE T> cell operator->*(T &&arg) const
  { return nv->*std::forward<T>(arg); }

  /** @brief Extract the underlying raw Emacs value. */
  operator value() const noexcept { return val; }
  /** @brief Assign from a raw Emacs value. */
  cell &operator=(value new_val) noexcept { val = new_val; return *this; }

  /** @brief Call this value as a function.
   *
   * This automatically performs @ref cppemacs_conversions
   * "conversions" on the arguments.
   */
  template <TO_EMACS_TYPE ...Args>
  cell operator()(Args&&...args) const noexcept(false)
  {
    value res = nv.funcall(val, {nv->*std::forward<Args>(args)...});
    return nv->*res;
  }

  /** @brief Get the type of this cell, via @ref envw::type_of(). */
  cell type() const {
    nv.maybe_non_local_exit();
    return nv->*nv.type_of(val);
  }

  /** @brief Return `true` if the value is non-nil. */
  operator bool() const {
    nv.maybe_non_local_exit();
    return nv.is_not_nil(val);
  }

  /** @brief Return `true` if this value is `eq` to some other. */
  bool operator==(value o) const {
    nv.maybe_non_local_exit();
    return nv.eq(val, o);
  }

  /** @brief Convert this cell to the given C++ type. See @ref cppemacs_conversions. */
  template <FROM_EMACS_TYPE T>
  T extract() const noexcept(false) { return nv.extract<T>(*this); }

  /** @brief Set this cell from the given C++ value. See @ref cppemacs_conversions. */
  template <TO_EMACS_TYPE T>
  void set(T &&new_val) { *this = nv->*std::forward<T>(new_val); }
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

}

#endif /* CPPEMACS_EMACS_HPP_ */
