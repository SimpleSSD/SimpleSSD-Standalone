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

namespace Standalone {

class ArgumentParser {
 private:
  bool valid;
  std::unordered_map<std::string, const char *> args;

 public:
  ArgumentParser(int, char *[]);
  ~ArgumentParser();

  bool isValid() noexcept { return valid; }
  const char *getArgument(std::string) noexcept;
};

}  // namespace Standalone

#endif
