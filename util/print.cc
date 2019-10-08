// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "util/print.hh"

void print(std::ostream &out, std::string str, uint32_t width) {
  if (str.length() >= width) {
    out << str;
  }
  else {
    width -= (uint32_t)str.length();

    str.append(width, ' ');

    out << str;
  }
}

void print(std::ostream &out, double val, uint32_t width) {
  print(out, std::to_string(val), width);
}
