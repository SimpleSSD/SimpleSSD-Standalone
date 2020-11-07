// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "util/argparse.hh"

#include <regex>

namespace Standalone {

static const std::regex regex_arg("(-\\w|--[\\w-]+)(?:=(.*))?");

ArgumentParser::ArgumentParser(int argc, char *argv[]) : valid(true) {
  bool value = false;
  std::cmatch match;
  std::string key;

  for (int i = 1; i < argc; i++) {
    char *param = argv[i];

    if (std::regex_match(param, match, regex_arg)) {
      value = false;
      key = match[1].str();

      if (match.length() > 2 && match[2].matched) {
        args.emplace(key, match[2].first);
      }
      else {
        value = true;
      }
    }
    else if (value && key.length() > 0) {
      value = false;

      args.emplace(key, param);
    }
    else {
      valid = false;

      break;
    }
  }
}

ArgumentParser::~ArgumentParser() {}

const char *ArgumentParser::getArgument(std::string key) noexcept {
  auto iter = args.find(key);

  if (iter != args.end()) {
    return iter->second;
  }

  return nullptr;
}

}  // namespace Standalone
