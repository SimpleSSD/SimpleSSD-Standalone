// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "igl/block_io.hh"

namespace Standalone::IGL {

BlockIOLayer::BlockIOLayer(ObjectData &o, Driver::AbstractInterface *i,
                           std::ostream *f)
    : Object(o),
      pInterface(i),
      pLatencyLogFile(f),
      bytesize(0),
      bs(0),
      requestCounter(0),
      maxIODepth(0),
      submissionLatency(0),
      completionLatency(0),
      io_count(0),
      minLatency(std::numeric_limits<uint64_t>::max()),
      maxLatency(0),
      sumLatency(0),
      squareSumLatency(0),
      lastProgressAt(0),
      io_progress(0),
      iglCallback(InvalidEventID) {
  eventDispatch = createEvent([this](uint64_t t, uint64_t) { dispatch(t); },
                              "IGL::BlockIOLayer::eventDispatch");
  eventCompletion = createEvent([this](uint64_t t, uint64_t) { completion(t); },
                                "IGL::BlockIOLayer::eventCompletion");
}

BlockIOLayer::~BlockIOLayer() {}

void BlockIOLayer::dispatch(uint64_t now) {
  auto &iter = submissionQueue.front();
  auto ret = dispatchedQueue.emplace(iter.tag, std::move(iter));

  submissionQueue.pop_front();

  panic_if(!ret.second, "Request ID conflict.");

  if (submissionQueue.size() > 0) {
    scheduleAbs(eventDispatch, 0, now + submissionLatency);
  }

  pInterface->submit(ret.first->second);
}

void BlockIOLayer::completion(uint64_t now) {
  auto &iter = completionQueue.front();

  calculateStat(now);

  object.engine->invoke(iglCallback, iter.tag);

  completionQueue.pop_front();

  if (completionQueue.size() > 0) {
    scheduleAbs(eventCompletion, 0,
                completionQueue.front().completedAt + completionLatency);
  }
}

void BlockIOLayer::calculateStat(uint64_t now) {
  auto &iter = completionQueue.front();

  auto lat = now - iter.submittedAt;

  {
    std::lock_guard<std::mutex> guard(m);

    io_progress++;

    progress.latency += lat;
    progress.iops++;
    progress.bandwidth += iter.length * bs;
  }

  if (pLatencyLogFile) {
    *pLatencyLogFile << std::to_string(iter.submittedAt) << ", "
                     << static_cast<int>(iter.type) << ", "
                     << std::to_string(iter.offset * bs) << ", "
                     << std::to_string(iter.length * bs) << ", "
                     << std::to_string(lat) << std::endl;
  }

  minLatency = MIN(minLatency, lat);
  maxLatency = MAX(maxLatency, lat);

  io_count++;
  sumLatency += lat;
  squareSumLatency += lat * lat;
}

void BlockIOLayer::initialize(uint16_t md, uint64_t sl, uint64_t cl,
                              Event cb) noexcept {
  panic_if(getTick() != 0,
           "Initialization should be done at the beginning of simulation.");

  maxIODepth = md;
  submissionLatency = sl;
  completionLatency = cl;
  iglCallback = cb;
}

bool BlockIOLayer::submitRequest(Driver::RequestType type, uint64_t offset,
                                 uint64_t length, uint16_t &tag) noexcept {
  uint64_t now = getTick();

  if (UNLIKELY(submissionQueue.size() + dispatchedQueue.size() +
                   completionQueue.size() >=
               maxIODepth)) {
    return false;
  }

  // Check offset
  warn_if((offset % bs) != 0, "Offset is not aligned to block size.");

  if (offset + length > bytesize) {
    warn("Request offset is larger than SSD size.");

    offset %= bytesize;
  }

  // Check length
  panic_if(length == 0, "Zero-sized request.");
  warn_if((length % bs) != 0, "Length is not aligned to block size.");

  if (length < bs) {
    warn("Length is smaller than block size.")
  }

  offset = offset / bs;
  length = DIVCEIL(length, bs);

  if (length > std::numeric_limits<uint32_t>::max()) {
    warn("Too large request.");

    length = std::numeric_limits<uint32_t>::max();
  }

  tag = requestCounter++;
  auto &ret = submissionQueue.emplace_back(
      Driver::Request(tag, offset, (uint32_t)length, type));

  ret.submittedAt = now;

  if (submissionQueue.size() == 1) {
    scheduleAbs(eventDispatch, 0, now + submissionLatency);
  }

  return true;
}

void BlockIOLayer::getSSDSize(uint64_t &size, uint32_t &blocksize) noexcept {
  if (LIKELY(bytesize == 0 || bs == 0)) {
    pInterface->getSSDInfo(bytesize, bs);
  }

  panic_if(bytesize == 0 || bs == 0, "Unexpected access to SSD size.");

  size = bytesize;
  blocksize = bs;
}

void *BlockIOLayer::postCompletion(uint16_t tag) noexcept {
  uint64_t now = getTick();
  auto iter = dispatchedQueue.find(tag);

  panic_if(iter == dispatchedQueue.end(), "Unexpected request ID.");

  iter->second.completedAt = now;

  auto ptr = iter->second.getDriverData();

  // Move to completionQueue
  completionQueue.emplace_back(std::move(iter->second));

  dispatchedQueue.erase(iter);

  if (completionQueue.size() == 1) {
    scheduleAbs(eventCompletion, 0, now + completionLatency);
  }

  return ptr;
}

void BlockIOLayer::printStats(std::ostream &out) noexcept {
  double avgLatency = (double)sumLatency / io_count;
  double stdevLatency = sqrt((double)squareSumLatency / io_count - avgLatency);
  double digit = log10(avgLatency);

  out << "*** Statistics of Block I/O Entry ***" << std::endl;

  if (digit < 6.0) {
    out << "Latency (ps): min=" << std::to_string(minLatency)
        << ", max=" << std::to_string(maxLatency)
        << ", avg=" << std::to_string(avgLatency)
        << ", stdev=" << std::to_string(stdevLatency) << std::endl;
  }
  else if (digit < 9.0) {
    out << "Latency (ns): min=" << std::to_string(minLatency / 1000.0)
        << ", max=" << std::to_string(maxLatency / 1000.0)
        << ", avg=" << std::to_string(avgLatency / 1000.0)
        << ", stdev=" << std::to_string(stdevLatency / 31.62277660168)
        << std::endl;
  }
  else if (digit < 12.0) {
    out << "Latency (us): min=" << std::to_string(minLatency / 1000000.0)
        << ", max=" << std::to_string(maxLatency / 1000000.0)
        << ", avg=" << std::to_string(avgLatency / 1000000.0)
        << ", stdev=" << std::to_string(stdevLatency / 1000.0) << std::endl;
  }
  else {
    out << "Latency (ms): min=" << std::to_string(minLatency / 1000000000.0)
        << ", max=" << std::to_string(maxLatency / 1000000000.0)
        << ", avg=" << std::to_string(avgLatency / 1000000000.0)
        << ", stdev=" << std::to_string(stdevLatency / 31622.77660168379)
        << std::endl;
  }

  out << "*** End of statistics ***" << std::endl;
}

void BlockIOLayer::getProgress(Progress &data) noexcept {
  uint64_t tick = getTick();
  uint64_t diff = tick - lastProgressAt;

  if (diff == 0) {
    data.iops = 0;
    data.bandwidth = 0;
    data.latency = 0;

    return;
  }

  double ratio = 1000000000000.0 / diff;

  {
    std::lock_guard<std::mutex> guard(m);

    data.iops = (uint64_t)(progress.iops * ratio);
    data.bandwidth = (uint64_t)(progress.bandwidth * ratio);

    if (io_progress == 0) {
      data.latency = 0;
    }
    else {
      data.latency = progress.latency / io_progress;
    }

    io_progress = 0;
    progress.iops = 0;
    progress.bandwidth = 0;
    progress.latency = 0;
  }

  lastProgressAt = tick;
}

}  // namespace Standalone::IGL
