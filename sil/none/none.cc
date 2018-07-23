/*
 * Copyright (C) 2017 CAMELab
 *
 * This file is part of SimpleSSD.
 *
 * SimpleSSD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSSD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimpleSSD.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sil/none/none.hh"

namespace SIL {

NoneDriver::NoneDriver(Engine &e, SimpleSSD::ConfigReader &conf)
    : BIL::DriverInterface(e), totalLogicalPages(0), logicalPageSize(0) {
  pHIL = new SimpleSSD::HIL::HIL(conf);
}

NoneDriver::~NoneDriver() {
  delete pHIL;
}

void NoneDriver::init() {
  pHIL->getLPNInfo(totalLogicalPages, logicalPageSize);
}

void NoneDriver::submitIO(BIL::BIO &bio) {
  SimpleSSD::HIL::Request req;
  auto *pFunc = new std::function<void(uint64_t)>(bio.callback);

  // Convert to request
  req.reqID = bio.id;
  req.range.slpn = bio.offset / logicalPageSize;
  req.range.nlp = bio.length / logicalPageSize;
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
  }
}

}  // namespace SIL
