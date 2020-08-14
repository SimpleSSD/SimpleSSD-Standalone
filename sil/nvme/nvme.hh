// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#pragma once

#ifndef __DRIVERS_NVME__
#define __DRIVERS_NVME__

#include <list>
#include <unordered_map>

#include "bil/interface.hh"
#include "sil/nvme/prp.hh"
#include "sil/nvme/queue.hh"
#include "simplessd/hil/nvme/controller.hh"
#include "simplessd/sim/simplessd.hh"
#include "simplessd/util/scheduler.hh"

#define QUEUE_ENTRY_ADMIN 256
#define QUEUE_ENTRY_IO 1024

namespace Standalone::SIL::NVMe {

using InterruptHandler = std::function<void(uint16_t, uint32_t, uint64_t)>;

// Copied from SimpleSSD-FullSystem
struct DMAEntry {
  uint64_t addr;
  uint64_t size;
  uint8_t *buffer;
  SimpleSSD::Event eid;
  uint64_t data;

  DMAEntry()
      : addr(0), size(0), buffer(nullptr), eid(SimpleSSD::InvalidEventID) {}

  static void backup(std::ostream &, DMAEntry *) {}
  static DMAEntry *restore(std::istream &, SimpleSSD::ObjectData &) {
    return nullptr;
  }
};

struct CommandEntry {
  uint16_t iv;  // Same as Queue ID
  uint16_t opcode;
  uint16_t cid;
  InterruptHandler func;
  uint64_t data;

  CommandEntry(uint16_t i, uint16_t o, uint16_t c, InterruptHandler &&f,
               uint64_t d)
      : iv(i), opcode(o), cid(c), func(std::move(f)), data(d) {}
};

struct IOWrapper {
  uint64_t id;
  PRP *prp;

  IOWrapper(uint64_t i, PRP *p) : id(i), prp(p) {}
};

class Driver : public BIL::DriverInterface, SimpleSSD::Interface {
 private:
  SimpleSSD::HIL::NVMe::Controller *controller;

  // PCI Express (for DMA throttling)
  SimpleSSD::DelayFunction delayFunction;

  // DMA scheduling
  SimpleSSD::Scheduler<DMAEntry *> scheduler;

  // NVMe Identify
  Event beginEvent;
  uint64_t capacity;
  uint32_t LBAsize;
  uint32_t namespaceID;

  // Queue
  uint32_t maxQueueEntries;
  uint16_t adminCommandID;
  uint16_t ioCommandID;
  bool phase;
  Queue *adminSQ;
  Queue *adminCQ;
  Queue *ioSQ;
  Queue *ioCQ;
  std::list<CommandEntry> pendingCommandList;
  std::unordered_map<uint64_t, IOWrapper> pendingIOList;

  uint64_t preSubmitRead(DMAEntry *);
  uint64_t preSubmitWrite(DMAEntry *);
  void postDone(DMAEntry *);

  void increaseCommandID(uint16_t &);

  PRP *adminPRP;

  void _init0();
  void _init1();
  void _init2();
  void _init3(uint16_t, uint32_t);
  void _init4(uint16_t);
  void _init5(uint16_t);
  void callback(uint16_t, uint64_t);

  void submitCommand(uint16_t, uint8_t *, InterruptHandler &&, uint64_t = 0);

 public:
  Driver(ObjectData &, SimpleSSD::SimpleSSD &);
  ~Driver();

  // BIL::DriverInterface
  void init(Event) override;
  void getInfo(uint64_t &, uint32_t &) override;
  void submitIO(BIL::BIO &) override;

  // SimpleSSD::Interface
  void read(uint64_t, uint32_t, uint8_t *, SimpleSSD::Event,
            uint64_t = 0) override;
  void write(uint64_t, uint32_t, uint8_t *, SimpleSSD::Event,
             uint64_t = 0) override;

  void postInterrupt(uint16_t, bool) override;
  void getPCIID(uint16_t &, uint16_t &) override;

  void initStats(std::vector<SimpleSSD::Stat> &) override;
  void getStats(std::vector<double> &) override;
};

}  // namespace Standalone::SIL::NVMe

#endif
