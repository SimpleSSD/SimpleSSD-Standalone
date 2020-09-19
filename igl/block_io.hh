// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __IGL_BLOCK_IO_HH__
#define __IGL_BLOCK_IO_HH__

#include "driver/abstract_interface.hh"
#include "main/object.hh"

namespace Standalone::IGL {

struct Progress {
  uint64_t iops;
  uint64_t bandwidth;
  uint64_t latency;

  Progress() : iops(0), bandwidth(0), latency(0) {}
};

class BlockIOLayer : public Object {
 protected:
  Driver::AbstractInterface *pInterface;

  // Latency file
  std::ostream *pLatencyLogFile;

  // Request queues
  std::deque<Driver::Request> submissionQueue;
  std::unordered_map<uint16_t, Driver::Request> dispatchedQueue;
  std::deque<Driver::Request> completionQueue;

  // SSD size
  uint64_t bytesize;
  uint32_t bs;

  uint16_t requestCounter;

  // Options
  uint16_t maxIODepth;
  uint64_t submissionLatency;
  uint64_t completionLatency;

  // Statistics
  uint64_t io_count;
  uint64_t minLatency;
  uint64_t maxLatency;
  uint64_t sumLatency;
  uint64_t squareSumLatency;

  // Progress
  std::mutex m;
  uint64_t lastProgressAt;
  uint64_t io_progress;
  Progress progress;

  Event iglCallback;  // Callback handler to I/O generator layer

  Event eventDispatch;
  void dispatch(uint64_t);

  Event eventCompletion;
  void completion(uint64_t);
  inline void calculateStat(uint64_t);

 public:
  BlockIOLayer(ObjectData &, Driver::AbstractInterface *, std::ostream *);
  ~BlockIOLayer();

  /**
   * \brief Initialize Block I/O Layer
   *
   * Configure Block I/O Layer.
   *
   * \param[in] md  Maximum I/O Depth that Block I/O Layer can handle.
   * \param[in] sl  Submission Latency. Request submitted to the Block I/O Layer
   *                will be dispatched to driver after this latency.
   * \param[in] cl  Completion Latency. Request completed by driver will be
   *                notified to I/O generator after this latency.
   * \param[in] cb  Callback Event which will be used for notifying request
   *                completion.
   */
  void initialize(uint16_t md, uint64_t sl, uint64_t cl, Event cb) noexcept;

  /**
   * \brief Submit request to Block I/O Layer
   *
   * Submit request.
   *
   * \param[in] type    Request Type
   * \param[in] offset  Byte offset
   * \param[in] length  Byte length
   */
  bool submitRequest(Driver::RequestType type, uint64_t offset,
                     uint64_t length) noexcept;

  /**
   * \brief Get SSD size information
   *
   * \param[out] size Byte size of SSD
   * \param[out] bs   Byte size of logical block
   */
  void getSSDSize(uint64_t &size, uint32_t &bs) noexcept;

  /**
   * \brief Complete Request (For Driver)
   *
   * \param[in] tag Request Tag ID
   * \return  Driver data stored in request
   */
  void *postCompletion(uint16_t) noexcept;

  void printStats(std::ostream &) noexcept;
  void getProgress(Progress &) noexcept;
};

}  // namespace Standalone::IGL

#endif
