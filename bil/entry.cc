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

#include "bil/entry.hh"

#include "bil/interface.hh"
#include "bil/noop_scheduler.hh"
#include "simplessd/sim/trace.hh"

namespace BIL {

BlockIOEntry::BlockIOEntry(ConfigReader &c, Engine &e, DriverInterface *i)
    : conf(c),
      engine(e),
      pScheduler(nullptr),
      pDriver(i),
      callback([this](uint64_t id) { completion(id); }) {
  switch (c.readUint(CONFIG_GLOBAL, GLOBAL_SCHEDULER)) {
    case SCHEDULER_NOOP:
      pScheduler = new NoopScheduler(e, i);

      break;
    default:
      SimpleSSD::panic("Invalid I/O scheduler specified");

      break;
  }

  pScheduler->init();
}

BlockIOEntry::~BlockIOEntry() {
  delete pScheduler;
}

void BlockIOEntry::submitIO(BIO &bio) {
  BIO copy(bio);

  ioQueue.push_back(bio);
  copy.callback = callback;

  pScheduler->submitIO(copy);
}

void BlockIOEntry::completion(uint64_t id) {
  for (auto iter = ioQueue.begin(); iter != ioQueue.end(); iter++) {
    if (iter->id == id) {
      iter->callback(id);

      ioQueue.erase(iter);

      break;
    }
  }
}

}  // namespace BIL
