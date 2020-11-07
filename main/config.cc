// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "main/config.hh"

#include <cstring>

namespace Standalone {

const char NAME_MODE[] = "Mode";
const char NAME_LOG_PERIOD[] = "LogPeriod";
const char NAME_OUTPUT_DIRECTORY[] = "OutputDirectory";
const char NAME_LOG_FILE[] = "StatLogFile";
const char NAME_DEBUG_LOG_FILE[] = "DebugLogFile";
const char NAME_LATENCY_LOG_FILE[] = "LatencyLogFile";
const char NAME_PROGRESS_PERIOD[] = "ProgressPeriod";
const char NAME_INTERFACE[] = "Interface";
const char NAME_SUBMISSION_LATENCY[] = "SubmissionLatency";
const char NAME_COMPLETION_LATENCY[] = "CompletionLatency";

//! A constructor
Config::Config() {
  mode = ModeType::RequestGenerator;
  statPeriod = 0;
  outputDirectory = ".";
  statFile = "";
  debugFile = "";
  latencyFile = "";
  progressPeriod = 0;
  interface = InterfaceType::NVMe;
  submissionLatency = 0;
  completionLatency = 0;
}

//! A destructor
Config::~Config() {}

void Config::loadFrom(pugi::xml_node &section) noexcept {
  for (auto node = section.first_child(); node; node = node.next_sibling()) {
    LOAD_NAME_UINT_TYPE(node, NAME_MODE, ModeType, mode);
    LOAD_NAME_TIME(node, NAME_LOG_PERIOD, statPeriod);
    LOAD_NAME_STRING(node, NAME_OUTPUT_DIRECTORY, outputDirectory);
    LOAD_NAME_STRING(node, NAME_LOG_FILE, statFile);
    LOAD_NAME_STRING(node, NAME_DEBUG_LOG_FILE, debugFile);
    LOAD_NAME_STRING(node, NAME_LATENCY_LOG_FILE, latencyFile);
    LOAD_NAME_UINT(node, NAME_PROGRESS_PERIOD, progressPeriod);
    LOAD_NAME_UINT_TYPE(node, NAME_INTERFACE, InterfaceType, interface);
    LOAD_NAME_TIME(node, NAME_SUBMISSION_LATENCY, submissionLatency);
    LOAD_NAME_TIME(node, NAME_COMPLETION_LATENCY, completionLatency);
  }
}

void Config::storeTo(pugi::xml_node &section) noexcept {
  // Assume section node is empty
  STORE_NAME_UINT(section, NAME_MODE, mode);
  STORE_NAME_TIME(section, NAME_LOG_PERIOD, statPeriod);
  STORE_NAME_STRING(section, NAME_OUTPUT_DIRECTORY, outputDirectory);
  STORE_NAME_STRING(section, NAME_LOG_FILE, statFile);
  STORE_NAME_STRING(section, NAME_DEBUG_LOG_FILE, debugFile);
  STORE_NAME_STRING(section, NAME_LATENCY_LOG_FILE, latencyFile);
  STORE_NAME_UINT(section, NAME_PROGRESS_PERIOD, progressPeriod);
  STORE_NAME_UINT(section, NAME_INTERFACE, interface);
  STORE_NAME_TIME(section, NAME_SUBMISSION_LATENCY, submissionLatency);
  STORE_NAME_TIME(section, NAME_COMPLETION_LATENCY, completionLatency);
}

void Config::update() noexcept {
  panic_if((uint8_t)mode > (uint8_t)ModeType::TraceReplayer, "Invalid Mode.");
  panic_if((uint8_t)interface > (uint8_t)InterfaceType::NVMe,
           "Invalid Interface.");
}

uint64_t Config::readUint(uint32_t idx) const noexcept {
  switch (idx) {
    case Key::Mode:
      return (uint64_t)mode;
    case Key::StatPeriod:
      return statPeriod;
    case Key::ProgressPeriod:
      return progressPeriod;
    case Key::Interface:
      return (uint64_t)interface;
    case Key::SubmissionLatency:
      return submissionLatency;
    case Key::CompletionLatency:
      return completionLatency;
  }

  return 0;
}

std::string Config::readString(uint32_t idx) const noexcept {
  switch (idx) {
    case Key::OutputDirectory:
      return outputDirectory;
    case Key::StatFile:
      return statFile;
    case Key::DebugFile:
      return debugFile;
    case Key::Latencyfile:
      return latencyFile;
  }

  return "";
}

bool Config::writeUint(uint32_t idx, uint64_t value) noexcept {
  bool ret = true;

  switch (idx) {
    case Key::Mode:
      mode = (ModeType)value;
      break;
    case Key::StatPeriod:
      statPeriod = value;
      break;
    case Key::ProgressPeriod:
      progressPeriod = value;
      break;
    case Key::Interface:
      interface = (InterfaceType)value;
      break;
    case Key::SubmissionLatency:
      submissionLatency = value;
      break;
    case Key::CompletionLatency:
      completionLatency = value;
      break;
    default:
      ret = false;
      break;
  }

  return ret;
}

bool Config::writeString(uint32_t idx, std::string &value) noexcept {
  bool ret = true;

  switch (idx) {
    case Key::OutputDirectory:
      outputDirectory = value;
      break;
    case Key::StatFile:
      statFile = value;
      break;
    case Key::DebugFile:
      debugFile = value;
      break;
    case Key::Latencyfile:
      latencyFile = value;
      break;
    default:
      ret = false;
      break;
  }

  return ret;
}

}  // namespace Standalone
