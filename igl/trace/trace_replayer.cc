// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "igl/trace/trace_replayer.hh"

#include "simplessd/util/algorithm.hh"
#include "util/print.hh"

namespace Standalone::IGL {

TraceReplayer::TraceReplayer(ObjectData &o, BlockIOLayer &b, Event e)
    : AbstractIOGenerator(o, b, e),
      useLBAOffset(false),
      useLBALength(false),
      reserveTermination(false),
      forceSubmit(false),
      delayed(0),
      io_submitted(0),
      io_count(0),
      read_count(0),
      write_count(0),
      io_depth(0),
      io_busy(0),
      io_busy_start(0) {
  // Check file
  auto filename =
      readConfigString(Section::TraceReplayer, TraceConfig::Key::File);
  file.open(filename);

  if (!file.is_open()) {
    panic("Failed to open trace file %s!", filename.c_str());
  }

  file.seekg(0, std::ios::end);
  fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  // Create regex
  try {
    regex = std::regex(
        readConfigString(Section::TraceReplayer, TraceConfig::Key::Regex));
  }
  catch (std::regex_error &) {
    panic("Invalid regular expression!");
  }

  // Fill flags
  mode = (TraceConfig::TimingModeType)readConfigUint(
      Section::TraceReplayer, TraceConfig::Key::TimingMode);
  maxQueueDepth =
      (uint32_t)readConfigUint(Section::TraceReplayer, TraceConfig::Key::Depth);
  max_io_count =
      readConfigUint(Section::TraceReplayer, TraceConfig::Key::CountLimit);
  max_io_size =
      readConfigUint(Section::TraceReplayer, TraceConfig::Key::SizeLimit);
  groupID[ID_OPERATION] = (uint32_t)readConfigUint(
      Section::TraceReplayer, TraceConfig::Key::GroupOperation);
  groupID[ID_BYTE_OFFSET] = (uint32_t)readConfigUint(
      Section::TraceReplayer, TraceConfig::Key::GroupByteOffset);
  groupID[ID_BYTE_LENGTH] = (uint32_t)readConfigUint(
      Section::TraceReplayer, TraceConfig::Key::GroupByteLength);
  groupID[ID_LBA_OFFSET] = (uint32_t)readConfigUint(
      Section::TraceReplayer, TraceConfig::Key::GroupLBAOffset);
  groupID[ID_LBA_LENGTH] = (uint32_t)readConfigUint(
      Section::TraceReplayer, TraceConfig::Key::GroupLBALength);
  groupID[ID_TIME_SEC] = (uint32_t)readConfigUint(
      Section::TraceReplayer, TraceConfig::Key::GroupSecond);
  groupID[ID_TIME_MS] = (uint32_t)readConfigUint(
      Section::TraceReplayer, TraceConfig::Key::GroupMiliSecond);
  groupID[ID_TIME_US] = (uint32_t)readConfigUint(
      Section::TraceReplayer, TraceConfig::Key::GroupMicroSecond);
  groupID[ID_TIME_NS] = (uint32_t)readConfigUint(
      Section::TraceReplayer, TraceConfig::Key::GroupNanoSecond);
  groupID[ID_TIME_PS] = (uint32_t)readConfigUint(
      Section::TraceReplayer, TraceConfig::Key::GroupPicoSecond);
  useHex = readConfigBoolean(Section::TraceReplayer,
                             TraceConfig::Key::UseHexadecimal);
  useWrap =
      readConfigBoolean(Section::TraceReplayer, TraceConfig::Key::WrapTrace);

  if (groupID[ID_OPERATION] == 0) {
    panic("Operation group ID cannot be 0");
  }

  if (groupID[ID_LBA_OFFSET] > 0) {
    useLBAOffset = true;
  }
  if (groupID[ID_LBA_LENGTH] > 0) {
    useLBALength = true;
  }

  if (useLBALength || useLBAOffset) {
    lbaSize = (uint32_t)readConfigUint(Section::TraceReplayer,
                                       TraceConfig::Key::LBASize);

    if (popcount32(lbaSize) != 1) {
      panic("LBA size should be power of 2");
    }
  }

  if (!useLBAOffset && groupID[ID_BYTE_OFFSET] == 0) {
    panic("Both LBA Offset and Byte Offset group ID cannot be 0");
  }
  if (!useLBALength && groupID[ID_BYTE_LENGTH] == 0) {
    panic("Both LBA Length and Byte Length group ID cannot be 0");
  }

  timeValids[0] = groupID[ID_TIME_SEC] > 0 ? true : false;
  timeValids[1] = groupID[ID_TIME_MS] > 0 ? true : false;
  timeValids[2] = groupID[ID_TIME_US] > 0 ? true : false;
  timeValids[3] = groupID[ID_TIME_NS] > 0 ? true : false;
  timeValids[4] = groupID[ID_TIME_PS] > 0 ? true : false;

  if (!(timeValids[0] || timeValids[1] || timeValids[2] || timeValids[3] ||
        timeValids[4])) {
    if (mode == TraceConfig::TimingModeType::Strict) {
      panic("No valid time field specified");
    }
  }

  firstTick = std::numeric_limits<uint64_t>::max();

  submitEvent = createEvent([this](uint64_t, uint64_t) { submitIO(); },
                            "IGL::TraceReplayer::submitEvent");
  completionEvent = createEvent([this](uint64_t t, uint64_t) { iocallback(t); },
                                "IGL::TraceReplayer::completionEvent");

  auto submissionLatency =
      readConfigUint(Section::Simulation, Config::Key::SubmissionLatency);
  auto completionLatency =
      readConfigUint(Section::Simulation, Config::Key::CompletionLatency);

  bioEntry.initialize(maxQueueDepth, submissionLatency, completionLatency,
                      completionEvent);
}

TraceReplayer::~TraceReplayer() {
  file.close();
}

void TraceReplayer::initialize() {
  bioEntry.getSSDSize(ssdSize, blocksize);

  if ((useLBALength || useLBAOffset) && lbaSize < blocksize) {
    warn("LBA size of trace file is smaller than SSD's LBA size");
  }
}

void TraceReplayer::begin() {
  initTime = getTick();

  parseLine();

  if (mode == TraceConfig::TimingModeType::Strict) {
    firstTick = linedata.tick;
  }
  else {
    firstTick = initTime;
  }

  if (UNLIKELY(reserveTermination)) {
    warn("No I/O submitted. Check regular expression.");

    scheduleAbs(endCallback, 0, initTime);
  }
  else {
    submitIO();
  }
}

void TraceReplayer::printStats(std::ostream &out) {
  uint64_t tick = getTick();

  out << "*** Statistics of Trace Replayer ***" << std::endl;
  out << "Replay mode: ";

  switch (mode) {
    case TraceConfig::TimingModeType::Synchronous:
      out << "Sync";
      break;
    case TraceConfig::TimingModeType::Asynchronous:
      out << "Async (depth: " << maxQueueDepth << ")";
      break;
    case TraceConfig::TimingModeType::Strict:
      out << "Strict (depth: " << maxQueueDepth << ")";
      break;
  }

  out << std::endl << "Termination mode: ";

  if (max_io_count == 0 && max_io_size == 0) {
    out << "None - Replay until end of trace.";
  }
  else if (max_io_count > 0 && max_io_size > 0) {
    out << "Until " << max_io_size << " bytes or " << max_io_count
        << " requests issued.";
  }
  else if (max_io_count > 0) {
    out << "Until " << max_io_count << " requests issued.";
  }
  else {
    out << "Until " << max_io_size << " bytes issued.";
  }

  out << std::endl << "Tick: " << tick << std::endl;
  out << "Time (ps): " << firstTick - initTime << " - " << tick << " ("
      << tick + firstTick - initTime << ")" << std::endl;
  out << "Busy time (ps): " << io_busy << std::endl;
  out << "I/O (bytes): " << io_submitted << " (idle=";

  printBandwidth(out, io_submitted, tick - initTime);

  out << ") (busy=";

  printBandwidth(out, io_submitted, io_busy);

  out << ")" << std::endl;
  out << "I/O (counts): " << io_count << " (Read: " << read_count
      << ", Write: " << write_count << ")" << std::endl;
  out << "*** End of statistics ***" << std::endl;

  bioEntry.printStats(out);
}

void TraceReplayer::getProgress(float &val) {
  if (max_io_count == 0 && max_io_size == 0) {
    // If I/O count is unlimited, use file pointer for fast progress calculation
    uint64_t ptr;

    {
      std::lock_guard<std::mutex> guard(m);
      ptr = file.tellg();
    }

    val = (float)ptr / fileSize;
  }
  else if (max_io_size > 0) {
    std::lock_guard<std::mutex> guard(m);

    // Use submitted I/O count in progress calculation
    // If trace file contains I/O requests smaller than max_io, progress value
    // cannot reach 1.0 (100%)
    val = (float)io_submitted / max_io_size;
  }
  else {
    std::lock_guard<std::mutex> guard(m);

    val = (float)io_count / max_io_count;
  }
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
    panic("Time parse failed");
  }

  return tick;
}

Driver::RequestType TraceReplayer::getType(std::string type) {
  switch (type[0]) {
    case 'r':
    case 'R':
      return Driver::RequestType::Read;
    case 'w':
    case 'W':
      return Driver::RequestType::Write;
    case 'f':
    case 'F':
      return Driver::RequestType::Flush;
    case 't':
    case 'T':
    case 'd':
    case 'D':
      return Driver::RequestType::Trim;
  }

  return Driver::RequestType::None;
}

void TraceReplayer::parseLine() {
  std::string line;
  std::smatch match;

  // Read line
  while (true) {
    bool eof = false;

    {
      std::lock_guard<std::mutex> guard(m);

      eof = file.eof();
      std::getline(file, line);
    }

    if (eof) {
      if (useWrap && (io_count < max_io_count || io_submitted < max_io_size)) {
        file.clear();                  // Clear fail
        file.seekg(0, std::ios::beg);  // Clear eof

        // Handle timing calculation
        delayed += linedata.tick - firstTick;

        continue;
      }

      reserveTermination = true;

      if (io_depth == 0) {
        // No on-the-fly I/O
        scheduleNow(endCallback);
      }

      return;
    }
    if (std::regex_match(line, match, regex)) {
      break;
    }
  }

  linedata.tick = mergeTime(match);

  if (useLBAOffset) {
    linedata.offset = strtoul(match[groupID[ID_LBA_OFFSET]].str().c_str(),
                              nullptr, useHex ? 16 : 10) *
                      lbaSize;
  }
  else {
    linedata.offset = strtoul(match[groupID[ID_BYTE_OFFSET]].str().c_str(),
                              nullptr, useHex ? 16 : 10);
  }

  if (useLBALength) {
    linedata.length = strtoul(match[groupID[ID_LBA_LENGTH]].str().c_str(),
                              nullptr, useHex ? 16 : 10) *
                      lbaSize;
  }
  else {
    linedata.length = strtoul(match[groupID[ID_BYTE_LENGTH]].str().c_str(),
                              nullptr, useHex ? 16 : 10);
  }

  // This function increases I/O count
  linedata.type = getType(match[groupID[ID_OPERATION]].str());
}

void TraceReplayer::submitIO() {
  uint16_t tag;

  panic_if(linedata.type == Driver::RequestType::None,
           "Unexpected request type.");

  auto ret = bioEntry.submitRequest(linedata.type, linedata.offset,
                                    linedata.length, tag);

  if (mode == TraceConfig::TimingModeType::Strict && !ret) {
    // Delay request submission
    forceSubmit = true;

    return;
  }

  panic_if(!ret, "BUG!");

  if (io_depth == 0) {
    io_busy_start = getTick();
  }

  // Stat
  io_submitted += linedata.length;
  io_depth++;
  io_count++;

  if (linedata.type == Driver::RequestType::Read) {
    read_count++;
  }
  else if (linedata.type == Driver::RequestType::Write) {
    write_count++;
  }

  // Limit check
  if (UNLIKELY((max_io_size != 0 && io_submitted >= max_io_size) ||
               (max_io_count != 0 && io_count >= max_io_count))) {
    reserveTermination = true;

    return;
  }

  // Get next line
  parseLine();

  // Are we done?
  if (UNLIKELY(reserveTermination)) {
    return;
  }

  // Reschedule submission
  switch (mode) {
    case TraceConfig::TimingModeType::Strict:
      scheduleAbs(submitEvent, 0ull,
                  linedata.tick - firstTick + initTime + delayed);
      break;
    case TraceConfig::TimingModeType::Asynchronous:
      rescheduleSubmit();
      break;
    default:
      break;
  }
}

void TraceReplayer::iocallback(uint64_t now) {
  io_depth--;

  if (io_depth == 0) {
    io_busy += now - io_busy_start;
  }

  if (reserveTermination) {
    // Everything is done
    if (io_depth == 0) {
      scheduleAbs(endCallback, 0ull, now);
    }

    return;
  }

  if (mode == TraceConfig::TimingModeType::Synchronous || forceSubmit) {
    rescheduleSubmit();

    // Async / Strict submission blocked by I/O depth limitation
    // Let's submit here
    forceSubmit = false;
  }
}

void TraceReplayer::rescheduleSubmit() {
  if (mode == TraceConfig::TimingModeType::Asynchronous) {
    if (io_depth >= maxQueueDepth) {
      forceSubmit = true;

      return;
    }
  }
  else if (mode == TraceConfig::TimingModeType::Strict) {
    if (forceSubmit) {
      delayed = getTick() - (linedata.tick - firstTick + initTime);
    }
    else {
      return;
    }
  }

  scheduleNow(submitEvent);
}

}  // namespace Standalone::IGL
