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

#ifndef CPPEMACS_WRAPPERS_HPP_
#define CPPEMACS_WRAPPERS_HPP_

#include "core.hpp"
#include "conversions.hpp"
#include <memory>
#include <type_traits>
#include <utility>

#ifdef __GNUC__
#include <cxxabi.h>
#endif

namespace cppemacs {
namespace detail {
#ifdef __GNUC__
inline std::string demangle(const char *name) {
  int status;
  std::unique_ptr<char[], decltype(&std::free)> s(
    abi::__cxa_demangle(name, 0, 0, &status),
    &std::free);
  return std::string(s ? s.get() : name);
}
#else
inline std::string demangle(std::string &&s) { return s; }
#endif

template <typename T>
inline std::string type_name() { return demangle(typeid(T).name()); }
}
}

/**
 * @defgroup cppemacs_utilities Utilities
 * @brief Utility types built on top of the core cppemacs API.
 *
 * @addtogroup cppemacs_utilities
 * @{
 */
namespace cppemacs {

/** @brief Type-safe Emacs user pointer representation. */
template <typename T, typename Deleter = std::default_delete<T>>
struct user_ptr {
  static_assert(!std::is_reference<T>::value, "Must not be a reference type");
  static_assert(std::is_default_constructible<Deleter>::value, "Deleter must be default-constructible");
private:
  T *ptr;

public:
  /** @brief Construct from a pointer. */
  user_ptr(T *ptr) noexcept: ptr(ptr) {}

  /** @brief Get the underlying pointer. */
  T *get() const noexcept { return ptr; }
  /** @brief Get the underlying pointer. */
  T *operator->() const noexcept { return ptr; }
  /** @brief Dereference the pointer. */
  T &operator*() const noexcept { return *ptr; }

  /**
   * @brief The finalizer for this pointer.
   *
   * This is used both for cleaning up the pointer once it is declared
   * unreachable by the GC, and for type-checking the pointer when
   * extracting (by comparing the function pointers).
   */
  static void fin(void *ptr) noexcept {
    try {
      Deleter()(reinterpret_cast<T*>(ptr));
    } catch (...) {
      // TODO: log warnings?
    }
  }

  /**
   * @brief Convert a user_ptr to Emacs, with the GC becoming responsible for the object.
   *
   * This function is safe against @ref
   * envw::non_local_exit_check() "pending non-local exits", in that
   * if the value cannot successfully be passed to Emacs' garbage
   * collector, it will be deallocated immediately.
   *
   * @pre The pointer must be owned by us: it has not been
   * converted before, nor was it obtained from an existing @ref
   * value.
   */
  friend value to_emacs(expected_type_t<user_ptr>, envw nv, const user_ptr &ptr) noexcept {
    value ret;
    void *raw = reinterpret_cast<void*>(ptr.ptr);
    if (nv.non_local_exit_check()
        || (ret = nv.make_user_ptr(user_ptr<T, Deleter>::fin, raw),
            nv.non_local_exit_check())) {
      fin(raw);
    }
    return ret;
  }

  /**
   * @brief Convert a user_ptr from Emacs, with the GC still responsible for the object.
   *
   * This function type-checks the argument by comparing val's
   * registered finalizer to @ref fin().
   *
   * @post The resulting user_ptr is not owned by us: it must not be
   * deleted, nor converted back to an Emacs @ref value.
   */
  friend user_ptr from_emacs(expected_type_t<user_ptr>, envw nv, value val) {
    emacs_finalizer fin = nv.get_user_finalizer(val);
    nv.maybe_non_local_exit();
    if (fin != user_ptr<T, Deleter>::fin) {
      throw std::runtime_error("User ptr type mismatch");
    }
    return user_ptr<T, Deleter>(reinterpret_cast<T*>(nv.get_user_ptr(val)));
  }
};

/**
 * @brief In-place construct a @ref user_ptr on the heap.
 *
 * @warning If this value isn't passed to Emacs via @ref
 * envw::inject(), then it will leak memory. The suggested idiom is:
 * @code
 * env.maybe_non_local_exit();
 * env->*make_user_ptr<T>(a, b, c);
 * @endcode
 * which will ensure that no allocation is made if there is already a
 * non-local exit pending.
 */
template <typename T, typename...Args> inline user_ptr<T>
make_user_ptr(Args &&...args) { return user_ptr<T>(new T(std::forward<Args>(args)...)); }

CPPEMACS_SUPPRESS_WCOMPAT_MANGLING_BEGIN
/**
 * @brief Data representation for storing C++ functions in Emacs
 * closures.
 *
 * Emacs lets us store one pointer worth of data along with a module function.
 *
 * The default implementation puts it in an off-heap pointer and
 * attaches a finalizer. There is a specialization that puts F
 * directly as the function data instead (if it fits).
 */
template <typename F, typename = void>
struct module_function_repr {
  /** @brief Get a reference to F from @e ptr. */
  static F &extract(void *ptr) noexcept { return *reinterpret_cast<F *>(ptr); }
  /** @brief Construct the Emacs function from F. */
  static value make(
    envw nv, ptrdiff_t min_arity, ptrdiff_t max_arity,
    value (*fun)(emacs_env*, ptrdiff_t, value*, void*) noexcept,
    const char *doc, F &&f
  ) noexcept(std::is_nothrow_move_constructible<F>::value) {
    if (nv.non_local_exit_check()) return nullptr;

    // The strategy here is to bind the module function to a new
    // symbol, which then has a finalizer attached. We own the
    // instance of F precisely until the finalizer is created,
    // at which point it is owned by the GC.

    static_assert(std::is_move_constructible<F>::value, "Type must be move constructible");
    std::unique_ptr<F> fptr(new F(std::move(f)));
    void *fptr_data = reinterpret_cast<void *>(fptr.get());
    void (*fin)(void*) noexcept = user_ptr<F>::fin;

    value retfn = nv.make_function(
      min_arity, max_arity, fun, doc,
      fptr_data); // fptr still owned by us for a bit

#if (EMACS_MAJOR_VERSION >= 28) // we have module finalizers, maybe
    if (nv.is_compatible<28>()) {
      // we have function finalizers, just attach it
      nv.set_function_finalizer(retfn, fin);
      if (nv.non_local_exit_check()) return nullptr;
      fptr.release(); // now managed by the GC
      return retfn;
    }
#endif
    value make_symbol = nv.intern("make-symbol");
    // bind the module function to a new symbol, for which we can add a finalizer
    value func_sym = (nv->*make_symbol)(nv.make_string("cpp--finalized-fun"));
    (nv->*nv.intern("defalias"))(func_sym, retfn);

    // make a user pointer as a finalizer
    value finalizer = nv.make_user_ptr(fin, fptr_data);
    if (nv.non_local_exit_check()) return nullptr;
    fptr.release(); // the finalizer was created successfully, F is now managed by the GC

    // attach the finalizer, so that the lifetimes of `retfn` and the finalizer are tied
    // (unless users do something stupid!)
    nv.funcall(nv.intern("put"), {func_sym, (nv->*"cpp--data-ptr"), finalizer});
    if (nv.non_local_exit_check()) return nullptr; // make sure that we don't return if the lifetimes weren't tied

    return func_sym; // an alias to our module function in a fresh uninterned symbol
  }
};

namespace detail {
/** @brief Whether T can be stored directly in a void pointer for the
 * typical C `Ret foo(Args...args, void *data)` closure pattern. */
template <typename T>
struct can_stuff_into_void_ptr : std::integral_constant<bool, (
  sizeof(T) <= sizeof(void*)
  && std::is_trivially_copyable<T>::value
  && std::is_trivially_destructible<T>::value
)> {};
}

#ifndef CPPEMACS_DOXYGEN_RUNNING // the type is ridiculously long
/**
 * Function representation, for storing C++ functions in Emacs
 * closures.
 *
 * Emacs lets us store one pointer worth of data. This specialization
 * simply stores the bits of F in that pointer, provided that it fits,
 * and provided that the type is trivially destructible and copyable.
 */
template <typename F>
struct module_function_repr<
  F,
  detail::enable_if_t<detail::can_stuff_into_void_ptr<F>::value>
  >
{
  static F &extract(void *&ptr) noexcept { return reinterpret_cast<F &>(ptr); }
  static value make(
    envw nv, ptrdiff_t min_arity, ptrdiff_t max_arity,
    value (*fun)(emacs_env*, ptrdiff_t, value*, void*) noexcept,
    const char *doc, F &&f) {
    return nv.make_function(
      min_arity, max_arity, fun, doc,
      reinterpret_cast<void *const &>(f));
  }
};
#endif

CPPEMACS_SUPPRESS_WCOMPAT_MANGLING_END

/**
 * @brief A wrapper over `F` that allows it to be @ref
 * envw::operator->*() "converted" to an Emacs function.
 *
 * A reference to `F` must be invocable with <code>(::emacs_env *env, ptrdiff_t
 * nargs, ::value *args)</code>, returning a <code>::value</code>. This differs
 * from a simple @ref emacs_function, in that the `data` parameter is converted
 * to `F` according to the @ref data_repr, instead of being passed to `F`.
 *
 * `F` will be wrapped in @ref envw::run_catching(), and so is free to
 * throw exceptions.
 *
 * @see make_module_function() and make_spreader_function() for creating
 * instances.
 */
template <typename F>
struct module_function {
  static_assert(!std::is_reference<F>::value, "Must not be a reference type");
#ifdef CPPEMACS_HAVE_IS_INVOCABLE
  static_assert(std::is_invocable_r_v<value, F&, emacs_env *, ptrdiff_t, value *>,
                "F must be invocable with Emacs function arguments");
#endif

  /** @brief The representation used to convert between the `data`
   * pointer and the stored instance of `F`.
   *
   * Unless @ref module_function_repr is (further) specialized by the
   * user, the behaviour is as follows:
   *
   * - If `F` is trivially copyable and destructible, and it is
   *   smaller than `void*`, it is directly stored in `data`.
   *
   * - Otherwise, `F` is move-constructed into a heap-allocated
   *   pointer, and a finalizer is registered for the Emacs function
   *   which default-deletes it, silently ignoring any exceptions in
   *   the finalizer.
   */
  using data_repr = module_function_repr<F>;

  /** @brief The minimum number of arguments that `F` is to be called with. */
  ptrdiff_t min_arity;
  /** @brief The maximum number of arguments that `F` is to be called with. */
  ptrdiff_t max_arity;
  /** @brief The documentation string for the Emacs function that will
   * be created. */
  const char *doc;
  /** @brief The invokable function. */
  F f;

  /**
   * @brief Construct with the provided arities, doc-string and
   * in-place arguments for `F`.
   *
   * @see make_module_function() which infers `F`.
   */
  template <typename...Args>
  module_function(
    ptrdiff_t min_arity, ptrdiff_t max_arity,
    const char *doc,
    Args &&...args):
    min_arity(min_arity),
    max_arity(max_arity),
    doc(doc),
    f(std::forward<Args>(args)...)
  {}

  /** @brief An @ref cppemacs::emacs_function "emacs_function" which
   * converts @e data to `F` and invokes it. */
  static value invoke(emacs_env *nv, ptrdiff_t nargs, value *args, void *data) noexcept {
    return envw(nv).run_catching(
      [&]() noexcept(
        noexcept(value(data_repr::extract(data)(nv, nargs, args)))
      ) -> value {
        return data_repr::extract(data)(nv, nargs, args);
      });
  }

  /**
   * @brief Converts this to an Emacs module function.
   *
   * @see envw::make_function() for the underlying conversion.
   */
  friend value to_emacs(expected_type_t<module_function>, envw nv, module_function &&func) {
    return data_repr::make(
      nv, func.min_arity, func.max_arity, &invoke, func.doc,
      std::move(func.f));
  }
};

#if (defined(__cpp_return_type_deduction) || (__cplusplus >= 201304L)) || defined(CPPEMACS_DOXYGEN_RUNNING)
// opaque return types for documentation and C++14
#define CPPEMACS_HAVE_RETURN_TYPE_DEDUCTION 1
#else
#define CPPEMACS_HAVE_RETURN_TYPE_DEDUCTION 0
#endif

/** @brief Make a module_function with the given minimum and maximum arity.
 *
 * @param min_arity The minimum number of arguments the function can be
 * invoked with.
 *
 * @param max_arity
 * The maximum number of arguments the function can be
 * invoked with. <br> If this is @ref emacs_variadic_function, the function can be called
 * with any number of arguments `>= min_arity`.
 *
 * @param doc The documentation string of the
 * function. @manual{Function-Documentation.html} In particular, a
 * <code>(fn @e arglist)</code> line at the end is helpful to provide
 * argument names, since they cannot be inferred from the source code.
 *
 * @param f The implementation of the function, which will be called with a new
 * environment and the arguments. It must be invocable with
 * <br><code>(::emacs_env *env, ptrdiff_t nargs, ::value *args)</code> for
 * `min_arity <= nargs <= max_arity`, and return a <code>::value</code>. It will
 * be wrapped with envw::run_catching().
 *
 * @returns A <code>@ref "module_function"<F></code>, suitable for @ref
 * cppemacs_conversions "conversion" to an Emacs function.
 *
 * @see make_spreader_function() which automatically unpacks the arguments.
 */
template <typename F> auto make_module_function(
  ptrdiff_t min_arity, ptrdiff_t max_arity,
  const char *doc, F &&f
)
#if !CPPEMACS_HAVE_RETURN_TYPE_DEDUCTION
  -> module_function<detail::remove_reference_t<F>>
#endif
{
  return module_function<detail::remove_reference_t<F>>(
    min_arity, max_arity, doc,
    std::forward<F>(f));
}

/**
 * @brief Used to provide a span-like argument to a @link
 * make_spreader_function() spreader function @endlink.
 *
 * This is just an pointer to arguments and a length. This can implicitly be
 * converted via a <code>(::value *begin, ::value *end)</code> constructor, such as
 * `std::vector`. In C++20, this is also compatible with the <a
 * href="https://en.cppreference.com/w/cpp/ranges">ranges library</a>, and can
 * be converted to a `std::span<value>`.
 *
 * @snippet utils_examples.cpp VA Spreader Functions
 */
struct spreader_restargs {
private:
  value *args; size_t nargs;

public:
  /** @brief Construct from a pointer and length. */
  spreader_restargs(value *args, size_t nargs);

  /** @brief Convert this to `T` with a <code>(::value *begin, ::value
      *end)</code> constructor. */
  template <typename T, detail::enable_if_t
            <std::is_constructible<T, value *, value *>::value, int> = 0>
  operator T() const { return {begin(), end()}; }

  /** @brief Get the argument pointer. */
  value *data() const { return args; }
  /** @brief Get the argument pointer. */
  value *begin() const { return args; }
  /** @brief Get a pointer one past the arguments. */
  value *end() const { return args + nargs; }
  /** @brief Get the number of aguments. */
  size_t size() const { return nargs; }
  /** @brief Get the nth argument. */
  value &operator[](ptrdiff_t n) const { return args[n]; }
};

namespace detail {
/** @brief Used to provide a default-constructed argument to a function. */
struct unprovided_value_t {
  /** @brief Return a default-constructed T. */
  template <typename T> operator T() const {
    static_assert(std::is_default_constructible<T>::value, "Optional parameter must be default constructible");
    return T();
  }
};

template <bool IsProvided>
inline typename std::conditional<IsProvided, cell, unprovided_value_t>::type
arg_or_default(envw nv, value *args, size_t idx) noexcept;
template <> inline cell arg_or_default<true>(envw nv, value *args, size_t idx)
  noexcept { return nv->*args[idx]; }
template <> inline unprovided_value_t arg_or_default<false>(envw, value *, size_t)
  noexcept { return {}; }

template <bool IsVar, typename SI> struct spreader_caller;
template <typename SI> struct spreader_caller<false, SI> {
  template <size_t NArgs, size_t...Idx> static value
  invoke_with_arity(detail::index_sequence<Idx...>, SI &si, envw nv, value *args) noexcept(false)
  { return si.asserted_call(nv, arg_or_default<Idx < NArgs>(nv, args, Idx)...); }
  template <size_t CallArity, size_t...Idx> static value
  invoke_variadic(detail::index_sequence<Idx...>, SI &, envw, ptrdiff_t, value *) noexcept(false) { return {}; }
};
template <typename SI> struct spreader_caller<true, SI> {
  template <size_t NArgs, size_t...Idx> static value
  invoke_with_arity(detail::index_sequence<Idx...>, SI &si, envw nv, value *args) noexcept(false) {
    return si.asserted_call(nv, arg_or_default<Idx < NArgs>(nv, args, Idx)...,
                            spreader_restargs(nullptr, 0));
  }
  template <size_t CallArity, size_t...Idx> static value
  invoke_variadic(detail::index_sequence<Idx...>, SI &si, envw nv, ptrdiff_t nargs, value *args) noexcept(false) {
    return si.asserted_call(nv, arg_or_default<true>(nv, args, Idx)...,
                            spreader_restargs(args + CallArity, nargs - CallArity));
  }
};

/**
 * @brief A wrapper over F that adapts it to an @ref module_function style callable.
 *
 * @see make_spread_invoker()
 */
template <ptrdiff_t MinArity, ptrdiff_t MaxArity, bool IsVariadic, typename F>
struct spread_invoker {
  /** @brief The underlying function. */
  F f;
  /** @brief Construct F from a forwarded argument. */
  template <typename Arg>
  spread_invoker(Arg &&arg): f(std::forward<Arg>(arg)) {}

private:
  using arity_seq = detail::make_index_sequence<MaxArity>;
  using caller = spreader_caller<IsVariadic, spread_invoker>;
  friend caller;

  template <typename ...Args>
  value asserted_call(envw nv, Args&&...args)
  {
    auto ret = f(nv, std::forward<Args>(args)...);
    return nv->*ret;
  }

  template <size_t...OffIdx>
  value invoke(
    detail::index_sequence<OffIdx...>,
    envw nv, ptrdiff_t nargs, value *args
  ) noexcept(false) {
    if (IsVariadic && nargs > MaxArity) {
      return caller::template invoke_variadic<MaxArity>(arity_seq(), *this, nv, nargs, args);
    } else if (nargs < MinArity || nargs > MaxArity) {
      // should have been caught by Emacs
      throw std::runtime_error("Bad arity");
    } else {
      static constexpr value (*Vt[])(spread_invoker *, envw, value*) = {
        ([](spread_invoker *self, envw nv, value *args)
        { return caller::template invoke_with_arity<MinArity + OffIdx>(arity_seq(), *self, nv, args); })
        ...
      };
      return Vt[nargs](this, nv, args);
    }
  }

public:
  /** @brief Call the underlying function with the given arguments. */
  value operator()(envw nv, ptrdiff_t nargs, value *args) noexcept {
    return nv.run_catching([&]() -> value {
      return invoke(
        detail::make_index_sequence<MaxArity - MinArity + 1>(),
        nv, nargs, args);
    });
  }
};
}

/**
 * @brief An encoded arity for @ref make_spreader_function().
 *
 * @see spreader_thunk, spreader_pred, spreader_variadic.
 */
template <ptrdiff_t MinArity, ptrdiff_t MaxArity = MinArity, bool IsVariadic = false>
struct spreader_arity {
  /** @brief The minimum number of arguments to the function. */
  static constexpr ptrdiff_t min_arity = MinArity;
  /** @brief The number of arguments that the spreader function receives. */
  static constexpr ptrdiff_t max_arity = MaxArity;
  /** @brief Whether an arbitrary number of arguments are accepted. */
  static constexpr bool is_variadic = IsVariadic;
};
/** @brief Arity for a function with no arguments. */
using spreader_thunk = spreader_arity<0>;
/** @brief Arity for a function with `MinArity` or more arguments,
 * where the C++ function takes exactly `CallArity` arguments. */
template <ptrdiff_t MinArity, ptrdiff_t CallArity = MinArity>
using spreader_variadic = spreader_arity<MinArity, CallArity, true>;

/**
 * @brief Make a spreader module function with the given minimum and maximum arity.
 *
 * Returns a module_function that can be called with at least `MinArity`
 * arguments. Arguments are passed as @ref cell "cells", and the return type is
 * @link cppemacs_conversions converted @endlink.
 *
 * @warning The function must return a @link cppemacs_conversions to-Emacs
 * convertible @endlink type (not void). Despite my best efforts, this can
 * generate incomprehensible errors, particularly if used without C++20
 * concepts.
 *
 * With `IsVariadic` false, `f` will be invoked with ::emacs_env followed by
 * exactly `MaxArity` arguments: <br><code>(::emacs_env *env, A_1 arg_1, ...,
 * A_MaxArity arg_MaxArity)</code>. Arguments that are provided by the caller
 * will be given as @ref cell "cells", whereas absent arguments will be
 * default-initialized. On the Emacs side, the function cannot be invoked with
 * more than `MaxArity` arguments.
 *
 * With `IsVariadic` true, the resulting function can be called with more than
 * `MaxArity` arguments, and `f` is additionally invoked with a
 * <code>@ref spreader_restargs</code> argument containing the remaining
 * arguments: <br><code>(::emacs_env *env, A_1 arg_1, ..., A_MaxArity
 * arg_MaxArity, @ref spreader_restargs rest)</code>.
 *
 * @see cell_extracted, which automatically converts function arguments to the
 * desired type.
 *
 * @b Examples
 *
 * @snippet utils_examples.cpp Spreader Functions
 */
template <ptrdiff_t MinArity, ptrdiff_t MaxArity, bool IsVariadic, typename F>
auto make_spreader_function(
  spreader_arity<MinArity, MaxArity, IsVariadic> ar,
  const char *doc, F &&f
)
#if !CPPEMACS_HAVE_RETURN_TYPE_DEDUCTION
  -> module_function<detail::spread_invoker<MinArity, MaxArity, IsVariadic, detail::remove_reference_t<F>>>
#endif
{
  constexpr bool is_variadic = decltype(ar)::is_variadic;
  static_assert(is_variadic || MaxArity >= MinArity, "MaxArity must be greater than or equal to MinArity");
  static_assert(MinArity >= 0, "MinArity must be nonnegative");
  return make_module_function(
    MinArity, MaxArity, doc,
    detail::spread_invoker<MinArity, MaxArity, is_variadic,
    detail::remove_reference_t<F>>(std::forward<F>(f)));
}

/** @brief Output a cell to an output stream, using `(format "%s" v)`. */
inline std::ostream &operator<<(std::ostream &os, const cell &v) {
  return os << (v->*"format")(v->make_string("%s", 2), v).extract<std::string>();
}

/**
 * @brief An instance of `ValueType` that can be implicitly constructed from a
 * @ref cell.
 *
 * This is useful for e.g. make_spreader_function() where cells are passed
 * directly, since @ref cell (deliberately) does not apply conversions
 * implicitly. It is particularly useful for functions with optional arguments,
 * which are default-constructed by make_spreader_function().
 *
 * @snippet utils_examples.cpp Cell Extracted
 */
template <FROM_EMACS_TYPE FromEmacsType, typename ValueType = FromEmacsType>
struct cell_extracted {
private:
  ValueType val;
public:
  /** @brief Construct `ValueType` by extracting a `FromEmacsType` from a @ref cell. */
  cell_extracted(cell val) noexcept(noexcept(ValueType(val.extract<FromEmacsType>()))) :
    val(val.extract<FromEmacsType>()) {}
  /** @brief Construct `ValueType` by forwarding to its constructor. */
  template <typename...Args>
  constexpr cell_extracted(Args&&...args) : val(std::forward<Args>(args)...) {}

  /** @brief Returns the held `ValueType`. */
  operator ValueType() { return val; }
  /** @brief Returns a reference to the held `ValueType`. */
  ValueType &get() noexcept { return val; }
  /** @brief Returns a pointer to the held `ValueType`. */
  ValueType *operator->() noexcept { return &val; }
  /** @brief Returns a reference to the held `ValueType`. */
  ValueType &operator*() noexcept { return val; }

  /** @brief Extract the `FromEmacsType` from `val`. */
  friend cell_extracted from_emacs(expected_type_t<cell_extracted>, envw env, value val)
    noexcept(noexcept(cell_extracted(cell(env, val))))
  { return cell_extracted(cell(env, val)); }
};

/** @} */
}

#endif /* CPPEMACS_WRAPPERS_HPP_ */
