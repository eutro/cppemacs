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
#include <cstring>
#include <memory>
#include <unordered_map>

#ifdef __cpp_lib_string_view
#include <string_view>
#endif

namespace cppemacs {

/** \defgroup Utilities
 *
 * @brief Utility types built on top of the core cppemacs API.
 */

/** \addtogroup Utilities
 * @{
 */

/** An environment that interns symbols on the C++-side. */
struct intern_env : public envw {
  template <typename...Args>
  intern_env(Args &&...args): envw(std::forward<Args>(args)...) {}

private:
  struct table_key {
    /* must have static lifetime */
    const char *name; size_t len;
    table_key(const char *name, size_t len): name(name), len(len) {}
    bool operator==(const table_key &o) const {
      return len == o.len &&
        (name == o.name || std::strncmp(name, o.name, len) == 0);
    }
  };
  struct hasher {
    size_t operator()(const table_key &key) const noexcept {
#ifdef __cpp_lib_string_view
      return std::hash<std::string_view>()(std::string_view(key.name, key.len));
#else
      // Java-like hashCode
      size_t hash = 7;
      for (size_t i = 0; i < key.len; ++i) {
        hash = (31 * hash) + static_cast<size_t>(key.name[i]);
      }
      return hash;
#endif
    }
  };
  std::unordered_map<table_key, value, hasher> table;

public:
  value intern(const char *name, size_t len) noexcept {
    auto pair = table.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(name, len),
      std::forward_as_tuple(nullptr));
    if (pair.second) {
      return pair.first->second = envw::intern(name);
    } else {
      return pair.first->second;
    }
  }

  value intern(const char *name) noexcept { return intern(name, std::strlen(name)); }

  /** Get a cell from a value. */
  cell operator->*(value val) const noexcept { return envw::operator->*(val); }
  template <typename T> cell operator->*(T &&arg) const
  { return *this->*to_emacs(expected_type_t<detail::decay_t<T>>{}, *this, std::forward<T>(arg)); }
  cell operator->*(const char *name) { return *this->*intern(name); }
};

struct vec_cell : cell {
  template <typename...Args>
  vec_cell(Args &&...args): cell(std::forward<Args>(args)...) {}

  struct reference {
    cell v;
    ptrdiff_t idx;

    reference(cell v, intmax_t idx): v(v), idx(idx) {}

    operator cell() const {
      value ret = v->vec_get(v, idx);
      v->maybe_non_local_exit();
      return v->*ret;
    }
    cell get() const { return *this; }
    cell operator=(value x) const {
      v->vec_set(v, idx, x);
      v->maybe_non_local_exit();
      return v->*x;
    }
    cell set(value x) const { return *this = x; }

    reference &operator*() noexcept { return *this; }
    const reference &operator*() const noexcept { return *this; }

    reference &operator+=(ptrdiff_t n) noexcept { idx += n; return *this; }
    reference operator+(ptrdiff_t n) const noexcept { return reference(v, idx + n); }
    reference &operator++() noexcept { ++idx; return *this; }
    reference operator++(int) noexcept { reference ret = *this; ++*this; return ret; }
    reference &operator-=(ptrdiff_t n) noexcept { idx -= n; return *this; }
    reference operator-(ptrdiff_t n) const noexcept { return reference(v, idx - n); }
    reference &operator--() noexcept { --idx; return *this; }
    reference operator--(int) noexcept { reference ret = *this; --*this; return ret; }
    ptrdiff_t operator-(const reference &o) const noexcept { return idx - o.idx; }

    bool operator==(const reference &o) const noexcept { return idx == o.idx; }
    bool operator!=(const reference &o) const noexcept { return idx != o.idx; }
    bool operator<(const reference &o) const noexcept { return idx < o.idx; }
    bool operator>(const reference &o) const noexcept { return idx > o.idx; }
    bool operator<=(const reference &o) const noexcept { return idx <= o.idx; }
    bool operator>=(const reference &o) const noexcept { return idx >= o.idx; }
  };

  reference operator[](ptrdiff_t idx) const { return reference(*this, idx); }
  ptrdiff_t size() const {
    ptrdiff_t ret = nv.vec_size(*this);
    nv.maybe_non_local_exit();
    return ret;
  }

  reference begin() const { return (*this)[0]; }
  reference end() const { return (*this)[size()]; }
};

/** Type-safe Emacs `user_ptr` representation. */
template <typename T, typename Deleter = std::default_delete<T>>
struct user_ptr {
  static_assert(!std::is_reference<T>::value, "Must not be a reference type");
  static_assert(std::is_default_constructible<Deleter>::value, "Deleter must be default-constructible");
  T *ptr;
  user_ptr(T *ptr) noexcept: ptr(ptr) {}

  T *get() const noexcept { return ptr; }
  T *operator->() const noexcept { return ptr; }
  T &operator*() const noexcept { return *ptr; }

  static void fin(void *ptr) noexcept {
    try {
      Deleter()(reinterpret_cast<T*>(ptr));
    } catch (...) {
      // TODO: log warnings?
    }
  }

  /** Convert a `user_ptr` to Emacs, with the GC becoming responsible for the object. */
  friend value to_emacs(expected_type_t<user_ptr>, envw nv, const user_ptr &ptr)
  { return nv.make_user_ptr(user_ptr<T, Deleter>::fin, reinterpret_cast<void*>(ptr.ptr)); }

  /** Convert a `user_ptr` from Emacs, with the GC still responsible for the object. */
  friend user_ptr from_emacs(expected_type_t<user_ptr>, envw nv, value val) {
    emacs_finalizer fin = nv.get_user_finalizer(val);
    nv.maybe_non_local_exit();
    if (fin != user_ptr<T, Deleter>::fin) {
      throw std::runtime_error("User ptr type mismatch");
    }
    return user_ptr<T, Deleter>(reinterpret_cast<T*>(nv.get_user_ptr(val)));
  }
};

template <typename T, typename...Args> inline user_ptr<T>
make_user_ptr(Args &&...args) { return user_ptr<T>(new T(std::forward<Args>(args)...)); }

/**
 * Function representation, for storing C++ functions in Emacs
 * closures.
 *
 * Emacs lets us store one pointer worth of data, the default
 * implementation puts it in an off-heap pointer and attaches a
 * finalizer.
 */
template <typename F, typename = void>
struct module_function_repr {
  static F &extract(void *ptr) noexcept { return *reinterpret_cast<F *>(ptr); }
  static value make(
    envw nv, ptrdiff_t min_arity, ptrdiff_t max_arity,
    emacs_function fun, const char *doc, F &&f
  ) {
    nv.maybe_non_local_exit();

    // The strategy here is to bind the module function to a new
    // symbol, which then has a finalizer attached. We own the
    // instance of F precisely until the finalizer is created,
    // at which point it is owned by the GC.

    std::unique_ptr<F> fptr(new F(std::move(f)));
    void *fptr_data = reinterpret_cast<void *>(fptr.get());
    emacs_finalizer fin = [](void *data) noexcept { delete reinterpret_cast<F *>(data); };

    value retfn = nv.make_function(
      min_arity, max_arity, fun, doc,
      fptr_data); // fptr still owned by us for a bit

#if (EMACS_MAJOR_VERSION >= 28) // we have module finalizers, maybe
    if (nv->size >= sizeof(emacs_env_28)) {
      // we have function finalizers, just attach it
      nv.set_function_finalizer(retfn, fin);
      nv.maybe_non_local_exit();
      fptr.release(); // now managed by the GC
      return retfn;
    }
#endif
    value gensym = nv.intern("gensym");
    // bind the module function to a new symbol, for which we can add a finalizer
    value func_sym = (nv->*gensym)(std::string("c++fun-"));
    (nv->*nv.intern("defalias"))(func_sym, retfn);

    // make a user pointer as a finalizer
    value finalizer = nv.make_user_ptr(fin, fptr_data);
    nv.maybe_non_local_exit();
    fptr.release(); // the finalizer was created successfully, F is now managed by the GC

    // attach the finalizer, so that the lifetimes of `retfn` and the finalizer are tied
    // (unless users do something stupid!)
    nv.funcall(nv.intern("put"), {func_sym, (nv->*gensym)(), finalizer});

    return func_sym; // an alias to our module function in a fresh uninterned symbol
  }
};

namespace detail {
template <typename T>
struct can_stuff_into_void_ptr : std::integral_constant<bool, (
  sizeof(T) <= sizeof(void*)
  && std::is_trivially_copyable<T>::value
  && std::is_trivially_destructible<T>::value
)> {};
}

/**
 * Function representation, for storing C++ functions in Emacs
 * closures.
 *
 * Emacs lets us store one pointer worth of data. This specialization
 * simply stores the bits of F in that pointer, provided that it fits,
 * and provided that the type is trivially destructible and copyable.
 */
template <typename F>
struct module_function_repr<F, detail::enable_if_t<detail::can_stuff_into_void_ptr<F>::value>>
{
  static F &extract(void *&ptr) { return reinterpret_cast<F &>(ptr); }
  static value make(
    envw nv, ptrdiff_t min_arity, ptrdiff_t max_arity,
    emacs_function fun, const char *doc, F &&f) {
    return nv.make_function(
      min_arity, max_arity, fun, doc,
      reinterpret_cast<void *const &>(f));
  }
};

template <typename F>
struct module_function {
  static_assert(!std::is_reference<F>::value, "Must not be a reference type");
#ifdef __cpp_lib_is_invocable
  static_assert(std::is_nothrow_invocable_r_v<value, F&, emacs_env *, ptrdiff_t, value *>,
                "Must be no-throw invocable with Emacs function arguments\n"
                "Add `nothrow` and consider wraping with `env.run_catching`");
#endif

  ptrdiff_t min_arity, max_arity;
  const char *doc;
  F f;
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

  using data_repr = module_function_repr<F>;

  static value invoke(emacs_env *nv, ptrdiff_t nargs, value *args, void *data) noexcept {
    return data_repr::extract(data)(nv, nargs, args);
  }

  friend value to_emacs(expected_type_t<module_function>, envw nv, module_function &&func) {
    return data_repr::make(
      nv, func.min_arity, func.max_arity, &invoke, func.doc,
      std::move(func.f));
  }
};

/** Make a module function with the given minimum and maximum arity. */
template <typename F> auto make_module_function(
  ptrdiff_t min_arity, ptrdiff_t max_arity,
  const char *doc, F &&f
) -> module_function<detail::remove_reference_t<F>> {
  return module_function<detail::remove_reference_t<F>>(
    min_arity, max_arity, doc,
    std::forward<F>(f));
}

namespace detail {

#if defined(__cpp_lib_integer_sequence)
template <size_t...Idx> using index_sequence = std::index_sequence<Idx...>;
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

struct unprovided_value_t {
  template <typename T> operator T() const {
    // explicitly no SFINAE
    static_assert(std::is_default_constructible<T>::value, "Optional parameter must be default constructible");
    return T();
  }
};

template <ptrdiff_t MinArity, ptrdiff_t MaxArity, bool IsVariadic, typename F>
struct spread_invoker {
  F f;
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
 * Returns a `module_function` that takes between `MinArity` and
 * `MaxArity` arguments. F will be invoked with an `emacs_env *`
 * followed by `MaxArity` value arguments. Arguments that are provided
 * by the caller will be given as `cell`s, whereas absent arguments
 * will be default-initialized.
 *
 * With `IsVariadic` true, F is additionally invoked with a
 * `{value *, size_t}` argument containing the rest args.
 */
template <ptrdiff_t MaxArity, bool IsVariadic = false, ptrdiff_t MinArity = MaxArity, typename F>
auto make_spreader_function(
  const char *doc, F &&f
) -> module_function<detail::spread_invoker<MinArity, MaxArity, IsVariadic, detail::remove_reference_t<F>>> {
  static_assert(MaxArity >= MinArity, "MaxArity must be greater than or equal to MinArity");
  static_assert(MinArity >= 0, "MinArity must be nonnegative");
  return make_module_function(
    MinArity, IsVariadic ? emacs_variadic_function : MaxArity, doc,
    detail::spread_invoker<MinArity, MaxArity, IsVariadic,
    detail::remove_reference_t<F>>(std::forward<F>(f)));
}

inline std::ostream &operator<<(std::ostream &os, const cell &v) {
  return os << (v->*"format")(std::string("%s"), v).unwrap<std::string>();
}

/** @} */

}

#endif /* CPPEMACS_WRAPPERS_HPP_ */
