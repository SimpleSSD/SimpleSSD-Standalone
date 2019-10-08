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
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "simplessd/sim/simulator.hh"
#include "util/stopwatch.hh"

class Engine : public SimpleSSD::Simulator {
 private:
  std::mutex mTick;
  uint64_t simTick;
  SimpleSSD::Event counter;
  bool forceStop;
  std::unordered_map<SimpleSSD::Event, SimpleSSD::EventFunction> eventList;
  std::list<std::pair<SimpleSSD::Event, uint64_t>> eventQueue;

  Stopwatch watch;

  std::mutex m;
  uint64_t eventHandled;

  bool insertEvent(SimpleSSD::Event, uint64_t, uint64_t * = nullptr);
  bool removeEvent(SimpleSSD::Event);
  bool isEventExist(SimpleSSD::Event, uint64_t * = nullptr);

 public:
  Engine();
  ~Engine();

  uint64_t getCurrentTick() override;

  SimpleSSD::Event allocateEvent(SimpleSSD::EventFunction) override;
  void scheduleEvent(SimpleSSD::Event, uint64_t) override;
  void descheduleEvent(SimpleSSD::Event) override;
  bool isScheduled(SimpleSSD::Event, uint64_t * = nullptr) override;
  void deallocateEvent(SimpleSSD::Event) override;

  bool doNextEvent();
  void stopEngine();
  void printStats(std::ostream &);
  void getStat(uint64_t &);
};

#endif
