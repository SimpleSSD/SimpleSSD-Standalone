// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "sim/engine.hh"

namespace Standalone {

EventEngine::EventEngine()
    : SimpleSSD::Engine(), simTick(0), forceStop(false), eventHandled(0) {
  watch.start();
}

EventEngine::~EventEngine() {}

void EventEngine::setFunction(SimpleSSD::EventFunction w,
                              SimpleSSD::InterruptFunction i) {
  engineEvent.func = std::move(w);
#ifdef SIMPLESSD_STANDALONE_DEBUG
  engineEvent.name = "SimpleSSD::Engine::EventFunction";
#endif

  intrFunction = std::move(i);
}

void EventEngine::schedule(uint64_t tick) {
  schedule(&engineEvent, 0ull, tick);
}

uint64_t EventEngine::getTick() {
  std::lock_guard<std::mutex> guard(mTick);

  return simTick;
}

#ifdef SIMPLESSD_DEBUG
Event EventEngine::createEvent(SimpleSSD::EventFunction &&func,
                               std::string &&name) noexcept {
  Event eid = new EventData(std::move(func), std::move(name));
#else
Event EventEngine::createEvent(SimpleSSD::EventFunction &&func,
                               std::string &&) noexcept {
  Event eid = new EventData(std::move(func));
#endif

  eventList.emplace_back(eid);

  return eid;
}

void EventEngine::schedule(Event eid, uint64_t data, uint64_t tick) {
  if (UNLIKELY(eid == InvalidEventID)) {
    // No need to schedule
    return;
  }

  uint64_t tickCopy;

  {
    std::lock_guard<std::mutex> guard(mTick);
    tickCopy = simTick;
  }

  if (tick < tickCopy) {
    fprintf(stderr,
            "Tried to schedule %" PRIu64 " < simTick to event %" PRIu64 ".\n",
            tick, eid);

    abort();
  }

  auto insert = jobQueue.end();

  for (auto entry = jobQueue.begin(); entry != jobQueue.end(); ++entry) {
    if (entry->eid->scheduledAt > tick) {
      insert = entry;

      break;
    }
  }

  eid->scheduledAt = tick;

  jobQueue.emplace(insert, Job(eid, data));
}

void EventEngine::deschedule(Event eid) {
  eid->deschedule();

  for (auto iter = jobQueue.begin(); iter != jobQueue.end(); ++iter) {
    if (iter->eid == eid) {
      jobQueue.erase(iter);

      return;
    }
  }
}

bool EventEngine::isScheduled(Event eid) {
  return eid->isScheduled();
}

bool EventEngine::doNextEvent() {
  uint64_t tickCopy;

  if (forceStop) {
    return false;
  }

  if (jobQueue.size() > 0) {
    auto job = std::move(jobQueue.front());
    jobQueue.pop_front();

    {
      std::lock_guard<std::mutex> guard(mTick);

      simTick = job.eid->scheduledAt;
      tickCopy = simTick;
    }

    job.eid->func(tickCopy, job.data);

    {
      std::lock_guard<std::mutex> guard(m);
      eventHandled++;
    }

    return true;
  }

  return false;
}

void EventEngine::stopEngine() {
  forceStop = true;
}

void EventEngine::printStats(std::ostream &out) {
  watch.stop();

  double duration = watch.getDuration();

  out << "*** Statistics of Event EventEngine ***" << std::endl;
  {
    std::lock_guard<std::mutex> guard(mTick);

    out << "Simulation Tick (ps): " << simTick << std::endl;
  }
  out << "Host time duration (sec): " << std::to_string(duration) << std::endl;
  out << "Event handled: " << eventHandled << " ("
      << std::to_string(eventHandled / duration) << " ops)" << std::endl;
  out << "*** End of statistics ***" << std::endl;
}

void EventEngine::getStat(uint64_t &val) {
  std::lock_guard<std::mutex> guard(m);
  val = eventHandled;
}

}  // namespace Standalone
