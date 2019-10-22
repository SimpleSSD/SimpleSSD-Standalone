// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __BIL_ENTRY__
#define __BIL_ENTRY__

#include <cinttypes>
#include <fstream>
#include <functional>
#include <list>

#include "sim/object.hh"

namespace Standalone::BIL {

class Scheduler;
class DriverInterface;

enum class BIOType : uint8_t {
  Read,
  Write,
  Flush,
  Trim,
};

typedef struct _BIO {
  uint64_t id;

  // I/O definition
  BIOType type;
  uint64_t offset;
  uint64_t length;

  // I/O completion
  Event callback;

  // Statistics
  uint64_t submittedAt;

  _BIO()
      : id(0),
        type(BIOType::Read),
        offset(0),
        length(0),
        submittedAt(0),
        callback(InvalidEventID) {}
} BIO;

typedef struct _Progress {
  uint64_t iops;
  uint64_t bandwidth;
  uint64_t latency;
} Progress;

class BlockIOEntry : public Object {
 private:
  std::list<BIO> ioQueue;

  std::ostream *pLatencyFile;

  Scheduler *pScheduler;
  DriverInterface *pDriver;

  std::mutex m;
  uint64_t lastProgress;
  uint64_t io_progress;
  Progress progress;

  // Statistics
  uint64_t io_count;
  uint64_t minLatency;
  uint64_t maxLatency;
  uint64_t sumLatency;
  uint64_t squareSumLatency;

  Event callback;
  void completion(uint64_t, uint64_t);

 public:
  BlockIOEntry(ObjectData &, DriverInterface *, std::ostream *);
  ~BlockIOEntry();

  void submitIO(BIO &);

  void printStats(std::ostream &);
  void getProgress(Progress &);
};

}  // namespace Standalone::BIL

#endif
