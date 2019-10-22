// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __IGL_TRACE_REPLAYER__
#define __IGL_TRACE_REPLAYER__

#include <fstream>
#include <list>
#include <mutex>
#include <regex>
#include <thread>

#include "bil/entry.hh"
#include "igl/io_gen.hh"

namespace Standalone::IGL {

class TraceReplayer : public IOGenerator {
 private:
  enum {
    ID_OPERATION,
    ID_BYTE_OFFSET,
    ID_BYTE_LENGTH,
    ID_LBA_OFFSET,
    ID_LBA_LENGTH,
    ID_TIME_SEC,
    ID_TIME_MS,
    ID_TIME_US,
    ID_TIME_NS,
    ID_TIME_PS,
    ID_NUM
  };

  std::mutex m;

  std::ifstream file;
  std::regex regex;

  uint64_t fileSize;

  TraceConfig::TimingModeType mode;
  uint64_t submissionLatency;
  uint64_t completionLatency;
  uint32_t maxQueueDepth;  // Only used in MODE_ASYNC

  bool useLBAOffset;
  bool useLBALength;
  uint32_t lbaSize;
  uint32_t groupID[ID_NUM];
  bool timeValids[5];
  bool useHex;

  uint64_t ssdSize;
  uint32_t blocksize;

  uint64_t initTime;   // Only used in MODE_STRICT
  uint64_t firstTick;  // Only used in MODE_STRICT
  bool nextIOIsSync;   // Only used in MODE_ASYNC

  bool reserveTermination;

  uint64_t max_io;
  uint64_t io_submitted;  // Submitted I/O in bytes
  uint64_t io_count;      // I/O count created and submitted
  uint64_t read_count;
  uint64_t write_count;

  uint64_t io_depth;

  BIL::BIO bio;

  uint64_t mergeTime(std::smatch &);
  BIL::BIOType getType(std::string);
  void handleNextLine(bool = false);
  void rescheduleSubmit(uint64_t);

  Event submitEvent;
  Event completionEvent;

  void submitIO();
  void iocallback(uint64_t, uint64_t);

 public:
  TraceReplayer(ObjectData &, BIL::BlockIOEntry &, Event);
  ~TraceReplayer();

  void init(uint64_t, uint32_t) override;
  void begin() override;
  void printStats(std::ostream &) override;
  void getProgress(float &) override;
};

}  // namespace Standalone::IGL

#endif
