// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "driver/none/none.hh"

#include "simplessd/hil/none/controller.hh"

namespace Standalone::Driver::None {

NoneInterface::NoneInterface(ObjectData &o, SimpleSSD::SimpleSSD &s)
    : AbstractInterface(o, s), pHIL(nullptr) {
  auto id = simplessd.createController(this);
  auto controller = dynamic_cast<SimpleSSD::HIL::None::Controller *>(
      simplessd.getController(id));

  panic_if(controller == nullptr, "No controller?");

  pHIL = controller->getHIL();

  completionEvent = simplessd.getObject().cpu->createEvent(
      [this](uint64_t t, uint64_t d) { complete(t, d); },
      "Standalone::SIL::None::completionEvent");
}

NoneInterface::~NoneInterface() {}

void NoneInterface::init(Event eid) {
  info("Initialization finished");

  scheduleNow(eid);
}

void NoneInterface::getInfo(uint64_t &bytesize, uint32_t &minbs) {
  minbs = pHIL->getLPNSize();
  bytesize = pHIL->getTotalPages() * minbs;
}

void NoneInterface::submitIO(BIL::BIO &bio) {
  auto ret = requestQueue.emplace(
      bio.id, SimpleSSD::HIL::Request(completionEvent, bio.id));

  panic_if(!ret.second, "BIO ID conflict!");

  auto &request = ret.first->second;

  request.setAddress(bio.offset >> 9, bio.length >> 9, 512);
  request.setHostTag(bio.id);

  switch (bio.type) {
    case BIL::BIOType::Read:
      pHIL->read(&request);

      break;
    case BIL::BIOType::Write:
      pHIL->write(&request);

      break;
    case BIL::BIOType::Flush:
      pHIL->flush(&request);

      break;
    case BIL::BIOType::Trim:
      pHIL->format(&request, SimpleSSD::HIL::FormatOption::None);

      break;
    default:
      panic("Unexpected I/O type.");

      break;
  }
}

void NoneInterface::complete(uint64_t, uint64_t id) {
  auto iter = requestQueue.find(id);

  panic_if(iter == requestQueue.end(), "Unexpected completion.");

  callCompletion(id);

  requestQueue.erase(iter);
}

void NoneInterface::read(uint64_t, uint32_t, uint8_t *, SimpleSSD::Event,
                         uint64_t) {
  panic("Calling not implemented function.");
}

void NoneInterface::write(uint64_t, uint32_t, uint8_t *, SimpleSSD::Event,
                          uint64_t) {
  panic("Calling not implemented function.");
}

void NoneInterface::postInterrupt(uint16_t, bool) {
  panic("Calling not implemented function.");
}

void NoneInterface::getPCIID(uint16_t &, uint16_t &) {
  panic("Calling not implemented function.");
}

void NoneInterface::initStats(std::vector<SimpleSSD::Stat> &list) {
  simplessd.getStatList(list, "");
}

void NoneInterface::getStats(std::vector<double> &values) {
  simplessd.getStatValues(values);
}

}  // namespace Standalone::Driver::None
