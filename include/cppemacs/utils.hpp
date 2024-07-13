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
#include <cstring>
#include <memory>

namespace cppemacs {

/**
 * @defgroup cppemacs_utilities Utilities
 * @brief Utility types built on top of the core cppemacs API.
 *
 * @addtogroup cppemacs_utilities
 * @{
 */

/**
 * @brief Wrapper type over Emacs vectors.
 *
 * Supports the @ref operator[]() "subscript operator" and @ref
 * size().
 */
struct vecw {
private:
  cell c;

public:
  /** @brief Constructor that forwards to @ref cell. */
  template <typename...Args>
  vecw(Args &&...args): c(std::forward<Args>(args)...) {}
  /** @brief Get the underlying @ref cell. */
  operator cell() const { return c; }

  /**
   * @brief An unevaluated reference to a vector element.
   *
   * This can be @ref operator=() "assigned", @ref get() "gotten",
   * or @ref set() "set".
   */
  struct reference {
  private:
    cell v;
    ptrdiff_t idx;

  public:
    /** @brief Construct a reference from a vector and index. */
    reference(cell v, intmax_t idx): v(v), idx(idx) {}

    /** @brief Get the value of the reference. */
    operator cell() const {
      value ret = v->vec_get(v, idx);
      v->maybe_non_local_exit();
      return v->*ret;
    }

    /** @brief Get the value of the reference, performing @ref cppemacs_conversions "conversions". */
    template <FROM_EMACS_TYPE T = cell>
    T get() const { return cell(*this).extract<T>(); }

    /** @brief Set the reference to the given value. */
    cell operator=(value x) const {
      v->vec_set(v, idx, x);
      v->maybe_non_local_exit();
      return v->*x;
    }

    /** @brief Set the reference to the given value, performing @ref cppemacs_conversions "conversions". */
    template <TO_EMACS_TYPE T>
    cell set(T &&x) const { return *this = v->*std::forward<T>(x); }
  };

  /** @brief Reference the idx-th element of the vector. */
  reference operator[](ptrdiff_t idx) const { return reference(*this, idx); }

  /** @brief Get the size of the vector. */
  ptrdiff_t size() const {
    ptrdiff_t ret = c->vec_size(c);
    c->maybe_non_local_exit();
    return ret;
  }
};

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
    if (nv->size >= sizeof(emacs_env_28)) {
      // we have function finalizers, just attach it
      nv.set_function_finalizer(retfn, fin);
      if (nv.non_local_exit_check()) return nullptr;
      fptr.release(); // now managed by the GC
      return retfn;
    }
#endif
    value gensym = nv.intern("gensym");
    // bind the module function to a new symbol, for which we can add a finalizer
    value func_sym = (nv->*gensym)(nv.make_string("c++fun-"));
    (nv->*nv.intern("defalias"))(func_sym, retfn);

    // make a user pointer as a finalizer
    value finalizer = nv.make_user_ptr(fin, fptr_data);
    if (nv.non_local_exit_check()) return nullptr;
    fptr.release(); // the finalizer was created successfully, F is now managed by the GC

    // attach the finalizer, so that the lifetimes of `retfn` and the finalizer are tied
    // (unless users do something stupid!)
    nv.funcall(nv.intern("put"), {func_sym, (nv->*gensym)(), finalizer});
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
 * A reference to `F` must be nothrow invocable with (@ref emacs_env
 * *env, ptrdiff_t nargs, @ref value *args), returning a @ref
 * value. This differs from a simple @ref emacs_function, in that the
 * `data` parameter is converted to `F` according to the @ref
 * data_repr.
 */
template <typename F>
struct module_function {
  static_assert(!std::is_reference<F>::value, "Must not be a reference type");
#ifdef CPPEMACS_HAVE_IS_INVOCABLE
  static_assert(std::is_nothrow_invocable_r_v<value, F&, emacs_env *, ptrdiff_t, value *>,
                "Must be no-throw invocable with Emacs function arguments\n"
                "Add `nothrow` and consider wraping with `env.run_catching`");
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

  /** @brief The minimum number of arguments that F is to be called with. */
  ptrdiff_t min_arity;
  /** @brief The maximum number of arguments that F is to be called with. */
  ptrdiff_t max_arity;
  /** @brief The documentation string for the Emacs function that will
   * be created. */
  const char *doc;
  /** @brief The invokable function. */
  F f;

  /** @brief Construct with the provided arities, doc-string and
   * in-place arguments for F. */
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
    return data_repr::extract(data)(nv, nargs, args);
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

/** Make a module_function with the given minimum and maximum arity. */
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

namespace detail {

#if (defined(__cpp_lib_integer_sequence) || (__cplusplus >= 201304L)) || defined(CPPEMACS_DOXYGEN_RUNNING)
/** @brief C++14 @c std::index_sequence */
template <size_t...Idx> using index_sequence = std::index_sequence<Idx...>;
/** @brief C++14 @c std::make_index_sequence */
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

/** @brief Used to provide a default-constructed argument to a function. */
struct unprovided_value_t {
  /** @brief Return a default-constructed T. */
  template <typename T> operator T() const {
    static_assert(std::is_default_constructible<T>::value, "Optional parameter must be default constructible");
    return T();
  }
};

/** @brief A wrapper over F that adapts it to an @ref module_function style callable.
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
  template <bool IsProvided> detail::enable_if_t<IsProvided, cell>
  arg_or_default(envw nv, value *args, size_t idx) noexcept(false) { return nv->*args[idx]; }
  template <bool IsProvided> detail::enable_if_t<!IsProvided, unprovided_value_t>
  arg_or_default(envw, value *, size_t) noexcept { return {}; }

  template <size_t NArgs, size_t...Idx, bool IsVar = IsVariadic> detail::enable_if_t<!IsVar, value>
  invoke_with_arity(detail::index_sequence<Idx...>, envw nv, value *args) noexcept(false)
  { return nv->*f(nv, arg_or_default<Idx < NArgs>(nv, args, Idx)...); }
  template <size_t...Idx, bool IsVar = IsVariadic> detail::enable_if_t<!IsVar, value>
  invoke_variadic(detail::index_sequence<Idx...>, envw, ptrdiff_t, value *) noexcept(false) { return {}; }

  template <size_t NArgs, size_t...Idx, bool IsVar = IsVariadic> detail::enable_if_t<IsVar, value>
  invoke_with_arity(detail::index_sequence<Idx...>, envw nv, value *args) noexcept(false)
  { return nv->*f(nv, arg_or_default<Idx < NArgs>(nv, args, Idx)..., {nullptr, 0}); }
  template <size_t...Idx, bool IsVar = IsVariadic> detail::enable_if_t<IsVar, value>
  invoke_variadic(detail::index_sequence<Idx...>, envw nv, ptrdiff_t nargs, value *args) noexcept(false)
  { return nv->*f(nv, arg_or_default<true>(nv, args, Idx)..., {args + MaxArity, nargs - MaxArity}); }

  template <size_t...OffIdx>
  value invoke(
    detail::index_sequence<OffIdx...>,
    envw nv, ptrdiff_t nargs, value *args
  ) noexcept(false) {
    using arity_seq = detail::make_index_sequence<MaxArity>;
    if (IsVariadic && nargs > MaxArity) {
      return invoke_variadic(arity_seq(), nv, nargs, args);
    } else if (nargs < MinArity || nargs > MaxArity) {
      // should have been caught by Emacs
      throw std::runtime_error("Bad arity");
    } else {
      static constexpr value (*Vt[])(spread_invoker *, envw, value*) = {
        ([](spread_invoker *self, envw nv, value *args)
        { return self->invoke_with_arity<MinArity + OffIdx>(arity_seq(), nv, args); })
        ...
      };
      return Vt[nargs](this, nv, args);
    }
  }

public:
  /** @brief Call the underlying function with the given arguments. */
  value operator()(envw nv, ptrdiff_t nargs, value *args) noexcept {
    return nv.run_catching([&]() noexcept(false) {
      return invoke(
        detail::make_index_sequence<MaxArity - MinArity + 1>(),
        nv, nargs, args);
    });
  }
};
}

/**
 * Make a spreader module function with the given minimum and maximum arity.
 *
 * Returns a module_function that takes between `MinArity` and
 * `MaxArity` arguments. `f` will be invoked with an `emacs_env *`
 * followed by `MaxArity` value arguments. Arguments that are provided
 * by the caller will be given as `cell`s, whereas absent arguments
 * will be default-initialized.
 *
 * With `IsVariadic` true, the function becomes variadic. `f` is
 * additionally invoked with a `{value *, size_t}` argument containing
 * the remaining arguments after the first `MaxArity`.
 *
 * Examples:
 *
 * @code
 * make_spreader_function<0>(
 *   "This function takes zero arguments.",
 *   [](envw env) {
 *     // ...
 *   })
 * @endcode
 *
 * @code
 * make_spreader_function<2, false, 1>(
 *   "This function takes one or two arguments.",
 *   [](envw env, value arg1, value arg2) {
 *     if (arg2) {
 *       // invoked with two arguments
 *     } else {
 *       // invoked with one argument
 *     }
 *   })
 * @endcode
 *
 * @code
 * make_spreader_function<1, true>(
 *   "This function takes one or more arguments.",
 *   [](envw env, value arg1, std::span<value> rest) {
 *     // ...
 *   })
 * @endcode
 */
template <ptrdiff_t MaxArity, bool IsVariadic = false, ptrdiff_t MinArity = MaxArity, typename F>
auto make_spreader_function(
  const char *doc, F &&f
)
#if !CPPEMACS_HAVE_RETURN_TYPE_DEDUCTION
  -> module_function<detail::spread_invoker<MinArity, MaxArity, IsVariadic, detail::remove_reference_t<F>>>
#endif
{
  static_assert(MaxArity >= MinArity, "MaxArity must be greater than or equal to MinArity");
  static_assert(MinArity >= 0, "MinArity must be nonnegative");
  return make_module_function(
    MinArity, IsVariadic ? emacs_variadic_function : MaxArity, doc,
    detail::spread_invoker<MinArity, MaxArity, IsVariadic,
    detail::remove_reference_t<F>>(std::forward<F>(f)));
}

/** @brief Output a cell to an output stream, using `(format "%s" v)`. */
inline std::ostream &operator<<(std::ostream &os, const cell &v) {
  return os << (v->*"format")(v->make_string("%s", 2), v).extract<std::string>();
}

/** @} */

}

#endif /* CPPEMACS_WRAPPERS_HPP_ */
