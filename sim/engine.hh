// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __SIM_ENGINE__
#define __SIM_ENGINE__

#include <iostream>
#include <map>
#include <mutex>
#include <thread>

#include "simplessd/sim/engine.hh"
#include "util/stopwatch.hh"

class EventEngine : public SimpleSSD::Engine {
 private:
  std::mutex mTick;
  uint64_t simTick;

  bool forceStop;

  SimpleSSD::Event counter;
  std::map<SimpleSSD::Event, SimpleSSD::EventFunction> eventList;
  std::map<SimpleSSD::Event, uint64_t> eventQueue;

  Stopwatch watch;

  std::mutex m;
  uint64_t eventHandled;

 public:
  EventEngine();
  ~EventEngine();

  uint64_t getTick() override;

  SimpleSSD::Event createEvent(SimpleSSD::EventFunction, std::string) override;
  void schedule(SimpleSSD::Event, uint64_t) override;
  void deschedule(SimpleSSD::Event) override;
  bool isScheduled(SimpleSSD::Event) override;
  void destroyEvent(SimpleSSD::Event) override;

  void createCheckpoint(std::ostream &) const override;
  void restoreCheckpoint(std::istream &) override;

  bool doNextEvent();
  void stopEngine();
  void printStats(std::ostream &);
  void getStat(uint64_t &);
};

#endif
