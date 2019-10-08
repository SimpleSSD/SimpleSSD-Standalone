// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __UTIL_STOPWATCH__
#define __UTIL_STOPWATCH__

#include <chrono>

class Stopwatch {
 private:
  std::chrono::high_resolution_clock::time_point begin;
  std::chrono::high_resolution_clock::time_point end;

 public:
  Stopwatch();
  ~Stopwatch();

  // Get timepoint
  double getTime();

  // Stopwatch
  void start();
  void stop();
  double getDuration();
};

#endif
