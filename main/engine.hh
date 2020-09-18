// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __MAIN_ENGINE_HH__
#define __MAIN_ENGINE_HH__

#include <iostream>
#include <list>
#include <mutex>
#include <thread>

#include "simplessd/sim/engine.hh"
#include "util/stopwatch.hh"

namespace Standalone {

class EventEngine;

class EventData {
 private:
  friend EventEngine;

  SimpleSSD::EventFunction func;
#ifdef SIMPLESSD_STANDALONE_DEBUG
  std::string name;
#endif

  uint64_t scheduled;

  inline bool isScheduled() { return scheduled > 0; }
  inline void deschedule() { scheduled--; }
  inline void schedule() { scheduled++; }

 public:
  EventData() : scheduled(0) {}
#ifdef SIMPLESSD_STANDALONE_DEBUG
  EventData(SimpleSSD::EventFunction &&f, std::string &&s)
      : func(std::move(f)), name(std::move(s)), scheduled(0) {}
#else
  EventData(SimpleSSD::EventFunction &&f) : func(std::move(f)), scheduled(0) {}
#endif
  EventData(const EventData &) = delete;
  EventData(EventData &&) noexcept = delete;

  EventData &operator=(const EventData &) = delete;
  EventData &operator=(EventData &&) = delete;
};

using Event = EventData *;
const Event InvalidEventID = nullptr;

class EventEngine : public SimpleSSD::Engine {
 private:
  class Job {
   public:
    Event eid;
    uint64_t data;
    uint64_t tick;

    Job(Event e, uint64_t d, uint64_t t) : eid(e), data(d), tick(t) {}
  };

  std::mutex mTick;
  uint64_t simTick;

  bool forceStop;

  std::vector<Event> eventList;
  std::list<Job> jobQueue;

  Stopwatch watch;

  std::mutex m;
  uint64_t eventHandled;

  EventData engineEvent;
  SimpleSSD::InterruptFunction intrFunction;

 public:
  EventEngine();
  ~EventEngine();

  void setFunction(SimpleSSD::EventFunction,
                   SimpleSSD::InterruptFunction) override;
  void schedule(uint64_t) override;

  uint64_t getTick() override;

  void forceUnlock();

  Event createEvent(SimpleSSD::EventFunction &&, std::string &&);
  void schedule(Event, uint64_t, uint64_t);
  void deschedule(Event);
  bool isScheduled(Event);
  inline void invoke(Event e, uint64_t d) { e->func(getTick(), d); }

  SimpleSSD::InterruptFunction &getInterruptFunction();

  bool doNextEvent();
  void stopEngine();
  void printStats(std::ostream &);
  void getStat(uint64_t &);
};

}  // namespace Standalone

#endif
