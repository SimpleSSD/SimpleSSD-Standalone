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
#include "sim/object.hh"
#include "simplessd/sim/simplessd.hh"

namespace Standalone::BIL {

class DriverInterface : public Object {
 protected:
  SimpleSSD::SimpleSSD &simplessd;

 public:
  DriverInterface(ObjectData &o, SimpleSSD::SimpleSSD &s)
      : Object(o), simplessd(s) {}
  virtual ~DriverInterface() {}

  virtual void init(Event) = 0;
  virtual void getInfo(uint64_t &, uint32_t &) = 0;
  virtual void submitIO(BIO &) = 0;

  virtual void initStats(std::vector<SimpleSSD::Stat> &) = 0;
  virtual void getStats(std::vector<double> &) = 0;
};

}  // namespace Standalone::BIL

#endif
