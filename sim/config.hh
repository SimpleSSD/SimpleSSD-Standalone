// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __SIM_CONFIG_HH__
#define __SIM_CONFIG_HH__

#include "simplessd/sim/base_config.hh"

/**
 * \brief SimConfig object declaration
 *
 * Stores simulation configurations such as simulation output directory.
 */
class Config : public SimpleSSD::BaseConfig {
 public:
  enum Key : uint32_t {
    Mode,
    StatPeriod,
    OutputDirectory,
    StatFile,
    DebugFile,
    Latencyfile,
    ProgressPeriod,
    Interface,
    Scheduler,
    SubmissionLatency,
    CompletionLatency,
  };

  enum class ModeType : uint8_t {
    RequestGenerator,
    TraceReplayer,
  };

  enum class InterfaceType : uint8_t {
    None,
    NVMe,
  };

  enum class SchedulerType : uint8_t {
    Noop,
  };

 private:
  ModeType mode;
  uint64_t statPeriod;
  std::string outputDirectory;
  std::string statFile;
  std::string debugFile;
  std::string latencyFile;
  uint64_t progressPeriod;
  InterfaceType interface;
  SchedulerType scheduler;
  uint64_t submissionLatency;
  uint64_t completionLatency;

 public:
  Config();
  ~Config();

  const char *getSectionName() override { return "sim"; }

  void loadFrom(pugi::xml_node &) override;
  void storeTo(pugi::xml_node &) override;
  void update() override;

  uint64_t readUint(uint32_t) override;
  std::string readString(uint32_t) override;
  bool writeUint(uint32_t, uint64_t) override;
  bool writeString(uint32_t, std::string &) override;
};

#endif