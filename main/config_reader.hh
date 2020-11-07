// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __MAIN_CONFIG_READER_HH__
#define __MAIN_CONFIG_READER_HH__

#include <functional>
#include <string>

#include "igl/request/request_config.hh"
#include "igl/trace/trace_config.hh"
#include "main/config.hh"
#include "simplessd/lib/pugixml/src/pugixml.hpp"

namespace Standalone {

//! Configuration section enum.
enum class Section {
  Simulation,
  RequestGenerator,
  TraceReplayer,
};

/**
 * \brief ConfigReader object declaration
 *
 * SimpleSSD-Standalone configuration object. This object provides configuration
 * parser. Also, you can override configuration by calling set function.
 */
class ConfigReader {
 private:
  pugi::xml_document file;

  Config simConfig;
  IGL::RequestConfig requestConfig;
  IGL::TraceConfig traceConfig;

 public:
  ConfigReader();
  ConfigReader(const ConfigReader &) = delete;
  ConfigReader(ConfigReader &&) noexcept = default;

  ConfigReader &operator=(const ConfigReader &) = delete;
  ConfigReader &operator=(ConfigReader &&) noexcept = default;

  void load(const char *, std::function<bool(pugi::xml_node &)>) noexcept;
  void load(std::string &, std::function<bool(pugi::xml_node &)>) noexcept;

  void save(const char *) noexcept;
  void save(std::string &) noexcept;

  int64_t readInt(Section, uint32_t) const noexcept;
  uint64_t readUint(Section, uint32_t) const noexcept;
  float readFloat(Section, uint32_t) const noexcept;
  std::string readString(Section, uint32_t) const noexcept;
  bool readBoolean(Section, uint32_t) const noexcept;

  bool writeInt(Section, uint32_t, int64_t) noexcept;
  bool writeUint(Section, uint32_t, uint64_t) noexcept;
  bool writeFloat(Section, uint32_t, float) noexcept;
  bool writeString(Section, uint32_t, std::string) noexcept;
  bool writeBoolean(Section, uint32_t, bool) noexcept;
};

}  // namespace Standalone

#endif
