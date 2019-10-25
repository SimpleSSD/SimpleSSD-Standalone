// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "main/engine.hh"

namespace Standalone {

EventEngine::EventEngine()
    : SimpleSSD::Engine(), simTick(0), forceStop(false), eventHandled(0) {
  watch.start();
}

EventEngine::~EventEngine() {
  // Delete all events
  for (auto &iter : eventList) {
    delete iter;
  }

  eventList.clear();
}

void EventEngine::setFunction(SimpleSSD::EventFunction w,
                              SimpleSSD::InterruptFunction i) {
  engineEvent.func = std::move(w);
#ifdef SIMPLESSD_STANDALONE_DEBUG
  engineEvent.name = "SimpleSSD::Engine::EventFunction";
#endif

  intrFunction = std::move(i);
}

void EventEngine::schedule(uint64_t tick) {
  if (LIKELY(engineEvent.isScheduled())) {
    deschedule(&engineEvent);
  }

  schedule(&engineEvent, 0ull, tick);
}

uint64_t EventEngine::getTick() {
  std::lock_guard<std::mutex> guard(mTick);

  return simTick;
}

void EventEngine::forceUnlock() {
  mTick.unlock();
}

#ifdef SIMPLESSD_STANDALONE_DEBUG
Event EventEngine::createEvent(SimpleSSD::EventFunction &&func,
                               std::string &&name) {
  Event eid = new EventData(std::move(func), std::move(name));
#else
Event EventEngine::createEvent(SimpleSSD::EventFunction &&func,
                               std::string &&) {
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
            "Tried to schedule event %" PRIx64 "h at %" PRIu64
            " < simTick (%" PRIu64 ").\n",
            (uint64_t)eid, tick, tickCopy);

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

  for (auto iter = jobQueue.begin(); iter != jobQueue.end();) {
    if (iter->eid == eid) {
      iter = jobQueue.erase(iter);
    }
    else {
      ++iter;
    }
  }
}

bool EventEngine::isScheduled(Event eid) {
  return eid->isScheduled();
}

uint64_t EventEngine::when(Event eid) {
  return eid->scheduledAt;
}

SimpleSSD::InterruptFunction &EventEngine::getInterruptFunction() {
  return intrFunction;
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

    job.eid->deschedule();
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
