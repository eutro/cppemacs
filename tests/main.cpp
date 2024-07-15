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

#include "common.hpp"

envw cppemacs::envp = nullptr;

static int run_tests(envw nv, const std::vector<std::string> &args) {
  std::vector<const char *> argv(args.size() + 1);
  {
    auto itl = argv.begin() + 1; auto itr = args.begin();
    for (; itl != argv.end(); ++itl, ++itr) *itl = itr->c_str();
  }
  argv[0] = "cppemacs-test";

  envp = nv;
  int ret = Catch::Session().run(argv.size(), argv.data());
  envp = nullptr;

  return ret;
}

extern "C" {

int plugin_is_GPL_compatible;

int emacs_module_init(emacs_runtime *rt) noexcept {
  envw nv(rt->get_environment(rt));
  if (!nv.is_compatible<25>()) return 1;

  nv.run_catching([&]() {
    (nv->*"defalias")(
      "cppemacs-test",
      make_spreader_function(
        spreader_arity<1>(),
        "Run cppemacs tests",
        [](envw nv, cell args) {
          std::vector<std::string> sargs(args.vec_size());
          nv.maybe_non_local_exit();
          for (size_t ii = 0; ii < sargs.size(); ++ii) {
            sargs[ii] = args.vec_get(ii).extract<std::string>();
          }
          return run_tests(nv, sargs);
        }));
  });

  return 0;
}

}
