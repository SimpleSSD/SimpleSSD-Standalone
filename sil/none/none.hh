// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __DRIVERS_NONE__
#define __DRIVERS_NONE__

#include "bil/interface.hh"
#include "simplessd/hil/hil.hh"

namespace SIL {

namespace None {

class Driver : public BIL::DriverInterface {
 private:
  SimpleSSD::HIL::HIL *pHIL;

  uint64_t totalLogicalPages;
  uint32_t logicalPageSize;

 public:
  Driver(Engine &, SimpleSSD::ConfigReader &);
  ~Driver();

  void init(std::function<void()> &) override;
  void getInfo(uint64_t &, uint32_t &) override;
  void submitIO(BIL::BIO &) override;

  void initStats(std::vector<SimpleSSD::Stats> &) override;
  void getStats(std::vector<double> &) override;
};

}  // namespace None

}  // namespace SIL

#endif
