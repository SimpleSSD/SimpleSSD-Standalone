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

#include "main/object.hh"
#include "simplessd/sim/simplessd.hh"

namespace Standalone::Driver {

enum class RequestType : uint8_t {
  None,
  Read,
  Write,
  Flush,
  Trim,
};

class Request {
 public:
  const uint64_t tag;

  const uint64_t offset;
  const uint32_t length;
  const RequestType type;

 private:
  void *driverData;

 public:
  Request()
      : tag(0),
        offset(0),
        length(0),
        type(RequestType::None),
        driverData(nullptr) {}
  Request(uint64_t t, uint64_t slba, uint32_t nlb, RequestType op)
      : tag(t), offset(slba), length(nlb), type(op), driverData(nullptr) {}

  inline void setDriverData(void *p) noexcept { driverData = p; }
  inline void *getDriverData() noexcept { return driverData; }
};

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

  virtual void submit(Request &) = 0;

  virtual void getStatList(std::vector<SimpleSSD::Stat> &) = 0;
  virtual void getStatValues(std::vector<double> &) = 0;
};

}  // namespace Standalone::Driver

#endif
