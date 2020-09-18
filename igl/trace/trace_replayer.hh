// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __IGL_TRACE_REPLAYER_HH__
#define __IGL_TRACE_REPLAYER_HH__

#include <fstream>
#include <list>
#include <mutex>
#include <regex>
#include <thread>

#include "igl/abstract_io_generator.hh"

namespace Standalone::IGL {

class TraceReplayer : public AbstractIOGenerator {
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

  bool timeValids[5];
  bool useHex;
  bool useLBAOffset;
  bool useLBALength;

  uint32_t lbaSize;
  uint32_t maxQueueDepth;  // Only used in MODE_ASYNC
  uint32_t groupID[ID_NUM];

  uint64_t ssdSize;
  uint32_t blocksize;

  bool reserveTermination;
  bool forceSubmit;  // Only used in MODE_ASYNC

  uint64_t initTime;   // Only used in MODE_STRICT
  uint64_t firstTick;  // Only used in MODE_STRICT

  uint64_t max_io;
  uint64_t io_submitted;  // Submitted I/O in bytes
  uint64_t io_count;      // I/O count created and submitted
  uint64_t read_count;
  uint64_t write_count;

  uint64_t io_depth;

  uint64_t mergeTime(std::smatch &);
  Driver::RequestType getType(std::string);
  void parseLine();
  void rescheduleSubmit();

  struct TraceLine {
    uint64_t tick;
    uint64_t offset;
    uint64_t length;
    Driver::RequestType type;

    TraceLine()
        : tick(0), offset(0), length(0), type(Driver::RequestType::None) {}
  } linedata;

  Event submitEvent;
  Event completionEvent;

  void submitIO();
  void iocallback(uint64_t, uint64_t);

 public:
  TraceReplayer(ObjectData &, BlockIOLayer &, Event);
  ~TraceReplayer();

  void initialize(uint64_t, uint32_t) override;

  void begin() override;

  void printStats(std::ostream &) override;
  void getProgress(float &) override;
};

}  // namespace Standalone::IGL

#endif
