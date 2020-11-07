// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "main/config_reader.hh"

#include <cstring>
#include <iostream>

#include "main/version.hh"
#include "simplessd/sim/base_config.hh"

namespace Standalone {

#define CONFIG_STANDALONE_NODE_NAME "standalone"

//! ConfigReader constructor
ConfigReader::ConfigReader() {}

/**
 * \brief Load configuration from file
 *
 * \param[in] path Input file path
 */
void ConfigReader::load(const char *path,
                        std::function<bool(pugi::xml_node &)> cb) noexcept {
  auto result = file.load_file(
      path, pugi::parse_default | pugi::parse_trim_pcdata, pugi::encoding_utf8);

  if (!result) {
    std::cerr << "Failed to parse configuration file: " << result.description()
              << std::endl;

    abort();
  }

  // Check node
  auto config = file.child(CONFIG_STANDALONE_NODE_NAME);

  if (config) {
    // Check version
    auto version = config.attribute("version").value();
    if (strncmp(version, SIMPLESSD_STANDALONE_VERSION,
                strlen(SIMPLESSD_STANDALONE_VERSION)) != 0) {
      std::cerr
          << "SimpleSSD-Standalone configuration file version is different."
          << std::endl;
      std::cerr << " File version: " << version << std::endl;
      std::cerr << " Program version: " << SIMPLESSD_STANDALONE_VERSION
                << std::endl;
    }

    // Call callback for override
    if (!cb(config)) {
      std::cerr << " Override failed with error." << std::endl;

      abort();
    }

    // Travel sections
    for (auto section = config.first_child(); section;
         section = section.next_sibling()) {
      if (strcmp(section.name(), CONFIG_SECTION_NAME)) {
        continue;
      }

      auto name = section.attribute(CONFIG_ATTRIBUTE).value();

      if (strcmp(name, simConfig.getSectionName()) == 0) {
        simConfig.loadFrom(section);
      }
      else if (strcmp(name, requestConfig.getSectionName()) == 0) {
        requestConfig.loadFrom(section);
      }
      else if (strcmp(name, traceConfig.getSectionName()) == 0) {
        traceConfig.loadFrom(section);
      }
    }

    // Update config objects
    simConfig.update();
    requestConfig.update();
    traceConfig.update();
  }

  // Close
  file.reset();
}

//! Load configuration from file
void ConfigReader::load(std::string &path,
                        std::function<bool(pugi::xml_node &)> cb) noexcept {
  load(path.c_str(), cb);
}

/**
 * \brief Save configuration to file
 *
 * \param[in] path Output file path
 */
void ConfigReader::save(const char *path) noexcept {
  // Create simplessd node
  auto config = file.append_child(CONFIG_STANDALONE_NODE_NAME);
  config.append_attribute("version").set_value(SIMPLESSD_STANDALONE_VERSION);

  // Append configuration sections
  pugi::xml_node section;

  STORE_SECTION(config, simConfig.getSectionName(), section);
  simConfig.storeTo(section);

  STORE_SECTION(config, requestConfig.getSectionName(), section);
  requestConfig.storeTo(section);

  STORE_SECTION(config, traceConfig.getSectionName(), section)
  traceConfig.storeTo(section);

  auto result =
      file.save_file(path, "  ", pugi::format_default, pugi::encoding_utf8);

  if (!result) {
    std::cerr << "Failed to save configuration file" << std::endl;

    abort();
  }

  // Close
  file.reset();
}

//! Save configuration to file
void ConfigReader::save(std::string &path) noexcept {
  save(path.c_str());
}

//! Read configuration as int64
int64_t ConfigReader::readInt(Section section, uint32_t key) const noexcept {
  switch (section) {
    case Section::Simulation:
      return simConfig.readInt(key);
    case Section::RequestGenerator:
      return requestConfig.readInt(key);
    case Section::TraceReplayer:
      return traceConfig.readInt(key);
  }

  return 0ll;
}

//! Read configuration as uint64
uint64_t ConfigReader::readUint(Section section, uint32_t key) const noexcept {
  switch (section) {
    case Section::Simulation:
      return simConfig.readUint(key);
    case Section::RequestGenerator:
      return requestConfig.readUint(key);
    case Section::TraceReplayer:
      return traceConfig.readUint(key);
  }

  return 0ull;
}

//! Read configuration as float
float ConfigReader::readFloat(Section section, uint32_t key) const noexcept {
  switch (section) {
    case Section::Simulation:
      return simConfig.readFloat(key);
    case Section::RequestGenerator:
      return requestConfig.readFloat(key);
    case Section::TraceReplayer:
      return traceConfig.readFloat(key);
  }

  return 0.f;
}

//! Read configuration as string
std::string ConfigReader::readString(Section section,
                                     uint32_t key) const noexcept {
  switch (section) {
    case Section::Simulation:
      return simConfig.readString(key);
    case Section::RequestGenerator:
      return requestConfig.readString(key);
    case Section::TraceReplayer:
      return traceConfig.readString(key);
  }

  return "";
}

//! Read configuration as boolean
bool ConfigReader::readBoolean(Section section, uint32_t key) const noexcept {
  switch (section) {
    case Section::Simulation:
      return simConfig.readBoolean(key);
    case Section::RequestGenerator:
      return requestConfig.readBoolean(key);
    case Section::TraceReplayer:
      return traceConfig.readBoolean(key);
  }

  return "";
}

//! Write configuration as int64
bool ConfigReader::writeInt(Section section, uint32_t key,
                            int64_t value) noexcept {
  bool ret = false;

  switch (section) {
    case Section::Simulation:
      ret = simConfig.writeInt(key, value);
      break;
    case Section::RequestGenerator:
      ret = requestConfig.writeInt(key, value);
      break;
    case Section::TraceReplayer:
      ret = traceConfig.writeInt(key, value);
      break;
  }

  return ret;
}

//! Write configuration as uint64
bool ConfigReader::writeUint(Section section, uint32_t key,
                             uint64_t value) noexcept {
  bool ret = false;

  switch (section) {
    case Section::Simulation:
      ret = simConfig.writeUint(key, value);
      break;
    case Section::RequestGenerator:
      ret = requestConfig.writeUint(key, value);
      break;
    case Section::TraceReplayer:
      ret = traceConfig.writeUint(key, value);
      break;
  }

  return ret;
}

//! Write configuration as float
bool ConfigReader::writeFloat(Section section, uint32_t key,
                              float value) noexcept {
  bool ret = false;

  switch (section) {
    case Section::Simulation:
      ret = simConfig.writeFloat(key, value);
      break;
    case Section::RequestGenerator:
      ret = requestConfig.writeFloat(key, value);
      break;
    case Section::TraceReplayer:
      ret = traceConfig.writeFloat(key, value);
      break;
  }

  return ret;
}

//! Write configuration as string
bool ConfigReader::writeString(Section section, uint32_t key,
                               std::string value) noexcept {
  bool ret = false;

  switch (section) {
    case Section::Simulation:
      ret = simConfig.writeString(key, value);
      break;
    case Section::RequestGenerator:
      ret = requestConfig.writeString(key, value);
      break;
    case Section::TraceReplayer:
      ret = traceConfig.writeString(key, value);
      break;
  }

  return ret;
}

//! Write configuration as boolean
bool ConfigReader::writeBoolean(Section section, uint32_t key,
                                bool value) noexcept {
  bool ret = false;

  switch (section) {
    case Section::Simulation:
      ret = simConfig.writeBoolean(key, value);
      break;
    case Section::RequestGenerator:
      ret = requestConfig.writeBoolean(key, value);
      break;
    case Section::TraceReplayer:
      ret = traceConfig.writeBoolean(key, value);
      break;
  }

  return ret;
}

}  // namespace Standalone
