// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __UTIL_ARGPARSE_HH__
#define __UTIL_ARGPARSE_HH__

#include <cinttypes>
#include <string>
#include <unordered_map>
#include <vector>

namespace Standalone {

enum {
  LONG_OPT,
  SHORT_OPT,
};

class ArgumentParser {
 private:
  bool valid;
  int pos;
  std::unordered_multimap<std::string, const char *> args;
  std::vector<const char *> posargs;

  void parse(int, char *[]);

 public:
  ArgumentParser(int, char *[]);
  ArgumentParser(int, char *[], int);
  ~ArgumentParser();

  bool isValid() noexcept { return valid; }
  const char *getArgument(const char *[]) noexcept;
  const char *getPositionalArgument(int) noexcept;
  std::vector<const char *> getMultipleArguments(const char *[]);
};

}  // namespace Standalone

#endif
