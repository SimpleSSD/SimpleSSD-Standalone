// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __MAIN_CONFIG_HH__
#define __MAIN_CONFIG_HH__

#include "simplessd/sim/base_config.hh"

namespace Standalone {

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

 private:
  ModeType mode;
  uint64_t statPeriod;
  std::string outputDirectory;
  std::string statFile;
  std::string debugFile;
  std::string latencyFile;
  uint64_t progressPeriod;
  InterfaceType interface;
  uint64_t submissionLatency;
  uint64_t completionLatency;

 public:
  Config();
  ~Config();

  const char *getSectionName() noexcept override { return "sim"; }

  void loadFrom(pugi::xml_node &) noexcept override;
  void storeTo(pugi::xml_node &) noexcept override;
  void update() noexcept override;

  uint64_t readUint(uint32_t) const noexcept override;
  std::string readString(uint32_t) const noexcept override;
  bool writeUint(uint32_t, uint64_t) noexcept override;
  bool writeString(uint32_t, std::string &) noexcept override;
};

}  // namespace Standalone

#endif
