/*
 * Copyright (C) 2017 CAMELab
 *
 * This file is part of SimpleSSD.
 *
 * SimpleSSD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSSD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimpleSSD.  If not, see <http://www.gnu.org/licenses/>.
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
