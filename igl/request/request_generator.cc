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

#include "igl/request/request_generator.hh"

#include "simplessd/sim/trace.hh"
#include "simplessd/util/algorithm.hh"

namespace IGL {

RequestGenerator::RequestGenerator(Engine &e, BIL::BlockIOEntry &b,
                                   std::function<void()> &f, ConfigReader &c)
    : IOGenerator(e, b, f),
      io_submitted(0),
      io_count(0),
      read_count(0),
      reserveTermination(false) {
  // Read config
  io_size = c.readUint(CONFIG_REQ_GEN, REQUEST_IO_SIZE);
  type = (IO_TYPE)c.readUint(CONFIG_REQ_GEN, REQUEST_IO_TYPE);
  mode = (IO_MODE)c.readUint(CONFIG_REQ_GEN, REQUEST_IO_MODE);
  iodepth = c.readUint(CONFIG_REQ_GEN, REQUEST_IO_DEPTH);
  rwmixread = c.readFloat(CONFIG_REQ_GEN, REQUEST_IO_MIX_RATIO);
  offset = c.readUint(CONFIG_REQ_GEN, REQUEST_OFFSET);
  size = c.readUint(CONFIG_REQ_GEN, REQUEST_SIZE);
  thinktime = c.readUint(CONFIG_REQ_GEN, REQUEST_THINKTIME);
  blocksize = c.readUint(CONFIG_REQ_GEN, REQUEST_BLOCK_SIZE);
  blockalign = c.readUint(CONFIG_REQ_GEN, REQUEST_BLOCK_ALIGN);
  randseed = c.readUint(CONFIG_REQ_GEN, REQUEST_RANDOM_SEED);
  time_based = c.readBoolean(CONFIG_REQ_GEN, REQUEST_TIME_BASED);
  runtime = c.readUint(CONFIG_REQ_GEN, REQUEST_RUN_TIME);

  if (blockalign == 0) {
    blockalign = blocksize;
  }

  if (mode == IO_SYNC) {
    iodepth = 1;
    mode = IO_ASYNC;
  }

  asyncBreak = c.readUint(CONFIG_GLOBAL, GLOBAL_BREAK_ASYNC);
  syncBreak = c.readUint(CONFIG_GLOBAL, GLOBAL_BREAK_SYNC);

  // Set random engine
  randengine.seed(randseed);

  submitIO = [this](uint64_t tick) { _submitIO(tick); };
  iocallback = [this](uint64_t id) { _iocallback(id); };

  submitEvent = engine.allocateEvent(submitIO);
}

RequestGenerator::~RequestGenerator() {}

void RequestGenerator::init(uint64_t bytesize, uint32_t bs) {
  ssdSize = bytesize;
  ssdBlocksize = bs;

  if (offset > ssdSize) {
    SimpleSSD::panic("offset is larger than SSD size");
  }
  if (size == 0 || offset + size > ssdSize) {
    size = ssdSize - offset;
  }
  if (size == 0) {
    SimpleSSD::panic("Invalid offset and size provided");
  }

  randgen = std::uniform_int_distribution<uint64_t>(offset, offset + size);
}

void RequestGenerator::begin() {
  initTime = engine.getCurrentTick();

  _submitIO(initTime);
}

void RequestGenerator::printStats() {
  uint64_t tick = engine.getCurrentTick();

  SimpleSSD::info("*** Statistics of Request Generator ***");
  SimpleSSD::info("Tick: %" PRIu64, tick);
  SimpleSSD::info("Time (ps): %" PRIu64 " - %" PRIu64 " (%" PRIu64 ")",
                  initTime, tick, tick - initTime);
  SimpleSSD::info(
      "I/O (bytes): %" PRIu64 " (%lf B/s)", io_submitted,
      (double)io_submitted / (tick - initTime) * 1000000000000.);
  SimpleSSD::info("I/O (counts): %" PRIu64 " (Read: %" PRIu64
                  ", Write: %" PRIu64 ")",
                  io_count, read_count, io_count - read_count);
  SimpleSSD::info("*** End of statistics ***");
}

void RequestGenerator::generateAddress(uint64_t &off, uint64_t &len) {
  // This function generates address to access
  // based on I/O type, blocksize/align and offset/size
  if (type == IO_RANDREAD || type == IO_RANDWRITE || type == IO_RANDRW) {
    off = randgen(randengine);
    off -= off % blockalign;
    len = blocksize;
  }
  else {
    off = io_count * blockalign;
    len = blocksize;
  }

  while (off + len > ssdSize) {
    if (off >= ssdSize) {
      off -= ssdSize;
    }
    else {
      off -= len;
    }
  }
}

bool RequestGenerator::nextIOIsRead() {
  // This function determine next I/O is read or write
  // based on rwmixread
  // io_count should not zero
  if (type == IO_READWRITE || type == IO_RANDRW) {
    if (rwmixread > (float)read_count / io_count) {
      return true;
    }
  }
  else if (type == IO_READ || type == IO_RANDREAD) {
    return true;
  }

  return false;
}

void RequestGenerator::_submitIO(uint64_t) {
  BIL::BIO bio;

  // This function uses io_count (=0 at very beginning)
  generateAddress(bio.offset, bio.length);

  bio.id = io_count++;

  // This function also uses io_count (=1 at very beginning)
  if (nextIOIsRead()) {
    bio.type = BIL::BIO_READ;
    read_count++;
  }
  else {
    bio.type = BIL::BIO_WRITE;
  }

  io_submitted += bio.length;

  bio.callback = iocallback;

  // push to queue
  bioList.push_back(bio);

  // Check on-the-fly I/O depth
  rescheduleSubmit(asyncBreak);

  // Submit to Block I/O entry
  bioEntry.submitIO(bio);
}

void RequestGenerator::_iocallback(uint64_t id) {
  // Find ID from list
  for (auto iter = bioList.begin(); iter != bioList.end(); iter++) {
    if (iter->id == id) {
      // This request is finished
      bioList.erase(iter);

      break;
    }
  }

  if (reserveTermination) {
    // No I/O will be generated anymore
    // If no pending I/O call endCallback
    if (bioList.size() == 0) {
      endCallback();
    }
  }
  else {
    // Check on-the-fly I/O depth
    rescheduleSubmit(syncBreak);
  }
}

void RequestGenerator::rescheduleSubmit(uint64_t breakTime) {
  uint64_t tick = engine.getCurrentTick();

  // We are done
  if ((!time_based && io_submitted >= io_size) ||
      (time_based && runtime > (tick - initTime))) {
    reserveTermination = true;

    return;
  }

  if (bioList.size() < iodepth) {
    uint64_t scheduledTick;
    bool doSchedule = true;

    // Check conflict
    if (engine.isScheduled(submitEvent, &scheduledTick)) {
      if (scheduledTick >= tick + breakTime) {
        doSchedule = false;
      }
    }

    // We can schedule it
    if (doSchedule) {
      engine.scheduleEvent(submitEvent, tick + breakTime);
    }
  }
}

}  // namespace IGL
