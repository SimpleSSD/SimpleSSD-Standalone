// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __IGL_IO_GENERATOR_HH__
#define __IGL_IO_GENERATOR_HH__

#include "igl/block_io.hh"
#include "main/object.hh"

namespace Standalone::IGL {

class AbstractIOGenerator : public Object {
 protected:
  BlockIOLayer &bioEntry;

  // Call this event to terminate simulation
  Event endCallback;

 public:
  AbstractIOGenerator(ObjectData &o, BlockIOLayer &b, Event e)
      : Object(o), bioEntry(b), endCallback(e) {}
  virtual ~AbstractIOGenerator() {}

  virtual void begin() = 0;

  virtual void printStats(std::ostream &) = 0;
  virtual void getProgress(float &) = 0;
};

}  // namespace Standalone::IGL

#endif
