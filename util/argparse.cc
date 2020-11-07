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
static const char str_true[] = "true";

ArgumentParser::ArgumentParser(int argc, char *argv[]) : valid(true), pos(0) {
  parse(argc, argv);
}

ArgumentParser::ArgumentParser(int argc, char *argv[], int p)
    : valid(true), pos(p) {
  parse(argc, argv);
}

ArgumentParser::~ArgumentParser() {}

void ArgumentParser::parse(int argc, char *argv[]) {
  bool value = false;
  std::cmatch match;
  std::string key;

  for (int i = 1; i < argc; i++) {
    char *param = argv[i];

    if (param[0] != '-') {
      if (value) {
        if (key.length() == 0) {
          goto out;
        }

        value = false;

        args.emplace(key, param);
      }
      else if (pos > 0) {
        pos--;
        posargs.emplace_back(param);
      }
      else {
        goto out;
      }
    }
    else {
      if (strcmp(param, "--") == 0) {
        value = false;

        // Assume positional arguments after this
        for (int j = i + 1; j < argc; j++) {
          if (pos > 0) {
            pos--;
            posargs.emplace_back(argv[j]);
          }
          else {
            goto out;
          }
        }

        break;
      }
      else if (std::regex_match(param, match, regex_arg)) {
        if (value) {
          // We got two consecutive options
          args.emplace(key, str_true);
        }

        key = match[1].str();

        if (match.length() > 2 && match[2].matched) {
          args.emplace(key, match[2].first);
        }
        else {
          value = true;
        }
      }
      else {
        goto out;
      }
    }
  }

  if (value && key.length() > 0) {
    args.emplace(key, str_true);
  }

  return;
out:
  valid = false;
}

const char *ArgumentParser::getArgument(std::string key,
                                        std::string key2) noexcept {
  auto iter = args.find(key);

  if (iter != args.end()) {
    return iter->second;
  }

  // Check with key 2
  if (key.length() > 0) {
    iter = args.find(key2);

    if (iter != args.end()) {
      return iter->second;
    }
  }

  return nullptr;
}

const char *ArgumentParser::getPositionalArgument(int pos) noexcept {
  if (pos < (int)posargs.size()) {
    return posargs.at(pos);
  }

  return nullptr;
}

}  // namespace Standalone
