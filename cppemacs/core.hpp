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

/**
 * Cppemacs -- C++11 Emacs module API wrapper.
 */

#include <cassert>
#include <cstring>
#include <emacs-module.h>

#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <utility>

namespace cppemacs {

/** Alias for `emacs_value` */
using value = emacs_value;

/** Enum wrapper for `emacs_funcall_exit` */
struct funcall_exit {
  emacs_funcall_exit raw;
  constexpr funcall_exit(emacs_funcall_exit raw) noexcept: raw(raw) {}
  constexpr operator emacs_funcall_exit() const noexcept { return raw; }

  static constexpr emacs_funcall_exit return_ = emacs_funcall_exit_return;
  static constexpr emacs_funcall_exit signal_ = emacs_funcall_exit_signal;
  static constexpr emacs_funcall_exit throw_ = emacs_funcall_exit_throw;

  operator bool() const { return raw != return_; }
};

/** Enum wrapper for `emacs_process_input_result` */
struct process_input_result {
  emacs_process_input_result raw;
  constexpr process_input_result(emacs_process_input_result raw) noexcept: raw(raw) {}
  constexpr operator emacs_process_input_result() const noexcept { return raw; }

  static constexpr emacs_process_input_result continue_ = emacs_process_input_continue;
  static constexpr emacs_process_input_result quit_ = emacs_process_input_quit;

  operator bool() const { return raw != continue_; }
};

/** A valueless exception indicating a pending non-local exit. */
struct non_local_exit {};

/** An exception representing an Emacs `signal` exit. */
struct signal {
  value symbol; value data;
  signal(value symbol, value data) noexcept
    : symbol(symbol), data(data) {}
};

/** An exception representing an Emacs `throw` non-local exit. */
struct thrown {
  value symbol; value data;
  thrown(value symbol, value data) noexcept
    : symbol(symbol), data(data) {}
};

struct cell;

/** Marker type for converting Emacs values to C++ values. */
template <typename T> struct expected_type_t {};

/** Transparent wrapper for an Emacs environment. */
struct env {
private:
  emacs_env *raw;
public:
  env(emacs_env *raw) noexcept: raw(raw) {}
  operator emacs_env *() const noexcept { return raw; }
  emacs_env *operator->() const noexcept { return raw; }
  emacs_env &operator*() const noexcept { return *raw; }

  /** @brief Get the major version of the current `emacs_env`
   * @return The minimum known major version, or UINT_MAX if unknown. */
  unsigned int major_version() const noexcept {
    struct entry { size_t size; int ver; };
    static constexpr entry entries[] = {
#if (EMACS_MAJOR_VERSION >= 25)
      { sizeof(emacs_env_25), 25 },
#endif
#if (EMACS_MAJOR_VERSION >= 26)
      { sizeof(emacs_env_26), 26 },
#endif
#if (EMACS_MAJOR_VERSION >= 27)
      { sizeof(emacs_env_27), 27 },
#endif
#if (EMACS_MAJOR_VERSION >= 28)
      { sizeof(emacs_env_28), 28 },
#endif
#if (EMACS_MAJOR_VERSION >= 29)
      { sizeof(emacs_env_29), 29 },
#endif
    };
    size_t size = raw->size;
    for (const auto &e : entries) {
      if (size <= e.size) return e.ver;
    }
    return std::numeric_limits<unsigned int>::max();
  }

  static_assert(EMACS_MAJOR_VERSION >= 25, "Bad Emacs installation");

  /* Memory management. */
  value make_global_ref(value val) const noexcept { return raw->make_global_ref(raw, val); }
  void free_global_ref(value global_value) const noexcept { return raw->free_global_ref(raw, global_value); }

  /* Non-local exit handling. */
  funcall_exit non_local_exit_check() const noexcept { return raw->non_local_exit_check(raw); }
  void non_local_exit_clear() const noexcept { return raw->non_local_exit_clear(raw); }
  funcall_exit non_local_exit_get(value &symbol, value &data) const noexcept { return raw->non_local_exit_get(raw, &symbol, &data); }
  void non_local_exit_signal(value symbol, value data) const noexcept { return raw->non_local_exit_signal(raw, symbol, data); }
  void non_local_exit_throw(value symbol, value data) const noexcept { return raw->non_local_exit_throw(raw, symbol, data); }

  /** Check for non-local exit, and throw if there is one. */
  void maybe_non_local_exit() const noexcept(false) {
    if (non_local_exit_check())
      throw non_local_exit{};
  }
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
  value make_function(ptrdiff_t min_arity, ptrdiff_t max_arity,
                      emacs_function func, const char *docstring,
                      void *data) const noexcept
  { return raw->make_function(raw, min_arity, max_arity, func, docstring, data); }

  value funcall(value func, ptrdiff_t nargs, value *args) const noexcept { return raw->funcall(raw, func, nargs, args); }

  value funcall(value func, std::initializer_list<value> args) const noexcept {
    return funcall(func, args.size(), const_cast<value*>(args.begin()));
  }

  value intern(const char *name) const noexcept { return raw->intern(raw, name); }

  /* Type conversion. */
  value type_of(value arg) const noexcept { return raw->type_of(raw, arg); }
  bool is_not_nil(value arg) const noexcept { return raw->is_not_nil(raw, arg); }
  bool eq(value a, value b) const noexcept { return raw->eq(raw, a, b); }
  intmax_t extract_integer(value arg) const noexcept { return raw->extract_integer(raw, arg); }
  value make_integer(intmax_t n) const noexcept { return raw->make_integer(raw, n); }
  double extract_float(value arg) const noexcept { return raw->extract_float(raw, arg); }
  value make_float(double d) const noexcept { return raw->make_float(raw, d); }

  /* Copy the content of the Lisp string VALUE to BUFFER as an utf8
     null-terminated string.

     SIZE must point to the total size of the buffer.  If BUFFER is
     NULL or if SIZE is not big enough, write the required buffer size
     to SIZE and return true.

     Note that SIZE must include the last null byte (e.g. "abc" needs
     a buffer of size 4).

     Return true if the string was successfully copied.  */
  bool copy_string_contents(value val, char *buf, ptrdiff_t &len) const noexcept {
    return raw->copy_string_contents(raw, val, buf, &len);
  }

  /* Create a Lisp string from a utf8 encoded string.  */
  value make_string(const char *str, ptrdiff_t len) const noexcept { return raw->make_string(raw, str, len); }
  value make_string(const std::string &str) const noexcept { return make_string(str.data(), str.length()); }

  /* Embedded pointer type. */
  value make_user_ptr(emacs_finalizer fin, void *ptr) const noexcept { return raw->make_user_ptr(raw, fin, ptr); }

  void *get_user_ptr(value arg) const noexcept { return raw->get_user_ptr(raw, arg); }
  void set_user_ptr(value arg, void *ptr) const noexcept { return raw->set_user_ptr(raw, arg, ptr); }
  emacs_finalizer get_user_finalizer(value uptr) const noexcept { return raw->get_user_finalizer(raw, uptr); }
  void set_user_finalizer(value uptr, emacs_finalizer fin) const noexcept { return raw->set_user_finalizer(raw, uptr, fin); }

  /* Vector functions. */
  value vec_get(value vector, ptrdiff_t index) const noexcept { return raw->vec_get(raw, vector, index); }
  void vec_set(value vector, ptrdiff_t index, value value) const noexcept { return raw->vec_set(raw, vector, index, value); }
  ptrdiff_t vec_size(value vector) const noexcept { return raw->vec_size(raw, vector); }

#if (EMACS_MAJOR_VERSION >= 26)

  /* Returns whether a quit is pending.  */
  bool should_quit() const noexcept { return raw->should_quit(raw); }

#endif // >= 26
#if (EMACS_MAJOR_VERSION >= 27)

  /* Processes pending input events and returns whether the module
     function should quit.  */
  process_input_result process_input() const noexcept { return raw->process_input(raw); }

  struct timespec extract_time(value arg) const noexcept { return raw->extract_time(raw, arg); }
  value make_time(struct timespec time) const noexcept { return raw->make_time(raw, time); }

  bool extract_big_integer(value arg, int *sign, ptrdiff_t *count, emacs_limb_t *magnitude) const noexcept
  { return raw->extract_big_integer(raw, arg, sign, count, magnitude); }
  value make_big_integer(int sign, ptrdiff_t count, const emacs_limb_t *magnitude) const noexcept
  { return raw->make_big_integer(raw, sign, count, magnitude); }

#endif // >= 27
#if (EMACS_MAJOR_VERSION >= 28)

  emacs_finalizer get_function_finalizer(value arg) const noexcept { return raw->get_function_finalizer(raw, arg); }
  void set_function_finalizer(value arg, emacs_finalizer fin) const noexcept { return raw->set_function_finalizer(raw, arg, fin); }
  int open_channel(value pipe_process) const noexcept { return raw->open_channel(raw, pipe_process); }
  void make_interactive(value function, value spec) const noexcept { return raw->make_interactive(raw, function, spec); }
  value make_unibyte_string(const char *str, ptrdiff_t len) const noexcept { return raw->make_unibyte_string(raw, str, len); }

#endif // >= 28

#define CPPEMACS_VERSION_FAIL(N) throw std::runtime_error("Emacs " #N "+ required");
#define CPPEMACS_CHECK_VERSION(env, N)                                  \
  do {                                                                  \
    if ((env)->size < sizeof(emacs_env_##N)) CPPEMACS_VERSION_FAIL(N);  \
  } while(0)                                                            \

#undef CPPEMACS_ENV_FUN

  /** Get a cell from a value. */
  cell operator->*(value val) const noexcept;

  /**
   * @brief Perform an arbitrary conversion to Emacs.
   *
   * Arbitrary conversions are performed using ADL for to_emacs(),
   * called with `*this`, `expected_type_t<decay_t<T>>` and (forwarded) `arg`.
   */
  template <typename T> auto operator->*(T &&arg) const
  { return *this->*to_emacs(expected_type_t<std::decay_t<T>>{}, *this, std::forward<T>(arg)); }

  /**
   * @brief Perform an arbitrary conversion from Emacs.
   *
   * Arbitrary conversions are performed using ADL for from_emacs(),
   * called with `expected_type_t<T>`, `*this` and the `value`.
   */
  template <typename T> T unwrap(expected_type_t<T> tag, value val) const noexcept(false) {
    auto ret = from_emacs(tag, *this, val);
    maybe_non_local_exit();
    return ret;
  }
  template <typename T> T unwrap(value val) const noexcept(false) { return unwrap(expected_type_t<T>{}, val); }

  template <typename F>
  auto run_catching(F f) const noexcept {
    using return_type = std::invoke_result_t<F>;
    static_assert(
      std::is_void_v<return_type> ||
      std::is_nothrow_default_constructible_v<return_type>,
      "Return type must be void or nothrow default constructible");
    try {
      return f();
    } catch (const signal &s) {
      non_local_exit_clear();
      non_local_exit_signal(s.symbol, s.data);
    } catch (const thrown &s) {
      non_local_exit_clear();
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
        {make_string(str, std::strlen(str))}
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

/** A value in a specific env. */
struct cell {
protected:
  env nv;
  value val;

public:
  /** Construct a cell from an environment and value. */
  cell(env nv, value val) noexcept: nv(nv), val(val) {}

  const env *operator->() const { return &nv; }
  template <typename T>
  auto operator->*(T &&arg) const { return nv->*std::forward<T>(arg); }

  operator value() const noexcept { return val; }
  cell &operator=(value new_val) noexcept { val = new_val; return *this; }

  /** Call this value as a function. */
  template <typename...Args>
  cell operator()(Args&&...args) const noexcept(false) {
    value res = nv.funcall(val, {nv->*std::forward<Args>(args)...});
    return nv->*res;
  }

  /** Get the type of this cell. */
  cell type() const {
    nv.maybe_non_local_exit();
    return nv->*nv.type_of(val);
  }

  /** Check if this is non-nil. */
  operator bool() const {
    nv.maybe_non_local_exit();
    return nv.is_not_nil(val);
  }

  /** Compare this against another value. */
  bool operator==(value o) const {
    nv.maybe_non_local_exit();
    return nv.eq(val, o);
  }

  /** Convert this to the given type. */
  template <typename T>
  T unwrap() const noexcept(false) { return nv.unwrap<T>(*this); }

  /** Set this cell from the given value. */
  template <typename T>
  void set(T &&new_val) { *this = nv->*std::forward<T>(new_val); }
};

inline cell env::operator->*(value value) const noexcept { return cell(*this, value); }
inline value to_emacs(expected_type_t<cell>, env, cell x) { return x; }

}

#endif /* CPPEMACS_EMACS_HPP_ */
