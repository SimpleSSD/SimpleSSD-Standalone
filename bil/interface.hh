// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __BIL_DRIVER_INTERFACE__
#define __BIL_DRIVER_INTERFACE__

#include <functional>

#include "bil/entry.hh"
#include "simplessd/sim/statistics.hh"

namespace BIL {

class DriverInterface {
 protected:
  Engine &engine;

  std::function<void()> beginFunction;

 public:
  DriverInterface(Engine &e) : engine(e) {}
  virtual ~DriverInterface() {}

  virtual void init(std::function<void()> &) = 0;
  virtual void getInfo(uint64_t &, uint32_t &) = 0;
  virtual void submitIO(BIO &) = 0;

  virtual void initStats(std::vector<SimpleSSD::Stats> &) = 0;
  virtual void getStats(std::vector<double> &) = 0;
};

}  // namespace BIL

#endif
