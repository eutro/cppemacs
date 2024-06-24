<!--
- Copyright (C) 2024 Eutro <https://eutro.dev>
-
- This file is part of cppemacs.
-
- cppemacs is free software: you can redistribute it and/or modify it
- under the terms of the GNU General Public License as published by
- the Free Software Foundation, either version 3 of the License, or
- (at your option) any later version.
-
- cppemacs is distributed in the hope that it will be useful, but
- WITHOUT ANY WARRANTY; without even the implied warranty of
- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
- General Public License for more details.
-
- You should have received a copy of the GNU General Public License
- along with cppemacs. If not, see <https://www.gnu.org/licenses/>.
-
- SPDX-FileCopyrightText: 2024 Eutro <https://eutro.dev>
-
- SPDX-License-Identifier: GPL-3.0-or-later
-->

# cppemacs

cppemacs is C++11 API wrapper for writing Emacs [dynamic
modules](https://www.gnu.org/software/emacs/manual/html_node/elisp/Dynamic-Modules.html).

For example:

```c++
#include <cppemacs/all.hpp>

extern "C" {

int plugin_is_GPL_compatible;

int emacs_module_init(emacs_runtime *rt) {
  using namespace cppemacs;
  using namespace cppemacs::literals;

  envw env(rt->get_environment(rt));

  // check that the features you want to use are available
  if (env->size < sizeof(emacs_env_28))
    return 1;

  env.run_catching([&]() {
    // define functions from C++
    (env->*"defalias")(
      "cppemacs-hello-world",
      make_module_function(
        0, 0, "Run Hello, world!",
        [](envw env, ptrdiff_t, value*) noexcept -> value {
          // call Emacs functions with ease
          (env->*"message")(
            "Hello, %s!"_Estr,
            (env->*"read-string")("What is your name? "_Estr)
          );

          return env->*true;
        }));
    });

  return 0;
}

}
```

# Obtaining

## Manually

This library is just the set of headers found in [`include/`](include/). Feel
free to copy these files and use them elsewhere (preferably along with
[`COPYING`](COPYING)).

## With [CMake](https://cmake.org/)

### Locally

You can add cppemacs to your CMake project in a few different ways, such as [git
submodules](https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html)
or [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html),
depending on your CMake version.

For example, on CMake 3.14+:

```cmake
find_package(cppemacs) # try to use a system installation first
# (if using CMake 3.24 or newer, you can use FIND_PACKAGE_ARGS in FetchContent_Declare instead)

if(NOT cppemacs_FOUND) # fall back to downloading
  include(FetchContent) # (CMake 3.11)
  FetchContent_Declare(
    GIT_REPOSITORY https://github.com/eutro/cppemacs
    GIT_TAG master
  )
  FetchContent_MakeAvailable(cppemacs) # (CMake 3.14)
endif()

# add includes to your Emacs dynamic module
add_library(your_emacs_module SHARED)
target_link_libraries(your_emacs_module PRIVATE cppemacs)
```

### Globally

CMake can be used to install cppemacs system-wide (or otherwise).

From the directory containing cppemacs sources:

```sh
# configure, add extra flags like '-GNinja' here
cmake -Bbuild .

# build tests/documentation/etc
cmake --build build

# run tests
cmake --build build -t test

# install system-wide (may need run as root)
cmake --install build --prefix=/path/to/prefix # e.g. /opt/cppemacs
```

cppemacs-specific configure flags:

- `-DCPPEMACS_Test=OFF` do not build tests.

- `-DCPPEMACS_Coverage=ON` build tests with code coverage

- `-DCPPEMACS_Documentation=ON` build documentation, using
  [doxygen](https://www.doxygen.nl/).

- `-DCPPEMACS_Install=OFF` do not add installation rules
