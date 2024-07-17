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

static std::string translate_signal(signalled const &sig) {
  return (envp->*"error-message-string")((envp->*"cons")(sig.symbol, sig.data))
    .extract<std::string>();
}
static std::string translate_throw(thrown const &sig) {
  return (envp->*"format")("(throw '%S '%S)"_Estr, sig.symbol, sig.data)
    .extract<std::string>();
}
static std::string translate_non_local_exit(non_local_exit const &) {
  value symbol, data;
  funcall_exit ty = envp.non_local_exit_get(symbol, data);
  envp.non_local_exit_clear();
  switch (ty.raw) {
  case funcall_exit::signal_: return translate_signal({symbol, data});
  case funcall_exit::throw_: return translate_throw({symbol, data});
  default: return "<unknown non-local exit>";
  }
}

class EmacsExnCkListener : public Catch::EventListenerBase {
public:
  using Catch::EventListenerBase::EventListenerBase;

  void check_status(const char *where, const Catch::StringRef &) {
    if (envp.non_local_exit_check()) {
      std::string transl = translate_non_local_exit(non_local_exit{});
      WARN("Exception (" << where << "): " << transl);
    }
  }

  void testRunStarting(Catch::TestRunInfo const &inf) override
  { check_status("Test Run Starting", inf.name); }
  void testRunEnded(Catch::TestRunStats const &stats) override
  { check_status("Test Run Ended", stats.runInfo.name); }
  void testCaseStarting(Catch::TestCaseInfo const &inf) override
  { check_status("Test Case Starting", inf.name); }
  void testCaseEnded(Catch::TestCaseStats const &) override
  { envp.non_local_exit_clear(); }
  void sectionStarting(Catch::SectionInfo const &inf) override
  { check_status("Section Starting", inf.name); }
  void sectionEnded(Catch::SectionStats const &) override
  { envp.non_local_exit_clear(); }
  void assertionStarting(Catch::AssertionInfo const &inf) override
  { check_status("Assertion Starting", inf.capturedExpression); }
  void assertionEnded(Catch::AssertionStats const &) override
  { envp.non_local_exit_clear(); }
};

CATCH_REGISTER_LISTENER(EmacsExnCkListener);

CATCH_TRANSLATE_EXCEPTION(signalled const &sig) { return translate_signal(sig); }
CATCH_TRANSLATE_EXCEPTION(thrown const &sig) { return translate_throw(sig); }
CATCH_TRANSLATE_EXCEPTION(non_local_exit const &sig) { return translate_non_local_exit(sig); }
