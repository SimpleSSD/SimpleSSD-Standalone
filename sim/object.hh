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

#include "sim/config_reader.hh"
#include "simplessd/sim/config_reader.hh"
#include "simplessd/sim/engine.hh"
#include "simplessd/sim/log.hh"
#include "simplessd/sim/object.hh"

using ObjectData = struct _ObjectData {
  SimpleSSD::ObjectData object;
  ConfigReader *simConfig;

  _ObjectData() : simConfig(nullptr) {}
  _ObjectData(SimpleSSD::Engine *e, SimpleSSD::ConfigReader *sc,
              SimpleSSD::Log *l, ConfigReader *c)
      : object(e, sc, l), simConfig(c) {}
};

/**
 * \brief Object object declaration
 *
 * Simulation object. All simulation module must inherit this class. Provides
 * API for accessing config, engine and log system.
 */
class Object : public SimpleSSD::Object {
 protected:
  ConfigReader *config;

  /* Helper APIs for Config */
  inline int64_t readSimConfigInt(Section s, uint32_t k) noexcept {
    return config->readInt(s, k);
  }
  inline uint64_t readSimConfigUint(Section s, uint32_t k) noexcept {
    return config->readUint(s, k);
  }
  inline float readSimConfigFloat(Section s, uint32_t k) noexcept {
    return config->readFloat(s, k);
  }
  inline std::string readSimConfigString(Section s, uint32_t k) noexcept {
    return config->readString(s, k);
  }
  inline bool readSimConfigBoolean(Section s, uint32_t k) noexcept {
    return config->readBoolean(s, k);
  }
  inline bool writeSimConfigInt(Section s, uint32_t k, int64_t v) noexcept {
    return config->writeInt(s, k, v);
  }
  inline bool writeSimConfigUint(Section s, uint32_t k, uint64_t v) noexcept {
    return config->writeUint(s, k, v);
  }
  inline bool writeSimConfigFloat(Section s, uint32_t k, float v) noexcept {
    return config->writeFloat(s, k, v);
  }
  inline bool writeSimConfigString(Section s, uint32_t k,
                                   std::string &v) noexcept {
    return config->writeString(s, k, v);
  }
  inline bool writeSimConfigBoolean(Section s, uint32_t k, bool v) noexcept {
    return config->writeBoolean(s, k, v);
  }

 public:
  Object(ObjectData &o) : SimpleSSD::Object(o.object), config(o.simConfig) {}
  virtual ~Object() {}

  /* Statistic API */
  virtual void getStatList(std::vector<Stat> &, std::string) noexcept = 0;
  virtual void getStatValues(std::vector<double> &) noexcept = 0;
  virtual void resetStatValues() noexcept = 0;

  /* No checkpoint in standalone (do we need it?) */
  void createCheckpoint(std::ostream &) const noexcept override {}
  void restoreCheckpoint(std::istream &) noexcept override {}
};

#endif
