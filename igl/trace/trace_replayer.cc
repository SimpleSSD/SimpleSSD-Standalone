/*
 * Copyright (C) 2017 CAMELab
 *
 * This file is part of SimpleSSD.
 *
 * SimpleSSD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSSD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimpleSSD.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "igl/trace/trace_replayer.hh"

#include "simplessd/sim/trace.hh"
#include "simplessd/util/algorithm.hh"

namespace IGL {

TraceReplayer::TraceReplayer(Engine &e, BIL::BlockIOEntry &b,
                             std::function<void()> &f, ConfigReader &c)
    : IOGenerator(e, b, f), reserveTermination(false), iodepth(0) {
  // Check file
  auto filename = c.readString(CONFIG_TRACE, TRACE_FILE);
  file.open(filename);

  if (!file.is_open()) {
    SimpleSSD::panic("Failed to open trace file %s!", filename.c_str());
  }

  // Create regex
  try {
    regex = std::regex(c.readString(CONFIG_TRACE, TRACE_LINE_REGEX));
  }
  catch (std::regex_error e) {
    SimpleSSD::panic("Invalid regular expression!");
  }

  // Fill flags
  groupID[ID_OPERATION] = c.readUint(CONFIG_TRACE, TRACE_GROUP_OPERATION);
  groupID[ID_BYTE_OFFSET] = c.readUint(CONFIG_TRACE, TRACE_GROUP_BYTE_OFFSET);
  groupID[ID_BYTE_LENGTH] = c.readUint(CONFIG_TRACE, TRACE_GROUP_BYTE_LENGTH);
  groupID[ID_LBA_OFFSET] = c.readUint(CONFIG_TRACE, TRACE_GROUP_LBA_OFFSET);
  groupID[ID_LBA_LENGTH] = c.readUint(CONFIG_TRACE, TRACE_GROUP_LBA_LENGTH);
  groupID[ID_TIME_SEC] = c.readUint(CONFIG_TRACE, TRACE_GROUP_SEC);
  groupID[ID_TIME_MS] = c.readUint(CONFIG_TRACE, TRACE_GROUP_MILI_SEC);
  groupID[ID_TIME_US] = c.readUint(CONFIG_TRACE, TRACE_GROUP_MICRO_SEC);
  groupID[ID_TIME_NS] = c.readUint(CONFIG_TRACE, TRACE_GROUP_NANO_SEC);
  groupID[ID_TIME_PS] = c.readUint(CONFIG_TRACE, TRACE_GROUP_PICO_SEC);
  useHex = c.readBoolean(CONFIG_TRACE, TRACE_USE_HEX);

  if (groupID[ID_OPERATION] == 0) {
    SimpleSSD::panic("Operation group ID cannot be 0");
  }

  if (groupID[ID_LBA_OFFSET] > 0 && groupID[ID_LBA_LENGTH] > 0) {
    useLBA = true;
    lbaSize = c.readUint(CONFIG_TRACE, TRACE_LBA_SIZE);

    if (SimpleSSD::popcount(lbaSize) != 1) {
      SimpleSSD::panic("LBA size should be power of 2");
    }
  }
  else if (groupID[ID_BYTE_OFFSET] > 0 && groupID[ID_BYTE_LENGTH] > 0) {
    useLBA = false;
  }
  else {
    SimpleSSD::panic("Group ID of offset and length are invalid");
  }

  timeValids[0] = groupID[ID_TIME_SEC] > 0 ? true : false;
  timeValids[1] = groupID[ID_TIME_MS] > 0 ? true : false;
  timeValids[2] = groupID[ID_TIME_US] > 0 ? true : false;
  timeValids[3] = groupID[ID_TIME_NS] > 0 ? true : false;
  timeValids[4] = groupID[ID_TIME_PS] > 0 ? true : false;

  if (!(timeValids[0] || timeValids[1] || timeValids[2] || timeValids[3] ||
        timeValids[4])) {
    SimpleSSD::panic("No valid time field specified");
  }

  submitIO = [this](uint64_t tick) { _submitIO(tick); };
  iocallback = [this](uint64_t id) { _iocallback(id); };

  submitEvent = engine.allocateEvent(submitIO);
}

TraceReplayer::~TraceReplayer() {
  file.close();
}

void TraceReplayer::init(uint64_t bytesize, uint32_t bs) {
  ssdSize = bytesize;
  blocksize = bs;

  if (useLBA && lbaSize < bs) {
    SimpleSSD::warn("LBA size of trace file is smaller than SSD's LBA size");
  }
}

void TraceReplayer::begin() {
  initTime = engine.getCurrentTick();

  handleNextLine(true);
}

uint64_t TraceReplayer::mergeTime(std::smatch &match) {
  uint64_t tick = 0;
  bool valid = true;

  if (timeValids[0] && match.size() > groupID[ID_TIME_SEC]) {
    tick += strtoul(match[groupID[ID_TIME_SEC]].str().c_str(), nullptr, 10) *
            1000000000000ULL;
  }
  else if (timeValids[0]) {
    valid = false;
  }

  if (timeValids[1] && match.size() > groupID[ID_TIME_MS]) {
    tick += strtoul(match[groupID[ID_TIME_MS]].str().c_str(), nullptr, 10) *
            1000000000ULL;
  }
  else if (timeValids[1]) {
    valid = false;
  }

  if (timeValids[2] && match.size() > groupID[ID_TIME_US]) {
    tick += strtoul(match[groupID[ID_TIME_US]].str().c_str(), nullptr, 10) *
            1000000ULL;
  }
  else if (timeValids[2]) {
    valid = false;
  }

  if (timeValids[3] && match.size() > groupID[ID_TIME_NS]) {
    tick += strtoul(match[groupID[ID_TIME_NS]].str().c_str(), nullptr, 10) *
            1000ULL;
  }
  else if (timeValids[3]) {
    valid = false;
  }

  if (timeValids[4] && match.size() > groupID[ID_TIME_PS]) {
    tick += strtoul(match[groupID[ID_TIME_PS]].str().c_str(), nullptr, 10);
  }
  else if (timeValids[4]) {
    valid = false;
  }

  if (!valid) {
    SimpleSSD::panic("Time parse failed");
  }

  return tick;
}

BIL::BIO_TYPE TraceReplayer::getType(std::string type) {
  switch (type[0]) {
    case 'r':
    case 'R':
      return BIL::BIO_READ;
    case 'w':
    case 'W':
      return BIL::BIO_WRITE;
    case 'f':
    case 'F':
      return BIL::BIO_FLUSH;
    case 't':
    case 'T':
      return BIL::BIO_TRIM;
  }

  return BIL::BIO_NUM;
}

void TraceReplayer::handleNextLine(bool begin) {
  std::string line;
  std::smatch match;

  if (reserveTermination) {
    // Nothing to do
    return;
  }

  // Read line
  while (true) {
    if (file.eof()) {
      reserveTermination = true;

      if (iodepth == 0) {
        // No on-the-fly I/O
        endCallback();
      }

      return;
    }

    std::getline(file, line);

    if (std::regex_match(line, match, regex)) {
      break;
    }
  }

  // Get time
  uint64_t tick = mergeTime(match);

  if (begin) {
    firstTick = tick;
  }

  // Fill BIO
  bio.id++;  // BIO.id is inited as 0

  if (useLBA) {
    bio.offset = strtoul(match[groupID[ID_LBA_OFFSET]].str().c_str(), nullptr,
                         useHex ? 16 : 10) *
                 lbaSize;
    bio.length = strtoul(match[groupID[ID_LBA_LENGTH]].str().c_str(), nullptr,
                         useHex ? 16 : 10) *
                 lbaSize;
  }
  else {
    bio.offset = strtoul(match[groupID[ID_BYTE_OFFSET]].str().c_str(), nullptr,
                         useHex ? 16 : 10);
    bio.length = strtoul(match[groupID[ID_BYTE_LENGTH]].str().c_str(), nullptr,
                         useHex ? 16 : 10);
  }

  bio.type = getType(match[groupID[ID_OPERATION]].str());
  bio.callback = iocallback;

  // Range check
  if (bio.offset + bio.length > ssdSize) {
    SimpleSSD::warn("I/O %" PRIu64 ": I/O out of range", bio.id);

    while (bio.offset >= ssdSize) {
      bio.offset -= ssdSize;
    }

    if (bio.offset + bio.length > ssdSize) {
      bio.length = ssdSize - bio.offset;
    }
  }

  // Schedule
  engine.scheduleEvent(submitEvent, tick - firstTick + initTime);
}

void TraceReplayer::_submitIO(uint64_t) {
  iodepth++;

  bioEntry.submitIO(bio);

  // Read next
  handleNextLine();
}

void TraceReplayer::_iocallback(uint64_t) {
  iodepth--;

  if (reserveTermination && iodepth == 0) {
    // Everything is done
    endCallback();
  }
}

}  // namespace IGL
