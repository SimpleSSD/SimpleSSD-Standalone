// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __IGL_IO_GEN__
#define __IGL_IO_GEN__

#include "bil/entry.hh"
#include "sim/object.hh"

namespace Standalone::IGL {

class IOGenerator : public Object {
 protected:
  BIL::BlockIOEntry &bioEntry;

  Event endCallback;

 public:
  IOGenerator(ObjectData &o, BIL::BlockIOEntry &b, Event e)
      : Object(o), bioEntry(b), endCallback(e) {}
  virtual ~IOGenerator() {}

  virtual void init(uint64_t, uint32_t) = 0;
  virtual void begin() = 0;
  virtual void printStats(std::ostream &) = 0;
  virtual void getProgress(float &) = 0;
};

}  // namespace Standalone::IGL

#endif
