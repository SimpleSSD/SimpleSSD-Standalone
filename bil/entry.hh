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

#include "sim/cfg_reader.hh"
#include "sim/engine.hh"

namespace BIL {

class Scheduler;
class DriverInterface;

enum BIO_TYPE : uint8_t {
  BIO_READ,
  BIO_WRITE,
  BIO_FLUSH,
  BIO_TRIM,
  BIO_NUM,
};

typedef struct _BIO {
  uint64_t id;

  // I/O definition
  BIO_TYPE type;
  uint64_t offset;
  uint64_t length;

  // I/O completion
  std::function<void(uint64_t)> callback;

  // Statistics
  uint64_t submittedAt;

  _BIO() : id(0), type(BIO_READ), offset(0), length(0), submittedAt(0) {}
} BIO;

typedef struct _Progress {
  uint64_t iops;
  uint64_t bandwidth;
  uint64_t latency;
} Progress;

class BlockIOEntry {
 private:
  ConfigReader &conf;
  Engine &engine;
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

  std::function<void(uint64_t)> callback;
  void completion(uint64_t);

 public:
  BlockIOEntry(ConfigReader &, Engine &, DriverInterface *, std::ostream *);
  ~BlockIOEntry();

  void submitIO(BIO &);

  void printStats(std::ostream &);
  void getProgress(Progress &);
};

}  // namespace BIL

#endif
