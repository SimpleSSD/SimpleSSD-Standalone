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

namespace Standalone {

namespace IGL {

class BlockIOLayer;

}

namespace Driver {

enum class RequestType : uint8_t {
  None,
  Read,
  Write,
  Flush,
  Trim,
};

class Request {
 public:
  const RequestType type;

  const uint16_t tag;

  const uint32_t length;  // LBA length
  const uint64_t offset;  // LBA offset

  uint64_t submittedAt;
  uint64_t completedAt;

 private:
  void *driverData;

 public:
  Request()
      : type(RequestType::None),
        tag(0),
        length(0),
        offset(0),
        submittedAt(0),
        completedAt(0),
        driverData(nullptr) {}
  Request(uint64_t t, uint64_t slba, uint32_t nlb, RequestType op)
      : type(op),
        tag(t),
        length(nlb),
        offset(slba),
        submittedAt(0),
        completedAt(0),
        driverData(nullptr) {}

  inline void setDriverData(void *p) noexcept { driverData = p; }
  inline void *getDriverData() noexcept { return driverData; }
};

class AbstractInterface : public Object {
 protected:
  IGL::BlockIOLayer *parent;
  SimpleSSD::SimpleSSD &simplessd;

 public:
  AbstractInterface(ObjectData &o, SimpleSSD::SimpleSSD &s)
      : Object(o), simplessd(s) {}
  virtual ~AbstractInterface() {}

  /**
   * \brief Initialize driver and underlying SSD
   *
   * \param[in] p   Pointer to IGL::BlockIOLayer
   * \param[in] cb  Callback which will be called when initialization completed.
   */
  virtual void initialize(IGL::BlockIOLayer *p, Event cb) {
    (void)cb;
    parent = p;
  }

  /**
   * \brief Get SSD size information
   *
   * \param[out] size Byte size of SSD
   * \param[out] bs   Byte size of logical block
   */
  virtual void getSSDInfo(uint64_t &size, uint32_t &bs) = 0;

  /**
   * \brief Submit request
   *
   * You can assign driver-specific data to reqeust
   *
   * \param[in] req Request structure
   */
  virtual void submit(Request &req) = 0;

  virtual void getStatList(std::vector<SimpleSSD::Stat> &) = 0;
  virtual void getStatValues(std::vector<double> &) = 0;
};

}  // namespace Driver

}  // namespace Standalone

#endif
