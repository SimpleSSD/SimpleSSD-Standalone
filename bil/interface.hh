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

namespace Standalone::BIL {

class DriverInterface : public Object {
 public:
  DriverInterface(ObjectData &o) : Object(o) {}
  virtual ~DriverInterface() {}

  virtual void init(SimpleSSD::Event) = 0;
  virtual void getInfo(uint64_t &, uint32_t &) = 0;
  virtual void submitIO(BIO &) = 0;
};

}  // namespace Standalone::BIL

#endif
