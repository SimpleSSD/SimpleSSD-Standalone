// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __DRIVER_ABSTRACT_INTERFACE_HH__
#define __DRIVER_ABSTRACT_INTERFACE_HH__

#include <functional>

#include "bil/entry.hh"
#include "main/object.hh"
#include "simplessd/sim/simplessd.hh"

namespace Standalone::Driver {

class AbstractInterface : public Object {
 protected:
  SimpleSSD::SimpleSSD &simplessd;

  Event completionEvent;

  void callCompletion(uint64_t id) {
    object.engine->invoke(completionEvent, id);
  }

 public:
  AbstractInterface(ObjectData &o, SimpleSSD::SimpleSSD &s)
      : Object(o), simplessd(s), completionEvent(InvalidEventID) {}
  virtual ~AbstractInterface() {}

  void setCompletionEvent(Event e) { completionEvent = e; }

  virtual void init(Event) = 0;
  virtual void getInfo(uint64_t &, uint32_t &) = 0;
  virtual void submitIO(BIO &) = 0;

  virtual void initStats(std::vector<SimpleSSD::Stat> &) = 0;
  virtual void getStats(std::vector<double> &) = 0;
};

}  // namespace Standalone::Driver

#endif
