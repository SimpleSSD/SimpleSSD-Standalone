// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "util/stopwatch.hh"

Stopwatch::Stopwatch() {}

Stopwatch::~Stopwatch() {}

double Stopwatch::getTime() {
  return std::chrono::duration_cast<std::chrono::duration<double>>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

void Stopwatch::start() {
  begin = std::chrono::high_resolution_clock::now();
}

void Stopwatch::stop() {
  end = std::chrono::high_resolution_clock::now();
}

double Stopwatch::getDuration() {
  return std::chrono::duration_cast<std::chrono::duration<double>>(end - begin)
      .count();
}
