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

  uint64_t scheduledAt;

  inline bool isScheduled() {
    return scheduledAt != std::numeric_limits<uint64_t>::max();
  }

  inline void deschedule() {
    scheduledAt = std::numeric_limits<uint64_t>::max();
  }

 public:
  EventData() : scheduledAt(std::numeric_limits<uint64_t>::max()) {}
#ifdef SIMPLESSD_STANDALONE_DEBUG
  EventData(SimpleSSD::EventFunction &&f, std::string &&s)
      : func(std::move(f)),
        name(std::move(s)),
        scheduledAt(std::numeric_limits<uint64_t>::max()) {}
#else
  EventData(SimpleSSD::EventFunction &&f)
      : func(std::move(f)), scheduledAt(std::numeric_limits<uint64_t>::max()) {}
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

    Job(Event e, uint64_t d) : eid(e), data(d) {}
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
  uint64_t when(Event);
  inline void invoke(Event e, uint64_t d) {
    e->func(getTick(), d);
  }

  SimpleSSD::InterruptFunction &getInterruptFunction();

  bool doNextEvent();
  void stopEngine();
  void printStats(std::ostream &);
  void getStat(uint64_t &);
};

}  // namespace Standalone

#endif
