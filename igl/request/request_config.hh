// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __IGL_REQUEST_CONFIG_HH__
#define __IGL_REQUEST_CONFIG_HH__

#include "simplessd/sim/base_config.hh"

namespace Standalone::IGL {

class RequestConfig : public SimpleSSD::BaseConfig {
 public:
  enum Key : uint32_t {
    Size,
    Type,
    ReadMix,
    BlockSize,
    BlockAlign,
    Mode,
    Depth,
    Offset,
    Limit,
    ThinkTime,
    RandomSeed,
    TimeBased,
    RunTime,
  };

  enum class IOType : uint8_t {
    Read,
    Write,
    RandRead,
    RandWrite,
    ReadWrite,
    RandRW,
  };

  enum class IOMode : uint8_t {
    Synchronous,
    Asynchronous,
  };

 private:
  uint64_t io_size;
  std::string _type;
  IOType type;
  float rwmixread;
  uint64_t blocksize;
  uint64_t blockalign;
  std::string _mode;
  IOMode mode;
  uint64_t iodepth;
  uint64_t offset;
  uint64_t size;
  uint64_t thinktime;
  uint64_t randseed;
  bool time_based;
  uint64_t runtime;

 public:
  RequestConfig();

  const char *getSectionName() noexcept override { return "generator"; }

  void loadFrom(pugi::xml_node &) noexcept override;
  void storeTo(pugi::xml_node &) noexcept override;
  void update() noexcept override;

  uint64_t readUint(uint32_t) const noexcept override;
  float readFloat(uint32_t) const noexcept override;
  bool readBoolean(uint32_t) const noexcept override;
  bool writeUint(uint32_t, uint64_t) noexcept override;
  bool writeFloat(uint32_t, float) noexcept override;
  bool writeBoolean(uint32_t, bool) noexcept override;
};

}  // namespace Standalone::IGL

#endif
