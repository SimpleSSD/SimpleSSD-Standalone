// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __DRIVER_NONE_HH__
#define __DRIVER_NONE_HH__

#include <unordered_map>

#include "driver/abstract_interface.hh"
#include "simplessd/hil/hil.hh"

namespace Standalone::Driver::None {

class NoneInterface : public AbstractInterface, SimpleSSD::Interface {
 private:
  SimpleSSD::HIL::HIL *pHIL;

  std::unordered_map<uint64_t, SimpleSSD::HIL::Request> requestQueue;

  SimpleSSD::Event completionEvent;

  void complete(uint64_t, uint64_t);

 public:
  NoneInterface(ObjectData &, SimpleSSD::SimpleSSD &);
  ~NoneInterface();

  // BIL::Interface
  void init(Event) override;
  void getInfo(uint64_t &, uint32_t &) override;
  void submitIO(BIL::BIO &) override;

  // SimpleSSD::Interface
  void read(uint64_t, uint32_t, uint8_t *, SimpleSSD::Event,
            uint64_t = 0) override;
  void write(uint64_t, uint32_t, uint8_t *, SimpleSSD::Event,
             uint64_t = 0) override;

  void postInterrupt(uint16_t, bool) override;
  void getPCIID(uint16_t &, uint16_t &) override;

  void initStats(std::vector<SimpleSSD::Stat> &) override;
  void getStats(std::vector<double> &) override;
};

}  // namespace Standalone::Driver::None

#endif
