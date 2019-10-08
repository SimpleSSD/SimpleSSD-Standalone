// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __SIM_GLOBAL_CONFIG__
#define __SIM_GLOBAL_CONFIG__

#include "simplessd/sim/base_config.hh"

typedef enum {
  GLOBAL_SIM_MODE,
  GLOBAL_LOG_PERIOD,
  GLOBAL_LOG_FILE,
  GLOBAL_DEBUG_LOG_FILE,
  GLOBAL_LATENCY_LOG_FILE,
  GLOBAL_PROGRESS_PERIOD,
  GLOBAL_INTERFACE,
  GLOBAL_SCHEDULER,
  GLOBAL_SUBMISSION_LATENCY,
  GLOBAL_COMPLETION_LATENCY,
} GLOBAL_CONFIG;

typedef enum {
  MODE_REQUEST_GENERATOR,
  MODE_TRACE_REPLAYER,
  MODE_NUM,
} SIM_MODE;

typedef enum {
  INTERFACE_NONE,
  INTERFACE_NVME,
  INTERFACE_SATA,
  INTERFACE_UFS,
  INTERFACE_NUM,
} INTERFACE;

typedef enum {
  SCHEDULER_NOOP,
  SCHEDULER_NUM,
} SCHEDULER;

class Config : public SimpleSSD::BaseConfig {
 private:
  SIM_MODE mode;
  uint64_t logPeriod;
  std::string logFile;
  std::string logDebugFile;
  std::string latencyFile;
  uint64_t progressPeriod;
  INTERFACE interface;
  SCHEDULER scheduler;
  uint64_t submissionLatency;
  uint64_t completionLatency;

 public:
  Config();

  bool setConfig(const char *, const char *) override;
  void update() override;

  uint64_t readUint(uint32_t) override;
  std::string readString(uint32_t) override;
};

#endif
