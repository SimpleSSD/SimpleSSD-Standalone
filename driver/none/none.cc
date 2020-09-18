// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "driver/none/none.hh"

#include "igl/block_io.hh"
#include "simplessd/hil/none/controller.hh"

namespace Standalone::Driver::None {

NoneInterface::NoneInterface(ObjectData &o, SimpleSSD::SimpleSSD &s)
    : AbstractInterface(o, s), pHIL(nullptr) {
  auto id = simplessd.createController(this);
  auto controller = dynamic_cast<SimpleSSD::HIL::None::Controller *>(
      simplessd.getController(id));

  panic_if(controller == nullptr, "No controller?");

  pHIL = controller->getHIL();

  lpnSize = pHIL->getLPNSize();

  completionEvent = simplessd.getObject().cpu->createEvent(
      [this](uint64_t t, uint64_t d) { complete(t, d); },
      "Standalone::SIL::None::completionEvent");
}

NoneInterface::~NoneInterface() {}

void NoneInterface::initialize(IGL::BlockIOLayer *p, Event eid) {
  AbstractInterface::initialize(p, eid);

  info("Initialization finished");

  scheduleNow(eid);
}

void NoneInterface::getSSDInfo(uint64_t &bytesize, uint32_t &minbs) {
  minbs = lpnSize;
  bytesize = pHIL->getTotalPages() * lpnSize;
}

void NoneInterface::submit(Request &req) {
  auto request = new SimpleSSD::HIL::Request(completionEvent, req.tag);

  req.setDriverData(request);

  request->setAddress(req.offset, req.length, lpnSize);
  request->setHostTag(req.tag);

  switch (req.type) {
    case RequestType::Read:
      pHIL->read(request);

      break;
    case RequestType::Write:
      pHIL->write(request);

      break;
    case RequestType::Flush:
      pHIL->flush(request);

      break;
    case RequestType::Trim:
      pHIL->format(request, SimpleSSD::HIL::FormatOption::None);

      break;
    default:
      panic("Unexpected I/O type.");

      break;
  }
}

void NoneInterface::complete(uint64_t, uint64_t id) {
  auto request = (SimpleSSD::HIL::Request *)parent->postCompletion(id);

  delete request;
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

void NoneInterface::getStatList(std::vector<SimpleSSD::Stat> &list) {
  simplessd.getStatList(list, "");
}

void NoneInterface::getStatValues(std::vector<double> &values) {
  simplessd.getStatValues(values);
}

}  // namespace Standalone::Driver::None
