// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "sil/none/none.hh"

#include "simplessd/util/algorithm.hh"

namespace SIL {

namespace None {

Driver::Driver(Engine &e, SimpleSSD::ConfigReader &conf)
    : BIL::DriverInterface(e), totalLogicalPages(0), logicalPageSize(0) {
  pHIL = new SimpleSSD::HIL::HIL(conf);
}

Driver::~Driver() {
  delete pHIL;
}

void Driver::init(std::function<void()> &func) {
  pHIL->getLPNInfo(totalLogicalPages, logicalPageSize);

  // No initialization process is needed for NoneDriver
  beginFunction = func;

  auto eid = engine.allocateEvent([this](uint64_t) { beginFunction(); });
  engine.scheduleEvent(eid, 0);

  SimpleSSD::info("SIL::None::Driver: Total SSD capacity: %" PRIu64 " bytes",
                  totalLogicalPages * logicalPageSize);
  SimpleSSD::info("SIL::None::Driver: Logical Page Size: %" PRIu32 " bytes",
                  logicalPageSize);
}

void Driver::getInfo(uint64_t &bytesize, uint32_t &minbs) {
  bytesize = totalLogicalPages * logicalPageSize;
  minbs = 512;
}

void Driver::submitIO(BIL::BIO &bio) {
  SimpleSSD::HIL::Request req;
  auto *pFunc = new std::function<void(uint64_t)>(bio.callback);

  // Convert to request
  req.reqID = bio.id;
  req.range.slpn = bio.offset / logicalPageSize;
  req.range.nlp = DIVCEIL(bio.length, logicalPageSize);
  req.offset = bio.offset % logicalPageSize;
  req.length = bio.length;
  req.context = (void *)bio.id;
  req.function = [pFunc](uint64_t, void *context) {
    pFunc->operator()((uint64_t)context);
    delete pFunc;
  };

  // Submit
  switch (bio.type) {
    case BIL::BIO_READ:
      pHIL->read(req);
      break;
    case BIL::BIO_WRITE:
      pHIL->write(req);
      break;
    case BIL::BIO_FLUSH:
      pHIL->flush(req);
      break;
    case BIL::BIO_TRIM:
      pHIL->trim(req);
      break;
    default:
      break;
  }
}

void Driver::initStats(std::vector<SimpleSSD::Stats> &list) {
  pHIL->getStatList(list, "");
  SimpleSSD::getCPUStatList(list, "cpu");
}

void Driver::getStats(std::vector<double> &values) {
  pHIL->getStatValues(values);
  SimpleSSD::getCPUStatValues(values);
}

}  // namespace None

}  // namespace SIL
