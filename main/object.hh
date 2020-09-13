// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __SIM_OBJECT_HH__
#define __SIM_OBJECT_HH__

#include <vector>

#include "main/config_reader.hh"
#include "main/engine.hh"
#include "simplessd/sim/log.hh"
#include "simplessd/sim/object.hh"

namespace Standalone {

struct ObjectData {
  ConfigReader *config;
  EventEngine *engine;
  SimpleSSD::Log *log;

  ObjectData() : config(nullptr), engine(nullptr) {}
  ObjectData(ConfigReader *c, EventEngine *e, SimpleSSD::Log *l)
      : config(c), engine(e), log(l) {}
};

class Object {
 protected:
  ObjectData &object;

  /* Helper APIs for Engine */
  inline uint64_t getTick() noexcept { return object.engine->getTick(); }
  inline Event createEvent(SimpleSSD::EventFunction ef,
                           std::string s) noexcept {
    return object.engine->createEvent(std::move(ef), std::move(s));
  }
  inline void scheduleNow(Event e, uint64_t c = 0) noexcept {
    object.engine->schedule(e, c, object.engine->getTick());
  }
  inline void scheduleAbs(Event e, uint64_t c, uint64_t t) noexcept {
    object.engine->schedule(e, c, t);
  }
  inline void deschedule(Event e) noexcept { object.engine->deschedule(e); }
  inline bool isScheduled(Event e) noexcept {
    return object.engine->isScheduled(e);
  }
  inline uint64_t when(Event e) noexcept { return object.engine->when(e); }

  /* Helper APIs for Config */
  inline int64_t readConfigInt(Section s, uint32_t k) noexcept {
    return object.config->readInt(s, k);
  }
  inline uint64_t readConfigUint(Section s, uint32_t k) noexcept {
    return object.config->readUint(s, k);
  }
  inline float readConfigFloat(Section s, uint32_t k) noexcept {
    return object.config->readFloat(s, k);
  }
  inline std::string readConfigString(Section s, uint32_t k) noexcept {
    return object.config->readString(s, k);
  }
  inline bool readConfigBoolean(Section s, uint32_t k) noexcept {
    return object.config->readBoolean(s, k);
  }
  inline bool writeConfigInt(Section s, uint32_t k, int64_t v) noexcept {
    return object.config->writeInt(s, k, v);
  }
  inline bool writeConfigUint(Section s, uint32_t k, uint64_t v) noexcept {
    return object.config->writeUint(s, k, v);
  }
  inline bool writeConfigFloat(Section s, uint32_t k, float v) noexcept {
    return object.config->writeFloat(s, k, v);
  }
  inline bool writeConfigString(Section s, uint32_t k,
                                std::string &v) noexcept {
    return object.config->writeString(s, k, v);
  }
  inline bool writeConfigBoolean(Section s, uint32_t k, bool v) noexcept {
    return object.config->writeBoolean(s, k, v);
  }

  /* Helper APIs for Log */
  template <class... T>
  inline void info_log(const char *format, T... args) const noexcept {
    object.log->print(SimpleSSD::Log::LogID::Info, format, args...);
  }
  template <class... T>
  inline void warn_log(const char *format, T... args) const noexcept {
    object.log->print(SimpleSSD::Log::LogID::Warn, format, args...);
  }
  template <class... T>
  inline void panic_log(const char *format, T... args) const noexcept {
    object.log->print(SimpleSSD::Log::LogID::Panic, format, args...);
  }

 public:
  Object(ObjectData &o) : object(o) {}
  virtual ~Object() {}
};

}  // namespace Standalone

#endif
