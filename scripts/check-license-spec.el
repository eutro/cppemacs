;; -*- lexical-binding: t -*-

;; Copyright (C) 2024 Eutro <https://eutro.dev>
;;
;; This file is part of cppemacs.
;;
;; cppemacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; cppemacs is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
;; General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with cppemacs. If not, see <https://www.gnu.org/licenses/>.
;;
;; SPDX-FileCopyrightText: 2024 Eutro <https://eutro.dev>
;;
;; SPDX-License-Identifier: GPL-3.0-or-later

((license "scripts/license-header-gpl.txt")
 (recurse
  (".")
  "CMakeLists.txt"
  "*.cmake"
  "Makefile"
  "*.md"
  "*.el"
  "*.hpp" "*.cpp"
  (recurse ("scripts" "include/cppemacs" "tests" "cmake") ..)

  (recurse
   ("docs")
   "CMakeLists.txt"
   "*.css"
   (recurse ("examples") "*.cpp")
   (recurse ("*.html" "*.xml") (license "../scripts/license-header-gfdl.txt")))))
