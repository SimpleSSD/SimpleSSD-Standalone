// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "driver/nvme/nvme.hh"

#include "igl/block_io.hh"
#include "simplessd/hil/nvme/def.hh"
#include "simplessd/sim/object.hh"
#include "simplessd/util/algorithm.hh"

namespace Standalone::Driver::NVMe {

NVMeInterface::NVMeInterface(ObjectData &o, SimpleSSD::SimpleSSD &s)
    : AbstractInterface(o, s),
      scheduler(
          s.getObject(), "Driver::NVMe::NVMeInterface::scheduler",
          [this](DMAEntry *d) -> uint64_t { return preSubmitRead(d); },
          [this](DMAEntry *d) -> uint64_t { return preSubmitWrite(d); },
          [this](DMAEntry *d) { postDone(d); },
          [this](DMAEntry *d) { postDone(d); }, DMAEntry::backup,
          DMAEntry::restore),
      beginEvent(InvalidEventID),
      capacity(0),
      LBAsize(0),
      namespaceID(0),
      maxQueueEntries(0),
      phase(true),
      initState(InitState::None),
      adminSQ(nullptr),
      adminCQ(nullptr),
      ioSQ(nullptr),
      ioCQ(nullptr),
      adminPRP(nullptr) {
  auto &simobj = s.getObject();

  auto gen = (SimpleSSD::PCIExpress::Generation)simobj.config->readUint(
      SimpleSSD::Section::HostInterface,
      SimpleSSD::HIL::Config::Key::PCIeGeneration);
  auto lane = (uint8_t)simobj.config->readUint(
      SimpleSSD::Section::HostInterface, SimpleSSD::HIL::Config::Key::PCIeLane);

  delayFunction = SimpleSSD::PCIExpress::makeFunction(gen, lane);

  // Create controller
  auto id = simplessd.createController(this);
  controller = (SimpleSSD::HIL::NVMe::Controller *)simplessd.getController(id);
}

NVMeInterface::~NVMeInterface() {
  delete adminSQ;
  delete adminCQ;
  delete ioSQ;
  delete ioCQ;
}

void NVMeInterface::initialize(IGL::BlockIOLayer *p, Event eid) {
  AbstractInterface::initialize(p, eid);

  beginEvent = eid;

  // NVMe Initialization process (Register)
  // See Section 7.6.1. Initialization of NVMe 1.3c
  union {
    uint64_t value;
    uint8_t buffer[8];
  } temp;

  // Step 1. Read CAP
  controller->read(
      (uint64_t)SimpleSSD::HIL::NVMe::Register::ControllerCapabilities, 8,
      temp.buffer);

  // MPSMAX/MIN is setted to 4KB
  // DSTRD is setted to 0 (4bytes)
  // Check MQES
  maxQueueEntries = (temp.value & 0xFFFF) + 1;

  // Step 2. Wait for CSTS.RDY = 0
  // Step 3. Configure admin queue
  // Step 3-1. Set admin queue entry size
  uint16_t entries = QUEUE_ENTRY_ADMIN;

  if (entries > maxQueueEntries) {
    entries = maxQueueEntries;
  }

  temp.value = entries - 1;
  temp.value |= (entries - 1) << 16;

  controller->write(
      (uint64_t)SimpleSSD::HIL::NVMe::Register::AdminQueueAttributes, 4,
      temp.buffer);

  adminSQ = new Queue(entries, 64);
  adminCQ = new Queue(entries, 16);

  // Step 3-2. Write base addresses
  adminSQ->getBaseAddress(temp.value);
  controller->write(
      (uint64_t)SimpleSSD::HIL::NVMe::Register::AdminSQBaseAddress, 8,
      temp.buffer);
  adminCQ->getBaseAddress(temp.value);
  controller->write(
      (uint64_t)SimpleSSD::HIL::NVMe::Register::AdminCQBaseAddress, 8,
      temp.buffer);

  // Step 4. Configure controller
  // Step 5. Enable controller
  temp.value = 1;            // Round Robin, 4K page, NVM command set, Enable
  temp.value |= 0x00460000;  // 64B SQEntry, 16B CQEntry
  controller->write(
      (uint64_t)SimpleSSD::HIL::NVMe::Register::ControllerConfiguration, 4,
      temp.buffer);

  // Step 6. Wait for CSTS.RDY = 1
  // Step 7. Send Identify
  // Step 7-1. Submit Identify Controller
  uint32_t cmd[16];
  adminPRP = new PRP(4096);

  memset(cmd, 0, 64);

  cmd[0] = (uint8_t)SimpleSSD::HIL::NVMe::AdminCommand::Identify;
  adminPRP->getPointer(*(uint64_t *)(cmd + 6), *(uint64_t *)(cmd + 8));
  cmd[10] = (uint8_t)SimpleSSD::HIL::NVMe::IdentifyStructure::Controller;

  initState = InitState::Phase0;

  submitCommand(0, (uint8_t *)cmd);
}

void NVMeInterface::_init0() {
  // Step 7-2. Send Identify Active Namespace List
  // Reuse PRP here
  uint32_t cmd[16];

  memset(cmd, 0, 64);
  cmd[0] = (uint8_t)SimpleSSD::HIL::NVMe::AdminCommand::Identify;
  adminPRP->getPointer(*(uint64_t *)(cmd + 6), *(uint64_t *)(cmd + 8));
  cmd[10] =
      (uint8_t)SimpleSSD::HIL::NVMe::IdentifyStructure::ActiveNamespaceList;

  initState = InitState::Phase1;

  submitCommand(0, (uint8_t *)cmd);
}

void NVMeInterface::_init1() {
  // Step 7-3. Check active Namespace
  // We will perform I/O on first Namespace
  uint32_t count = 0;
  uint32_t nsid = 0;

  for (count = 0; count < 1024; count++) {
    adminPRP->readData(count * 4, 4, (uint8_t *)&nsid);

    if (nsid == 0) {
      break;
    }

    if (count == 0) {
      namespaceID = nsid;
    }
  }

  if (count == 0) {
    panic("This NVMe SSD does not have any namespaces.");
  }
  else if (count > 1) {
    warn("This NVMe SSD has %u namespaces.", count);
    warn("All I/O will performed on namespace ID %u.", namespaceID);
  }

  // Step 7-4. Send Identify Namespace
  // Reuse PRP here
  uint32_t cmd[16];

  memset(cmd, 0, 64);
  cmd[0] = (uint8_t)SimpleSSD::HIL::NVMe::AdminCommand::Identify;
  cmd[1] = namespaceID;
  adminPRP->getPointer(*(uint64_t *)(cmd + 6), *(uint64_t *)(cmd + 8));
  cmd[10] = (uint8_t)SimpleSSD::HIL::NVMe::IdentifyStructure::Namespace;

  initState = InitState::Phase2;

  submitCommand(0, (uint8_t *)cmd);
}

void NVMeInterface::_init2() {
  union {
    uint64_t value;
    uint8_t buffer[8];
  } temp;

  uint8_t nFormat, currentFormat;
  uint32_t formatData;

  // Step 7-4. Check structures
  adminPRP->readData(0, 8, temp.buffer);
  capacity = temp.value;

  adminPRP->readData(25, 1, &nFormat);
  nFormat++;

  adminPRP->readData(26, 1, &currentFormat);
  adminPRP->readData(128 + currentFormat * 4ull, 4, (uint8_t *)&formatData);

  LBAsize = (uint32_t)powf(2.f, (float)((formatData >> 16) & 0xFF));
  capacity *= LBAsize;

  delete adminPRP;

  info("Total SSD capacity: %" PRIu64 " bytes", capacity);
  info("Logical Block Size: %" PRIu32 " bytes", LBAsize);

  // Step 8. Determine I/O queue count
  // Step 8-1. Send Set Feature
  uint32_t cmd[16];

  memset(cmd, 0, 64);
  cmd[0] = (uint8_t)SimpleSSD::HIL::NVMe::AdminCommand::SetFeatures;
  cmd[10] = (uint8_t)SimpleSSD::HIL::NVMe::FeatureID::NumberOfQueues;
  cmd[11] = 0x00000000;  // One I/O SQ, One I/O CQ

  initState = InitState::Phase3;

  submitCommand(0, (uint8_t *)cmd);
}

void NVMeInterface::_init3(uint32_t dw0) {
  // Step 8-2. Check response
  if (dw0 != 0x00000000) {
    warn("NVMe SSD responsed too many I/O queue");
  }

  // Step 9. Allocate I/O Completion Queue
  // Step 9-1. Send Create I/O Completion Queue
  uint32_t cmd[16];
  uint16_t entries = QUEUE_ENTRY_IO;

  if (entries > maxQueueEntries) {
    entries = maxQueueEntries;
  }

  ioCQ = new Queue(entries, 16);

  memset(cmd, 0, 64);
  cmd[0] = (uint8_t)SimpleSSD::HIL::NVMe::AdminCommand::CreateIOCQ;
  ioCQ->getBaseAddress(*(uint64_t *)(cmd + 6));        // DPTR.PRP1
  cmd[10] = ((uint32_t)(entries - 1) << 16) | 0x0001;  // QSIZE, QID
  cmd[11] = 0x00010003;                                // IV, IEN, PC

  initState = InitState::Phase4;

  submitCommand(0, (uint8_t *)cmd);
}

void NVMeInterface::_init4(uint16_t status) {
  // Step 9-2. Check result
  if (status != 0) {
    panic("Failed to create I/O Completion Queue");
  }

  // Step 10. Allocate I/O Submission Queue
  // Step 10-1. Send Create I/O Submission Queue
  uint32_t cmd[16];
  uint16_t entries = QUEUE_ENTRY_IO;

  if (entries > maxQueueEntries) {
    entries = maxQueueEntries;
  }

  ioSQ = new Queue(entries, 64);

  memset(cmd, 0, 64);
  cmd[0] = (uint8_t)SimpleSSD::HIL::NVMe::AdminCommand::CreateIOSQ;
  ioSQ->getBaseAddress(*(uint64_t *)(cmd + 6));        // DPTR.PRP1
  cmd[10] = ((uint32_t)(entries - 1) << 16) | 0x0001;  // QSIZE, QID
  cmd[11] = 0x00010001;                                // CQID, QPRIO, PC

  initState = InitState::Phase5;

  submitCommand(0, (uint8_t *)cmd);
}

void NVMeInterface::_init5(uint16_t status) {
  // Step 10-2. Check result
  if (status != 0) {
    panic("Failed to create I/O Submission Queue");
  }

  info("Initialization finished");

  initState = InitState::Inited;

  // Now we initialized NVMe SSD
  scheduleNow(beginEvent);
}

void NVMeInterface::submitCommand(uint16_t iv, uint8_t *cmd) {
  uint32_t tail = 0;
  Queue *queue = nullptr;

  // Push to queue
  if (iv == 0) {
    queue = adminSQ;
  }
  else if (iv == 1 && ioSQ) {
    queue = ioSQ;
  }
  else {
    panic("I/O Submission Queue is not initialized");
  }

  queue->setData(cmd, 64);
  tail = queue->getTail();

  // Ring doorbell
  uint64_t offset = (uint64_t)SimpleSSD::HIL::NVMe::Register::DoorbellBegin;
  offset += (uint64_t)iv << 3;

  controller->write(offset, 4, (uint8_t *)&tail);
  queue->incrHead();
}

void NVMeInterface::getSSDInfo(uint64_t &bytesize, uint32_t &minbs) {
  bytesize = capacity;
  minbs = LBAsize;
}

void NVMeInterface::submit(Request &req) {
  uint32_t cmd[16];
  uint16_t length = (uint16_t)req.length;
  PRP *prp = nullptr;

  memset(cmd, 0, 64);

  cmd[1] = namespaceID;  // NSID

  if (UNLIKELY(req.length > 65535)) {
    length = 1;

    warn("Too large request. Truncate to 64K blocks (%u bytes).",
         65535 * LBAsize);
  }

  if (req.type == RequestType::Read) {
    cmd[0] = (uint8_t)SimpleSSD::HIL::NVMe::NVMCommand::Read;
    cmd[10] = (uint32_t)req.offset;
    cmd[11] = req.offset >> 32;
    cmd[12] = length - 1;  // LR, FUA, PRINFO, NLB

    prp = new PRP(length * LBAsize);
    prp->getPointer(*(uint64_t *)(cmd + 6), *(uint64_t *)(cmd + 8));  // DPTR
  }
  else if (req.type == RequestType::Write) {
    cmd[0] = (uint8_t)SimpleSSD::HIL::NVMe::NVMCommand::Write;
    cmd[10] = (uint32_t)req.offset;
    cmd[11] = req.offset >> 32;
    cmd[12] = length - 1;  // LR, FUA, PRINFO, DTYPE, NLB

    prp = new PRP(length * LBAsize);
    prp->getPointer(*(uint64_t *)(cmd + 6), *(uint64_t *)(cmd + 8));  // DPTR
  }
  else if (req.type == RequestType::Flush) {
    cmd[0] = (uint8_t)SimpleSSD::HIL::NVMe::NVMCommand::Flush;
  }
  else if (req.type == RequestType::Trim) {
    cmd[0] = (uint8_t)SimpleSSD::HIL::NVMe::NVMCommand::DatasetManagement;
    cmd[10] = 0;     // NR
    cmd[11] = 0x04;  // AD

    prp = new PRP(16);
    prp->getPointer(*(uint64_t *)(cmd + 6), *(uint64_t *)(cmd + 8));  // DPTR

    // Fill range definition
    uint8_t data[16];

    memset(data, 0, 16);
    memcpy(data + 4, &req.length, 4);
    memcpy(data + 8, &req.offset, 8);

    prp->writeData(0, 16, data);
  }

  // Write Command ID
  cmd[0] = MAKE32(req.tag, cmd[0]);

  req.setDriverData(new IOWrapper(req.tag, prp));

  submitCommand(1, (uint8_t *)cmd);
}

void NVMeInterface::callback(uint16_t iv, uint8_t *cqentry) {
  uint32_t *cqdata = (uint32_t *)cqentry;
  auto status = (uint16_t)(cqdata[3] >> 17);
  auto cid = (uint16_t)cqdata[3];

  if (UNLIKELY(iv == 0)) {
    // Admin
    switch (initState) {
      case InitState::Phase0:
        _init0();
        break;
      case InitState::Phase1:
        _init1();
        break;
      case InitState::Phase2:
        _init2();
        break;
      case InitState::Phase3:
        _init3(cqdata[0]);
        break;
      case InitState::Phase4:
        _init4(status);
        break;
      case InitState::Phase5:
        _init5(status);
        break;
      default:
        panic("Unexpected admin command completion.");
        break;
    }
  }
  else {
    auto wrapper = (IOWrapper *)parent->postCompletion(cid);

    if (status != 0) {
      warn("I/O error: %04X", status);
    }

    if (LIKELY(wrapper)) {
      delete wrapper->prp;
      delete wrapper;
    }
  }
}

void NVMeInterface::read(uint64_t addr, uint32_t size, uint8_t *buffer,
                         SimpleSSD::Event eid, uint64_t data) {
  if (size == 0) {
    warn("Zero-size DMA write request. Ignore.");

    return;
  }

  auto entry = new DMAEntry();

  entry->addr = addr;
  entry->size = size;
  entry->buffer = buffer;
  entry->eid = eid;
  entry->data = data;

  scheduler.read(entry);
}

void NVMeInterface::write(uint64_t addr, uint32_t size, uint8_t *buffer,
                          SimpleSSD::Event eid, uint64_t data) {
  if (size == 0) {
    warn("Zero-size DMA write request. Ignore.");

    return;
  }

  auto entry = new DMAEntry();

  entry->addr = addr;
  entry->size = size;
  entry->buffer = buffer;
  entry->eid = eid;
  entry->data = data;

  scheduler.write(entry);
}

void NVMeInterface::postDone(DMAEntry *entry) {
  object.engine->getInterruptFunction()(entry->eid, getTick(), entry->data);

  delete entry;
}

uint64_t NVMeInterface::preSubmitRead(DMAEntry *entry) {
  if (entry->buffer) {
    memcpy(entry->buffer, (uint8_t *)entry->addr, entry->size);
  }

  return delayFunction(entry->size);
}

uint64_t NVMeInterface::preSubmitWrite(DMAEntry *entry) {
  if (entry->buffer) {
    memcpy((uint8_t *)entry->addr, entry->buffer, entry->size);
  }

  return delayFunction(entry->size);
}

void NVMeInterface::postInterrupt(uint16_t iv, bool post) {
  uint8_t cqdata[16];

  if (post) {
    uint16_t count = 0;
    Queue *queue = nullptr;

    if (iv == 0) {
      queue = adminCQ;
    }
    else if (iv == 1 && ioCQ) {
      queue = ioCQ;
    }
    else {
      panic("I/O Completion Queue is not initialized");
    }

    // Peek queue for count how many requests are finished
    while (true) {
      queue->peekData(cqdata, 16);

      // Check phase tag
      if ((cqdata[14] & 0x01) == phase) {
        queue->incrTail();
        count++;

        // Handle CQE
        callback(iv, cqdata);

        queue->incrHead();

        if (queue->getHead() == 0) {
          // Inverted
          phase = !phase;
        }
      }
      else {
        if (count > 0) {
          uint32_t head = queue->getHead();
          uint64_t offset =
              (uint64_t)SimpleSSD::HIL::NVMe::Register::DoorbellBegin;
          offset += (1 + ((uint64_t)iv << 1)) << 2;

          controller->write(offset, 4, (uint8_t *)&head);
        }

        break;
      }
    }
  }
}

void NVMeInterface::getPCIID(uint16_t &vid, uint16_t &ssvid) {
  // Copied from SimpleSSD-FullSystem
  vid = 0x144D;
  ssvid = 0x8086;
}

void NVMeInterface::getStatList(std::vector<SimpleSSD::Stat> &list) {
  simplessd.getStatList(list, "");
}

void NVMeInterface::getStatValues(std::vector<double> &values) {
  simplessd.getStatValues(values);
}

}  // namespace Standalone::Driver::NVMe
