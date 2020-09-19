// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "igl/request/request_generator.hh"

#include <iostream>

#include "simplessd/util/algorithm.hh"
#include "util/print.hh"

namespace Standalone::IGL {

RequestGenerator::RequestGenerator(ObjectData &o, BlockIOLayer &b, Event e)
    : AbstractIOGenerator(o, b, e),
      io_submitted(0),
      reserveTermination(false),
      io_depth(0),
      io_count(0),
      read_count(0) {
  // Read config
  io_size = readConfigUint(Section::RequestGenerator, RequestConfig::Key::Size);
  type = (RequestConfig::IOType)readConfigUint(Section::RequestGenerator,
                                               RequestConfig::Key::Type);
  mode = (RequestConfig::IOMode)readConfigUint(Section::RequestGenerator,
                                               RequestConfig::Key::Mode);
  iodepth =
      readConfigUint(Section::RequestGenerator, RequestConfig::Key::Depth);
  rwmixread =
      readConfigFloat(Section::RequestGenerator, RequestConfig::Key::ReadMix);
  offset =
      readConfigUint(Section::RequestGenerator, RequestConfig::Key::Offset);
  size = readConfigUint(Section::RequestGenerator, RequestConfig::Key::Limit);
  thinktime =
      readConfigUint(Section::RequestGenerator, RequestConfig::Key::ThinkTime);
  blocksize =
      readConfigUint(Section::RequestGenerator, RequestConfig::Key::BlockSize);
  blockalign =
      readConfigUint(Section::RequestGenerator, RequestConfig::Key::BlockAlign);
  randseed =
      readConfigUint(Section::RequestGenerator, RequestConfig::Key::RandomSeed);
  time_based = readConfigBoolean(Section::RequestGenerator,
                                 RequestConfig::Key::TimeBased);
  runtime =
      readConfigUint(Section::RequestGenerator, RequestConfig::Key::RunTime);

  if (blockalign == 0) {
    blockalign = blocksize;
  }

  if (mode == RequestConfig::IOMode::Synchronous) {
    iodepth = 1;
    mode = RequestConfig::IOMode::Asynchronous;
  }

  auto submissionLatency =
      readConfigUint(Section::Simulation, Config::Key::SubmissionLatency);
  auto completionLatency =
      readConfigUint(Section::Simulation, Config::Key::CompletionLatency);

  // Set random engine
  randengine.seed(randseed);

  submitEvent = createEvent([this](uint64_t t, uint64_t) { submitIO(t); },
                            "IGL::RequestGenerator::submitEvent");
  completionEvent = createEvent([this](uint64_t t, uint64_t) { iocallback(t); },
                                "IGL::RequestGenerator::completionEvent");

  bioEntry.initialize(iodepth, submissionLatency, completionLatency,
                      completionEvent);
}

RequestGenerator::~RequestGenerator() {}

void RequestGenerator::initialize() {
  uint64_t bytesize;
  uint32_t bs;

  bioEntry.getSSDSize(bytesize, bs);

  if (offset > bytesize) {
    panic("offset is larger than SSD size");
  }
  if (blocksize < bs) {
    panic("blocksize is smaller than SSD's logical block");
  }
  if (blockalign < bs) {
    panic("blockalign is smaller than SSD's logical block");
  }
  if (blocksize % bs != 0) {
    warn("blocksize is not aligned to SSD's logical block");

    blocksize /= bs;
    blocksize *= bs;
  }
  if (blockalign % bs != 0) {
    warn("blockalign is not aligned to SSD's logical block");

    blockalign /= bs;
    blockalign *= bs;
  }
  if (offset % blockalign != 0) {
    warn("offset is not aligned to blockalign");

    offset /= blockalign;
    offset *= blockalign;
  }
  if (size == 0 || offset + size > bytesize) {
    size = bytesize - offset;
  }
  if (size == 0) {
    panic("Invalid offset and size provided");
  }

  randgen = std::uniform_int_distribution<uint64_t>(offset,
                                                    offset + size - blocksize);
}

void RequestGenerator::begin() {
  initTime = getTick();

  scheduleAbs(submitEvent, 0ull, initTime);
}

void RequestGenerator::printStats(std::ostream &out) {
  uint64_t tick = getTick();

  out << "*** Statistics of Request Generator ***" << std::endl;
  out << "I/O mode: ";

  if (iodepth == 1) {
    out << "Sync, ";
  }
  else {
    out << "Async (depth: " << iodepth << "), ";
  }

  out << "I/O type: ";

  switch (type) {
    case RequestConfig::IOType::Read:
      out << "Seq. Read";
      break;
    case RequestConfig::IOType::Write:
      out << "Seq. Write";
      break;
    case RequestConfig::IOType::RandRead:
      out << "Rand. Read";
      break;
    case RequestConfig::IOType::RandWrite:
      out << "Rand. Write";
      break;
    case RequestConfig::IOType::ReadWrite:
      out << "Seq. R/W Mix";
      break;
    case RequestConfig::IOType::RandRW:
      out << "Rand. R/W Mix";
      break;
  }

  out << ", blocksize: " << blocksize << " bytes" << std::endl;
  out << "Termination mode: ";

  if (time_based) {
    out << "Time based (runtime: " << runtime / 1000000000 << " ms)"
        << std::endl;
  }
  else {
    out << "I/O size based (size: " << io_size << " bytes)" << std::endl;
  }

  out << "Tick: " << tick << std::endl;
  out << "Time (ps): " << initTime << " - " << tick << " (" << tick - initTime
      << ") (";

  printLatency(out, tick - initTime);

  out << ")" << std::endl;
  out << "I/O (bytes): " << io_submitted << " (";

  printBandwidth(out, io_submitted, tick - initTime);

  out << ")" << std::endl;
  out << "I/O (counts): " << io_count << " (Read: " << read_count
      << ", Write: " << io_count - read_count << ")" << std::endl;
  out << "*** End of statistics ***" << std::endl;

  bioEntry.printStats(out);
}

void RequestGenerator::getProgress(float &val) {
  bool zero = false;

  {
    std::lock_guard<std::mutex> guard(m);

    if (io_submitted == 0) {
      zero = true;
    }
  }

  if (zero) {
    val = 0.f;

    return;
  }

  if (time_based) {             // Read-only variable after init
    uint64_t tick = getTick();  // Thread-safe

    // initTime is read-only after begin() called
    val = (float)(tick - initTime) / runtime;
  }
  else {
    std::lock_guard<std::mutex> guard(m);

    val = (float)io_submitted / io_size;
  }
}

void RequestGenerator::generateAddress(uint64_t &off, uint64_t &len) {
  // This function generates address to access
  // based on I/O type, blocksize/align and offset/size
  if (type == RequestConfig::IOType::RandRead ||
      type == RequestConfig::IOType::RandWrite ||
      type == RequestConfig::IOType::RandRW) {
    off = randgen(randengine);  // randgen range: [offset, offset + size)
    off -= off % blockalign;
    len = blocksize;
  }
  else {
    off = io_count * blockalign;
    len = blocksize;

    // Limit range of address to [offset, offset + size)
    while (off + len > size) {
      if (off >= size) {
        off -= size;
      }
      else {
        // TODO: is this correct?
        off -= len;
      }
    }

    off += offset;
  }
}

bool RequestGenerator::nextIOIsRead() {
  // This function determine next I/O is read or write
  // based on rwmixread
  // io_count should not zero
  if (type == RequestConfig::IOType::ReadWrite ||
      type == RequestConfig::IOType::RandRW) {
    if (rwmixread > (float)read_count / io_count) {
      return true;
    }
  }
  else if (type == RequestConfig::IOType::Read ||
           type == RequestConfig::IOType::RandRead) {
    return true;
  }

  return false;
}

void RequestGenerator::submitIO(uint64_t now) {
  uint64_t offset, length;
  Driver::RequestType type;

  // This function uses io_count (=0 at very beginning)
  generateAddress(offset, length);

  io_count++;

  // This function also uses io_count (=1 at very beginning)
  if (nextIOIsRead()) {
    type = Driver::RequestType::Read;
    read_count++;
  }
  else {
    type = Driver::RequestType::Write;
  }

  io_submitted += length;

  // push to queue
  io_depth++;

  // Submit to Block I/O entry
  auto ret = bioEntry.submitRequest(type, offset, length);

  panic_if(!ret, "BUG!");

  // Check on-the-fly I/O depth
  if (io_depth < iodepth && !reserveTermination) {
    if (!time_based && io_submitted >= io_size) {
      reserveTermination = true;

      return;
    }

    scheduleAbs(submitEvent, 0ull, now);
  }
}

void RequestGenerator::iocallback(uint64_t now) {
  io_depth--;

  if (reserveTermination) {
    // No I/O will be generated anymore
    // If no pending I/O call endCallback
    if (io_depth == 0) {
      scheduleAbs(endCallback, 0ull, now);
    }
  }
  else {
    // Check completion
    if ((!time_based && io_submitted >= io_size) ||
        (time_based && runtime <= (now - initTime))) {
      reserveTermination = true;

      // We need to double-check this for following case:
      // _iocallback (all I/O completed) -> rescheduleSubmit
      if (io_depth == 0) {
        scheduleAbs(endCallback, 0ull, now);
      }

      return;
    }

    // Check I/O depth
    if (io_depth < iodepth && !isScheduled(submitEvent)) {
      scheduleAbs(submitEvent, 0ull, now + thinktime);
    }
  }
}

}  // namespace Standalone::IGL
