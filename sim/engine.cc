// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "sim/engine.hh"

EventEngine::EventEngine()
    : SimpleSSD::Engine(),
      simTick(0),
      forceStop(false),
      counter(0),
      eventHandled(0) {
  watch.start();
}

EventEngine::~EventEngine() {}

uint64_t EventEngine::getTick() {
  std::lock_guard<std::mutex> guard(mTick);

  return simTick;
}

SimpleSSD::Event EventEngine::createEvent(SimpleSSD::EventFunction func,
                                          std::string) {
  auto iter = eventList.insert({++counter, func});

  if (!iter.second) {
    fprintf(stderr, "Fail to allocate event\n");

    abort();
  }

  return counter;
}

void EventEngine::schedule(SimpleSSD::Event eid, uint64_t tick) {
  auto iter = eventList.find(eid);

  if (iter != eventList.end()) {
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

    eventQueue.insert_or_assign(eid, tick);
  }
  else {
    fprintf(stderr, "Event %" PRIu64 " does not exists.\n", eid);

    abort();
  }
}

void EventEngine::deschedule(SimpleSSD::Event eid) {
  auto iter = eventList.find(eid);

  if (iter != eventList.end()) {
    auto iter = eventQueue.find(eid);

    if (iter == eventQueue.end()) {
      fprintf(stderr, "Event %" PRIu64 " not scheduled.\n", eid);

      abort();
    }

    eventQueue.erase(iter);
  }
  else {
    fprintf(stderr, "Event %" PRIu64 " does not exists.\n", eid);

    abort();
  }
}

bool EventEngine::isScheduled(SimpleSSD::Event eid) {
  bool ret = false;
  auto iter = eventList.find(eid);

  if (iter != eventList.end()) {
    auto iter = eventQueue.find(eid);

    return iter != eventQueue.end();
  }
  else {
    fprintf(stderr, "Event %" PRIu64 " does not exists.\n", eid);

    abort();
  }

  return ret;
}

void EventEngine::destroyEvent(SimpleSSD::Event eid) {
  auto iter = eventList.find(eid);

  if (iter != eventList.end()) {
    eventList.erase(iter);

    auto iter = eventQueue.find(eid);

    if (iter != eventQueue.end()) {
      eventQueue.erase(iter);
    }
  }
  else {
    fprintf(stderr, "Event %" PRIu64 " does not exists.\n", eid);

    abort();
  }
}

bool EventEngine::doNextEvent() {
  uint64_t tickCopy;

  if (forceStop) {
    return false;
  }

  if (eventQueue.size() > 0) {
    auto now = eventQueue.begin();

    {
      std::lock_guard<std::mutex> guard(mTick);

      simTick = now->second;
      tickCopy = simTick;
    }

    auto iter = eventList.find(now->first);

    if (iter != eventList.end()) {
      iter->second(tickCopy);
    }
    else {
      fprintf(stderr, "Event %" PRIu64 " does not exists.\n", now->first);

      abort();
    }

    eventQueue.erase(now);

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
