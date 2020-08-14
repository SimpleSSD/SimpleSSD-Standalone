// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "bil/entry.hh"

#include <cmath>

#include "bil/interface.hh"
#include "bil/noop_scheduler.hh"

namespace Standalone::BIL {

BlockIOEntry::BlockIOEntry(ObjectData &o, DriverInterface *i, std::ostream *f)
    : Object(o),
      pLatencyFile(f),
      pScheduler(nullptr),
      pDriver(i),
      lastProgress(0),
      io_progress(0),
      io_count(0),
      minLatency(std::numeric_limits<uint64_t>::max()),
      maxLatency(0),
      sumLatency(0),
      squareSumLatency(0),
      iglCallback(InvalidEventID) {
  switch ((Config::SchedulerType)readConfigUint(Section::Simulation,
                                                Config::Key::Scheduler)) {
    case Config::SchedulerType::Noop:
      pScheduler = new NoopScheduler(o, pDriver);

      break;
    default:
      panic("Invalid I/O scheduler specified");

      break;
  }

  callback = createEvent([this](uint64_t t, uint64_t d) { completion(t, d); },
                         "BIL::BlockIOEntry::callback");

  pScheduler->init();
  pDriver->setCompletionEvent(callback);
}

BlockIOEntry::~BlockIOEntry() {
  delete pScheduler;
}

void BlockIOEntry::registerCallback(Event e) {
  iglCallback = e;
}

void BlockIOEntry::submitIO(BIO &bio) {
  BIO copy(bio);

  io_count++;
  bio.submittedAt = getTick();

  ioQueue.push_back(bio);

  pScheduler->submitIO(copy);
}

void BlockIOEntry::completion(uint64_t now, uint64_t id) {
  uint64_t tick = now;

  for (auto iter = ioQueue.begin(); iter != ioQueue.end(); iter++) {
    if (iter->id == id) {
      tick = tick - iter->submittedAt;

      {
        std::lock_guard<std::mutex> guard(m);

        io_progress++;

        progress.latency += tick;
        progress.iops++;
        progress.bandwidth += iter->length;
      }

      if (pLatencyFile) {
        *pLatencyFile << std::to_string(now) << ", "
                      << std::to_string(iter->offset) << ", "
                      << std::to_string(iter->length) << ", "
                      << std::to_string(tick) << std::endl;
      }

      // TODO: Fix me!
      object.engine->invoke(iglCallback, id);

      ioQueue.erase(iter);

      break;
    }
  }

  if (minLatency > tick) {
    minLatency = tick;
  }
  if (maxLatency < tick) {
    maxLatency = tick;
  }

  sumLatency += tick;
  squareSumLatency += tick * tick;
}

void BlockIOEntry::printStats(std::ostream &out) {
  double avgLatency = (double)sumLatency / io_count;
  double stdevLatency = sqrt((double)squareSumLatency / io_count - avgLatency);
  double digit = log10(avgLatency);

  out << "*** Statistics of Block I/O Entry ***" << std::endl;

  if (digit < 6.0) {
    out << "Latency (ps): min=" << std::to_string(minLatency)
        << ", max=" << std::to_string(maxLatency)
        << ", avg=" << std::to_string(avgLatency)
        << ", stdev=" << std::to_string(stdevLatency) << std::endl;
  }
  else if (digit < 9.0) {
    out << "Latency (ns): min=" << std::to_string(minLatency / 1000.0)
        << ", max=" << std::to_string(maxLatency / 1000.0)
        << ", avg=" << std::to_string(avgLatency / 1000.0)
        << ", stdev=" << std::to_string(stdevLatency / 31.62277660168)
        << std::endl;
  }
  else if (digit < 12.0) {
    out << "Latency (us): min=" << std::to_string(minLatency / 1000000.0)
        << ", max=" << std::to_string(maxLatency / 1000000.0)
        << ", avg=" << std::to_string(avgLatency / 1000000.0)
        << ", stdev=" << std::to_string(stdevLatency / 1000.0) << std::endl;
  }
  else {
    out << "Latency (ms): min=" << std::to_string(minLatency / 1000000000.0)
        << ", max=" << std::to_string(maxLatency / 1000000000.0)
        << ", avg=" << std::to_string(avgLatency / 1000000000.0)
        << ", stdev=" << std::to_string(stdevLatency / 31622.77660168379)
        << std::endl;
  }

  out << "*** End of statistics ***" << std::endl;
}

void BlockIOEntry::getProgress(Progress &data) {
  uint64_t tick = getTick();
  uint64_t diff = tick - lastProgress;

  if (diff == 0) {
    data.iops = 0;
    data.bandwidth = 0;
    data.latency = 0;

    return;
  }

  double ratio = 1000000000000.0 / diff;

  {
    std::lock_guard<std::mutex> guard(m);

    data.iops = (uint64_t)(progress.iops * ratio);
    data.bandwidth = (uint64_t)(progress.bandwidth * ratio);

    if (io_progress == 0) {
      data.latency = 0;
    }
    else {
      data.latency = progress.latency / io_progress;
    }

    io_progress = 0;
    progress.iops = 0;
    progress.bandwidth = 0;
    progress.latency = 0;
  }

  lastProgress = tick;
}

}  // namespace Standalone::BIL
