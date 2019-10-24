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

#include <fstream>
#include <iostream>
#include <thread>

#include "bil/entry.hh"
#include "igl/request/request_generator.hh"
#include "igl/trace/trace_replayer.hh"
#include "main/engine.hh"
#include "main/object.hh"
#include "main/signal.hh"
#include "sil/nvme/nvme.hh"
#include "simplessd/sim/simplessd.hh"
#include "util/print.hh"

using namespace Standalone;

// Global objects
ObjectData standaloneObject;
EventEngine engine;
ConfigReader simConfig;
SimpleSSD::SimpleSSD simplessd;
SimpleSSD::ConfigReader ssdConfig;
BIL::DriverInterface *pInterface = nullptr;
BIL::BlockIOEntry *pBIOEntry = nullptr;
IGL::IOGenerator *pIOGen = nullptr;
std::ostream *pLog = nullptr;
std::ostream *pDebugLog = nullptr;
std::ostream *pLatencyFile = nullptr;
std::thread *pThread = nullptr;
std::mutex killLock;
Event statEvent;
std::vector<SimpleSSD::Stat> statList;
std::ofstream logOut;
std::ofstream debugLogOut;
std::ofstream latencyFile;

// Declaration
void cleanup(int);
void statistics(uint64_t);
void threadFunc(int);

void joinPath(std::string &lhs, std::string &rhs) {
  if (rhs.front() == '/') {
    // Assume absolute path
    lhs = rhs;
  }
  else if (lhs.back() == '/') {
    lhs += rhs;
  }
  else {
    lhs += '/';
    lhs += rhs;
  }
}

int main(int argc, char *argv[]) {
  std::cout << "SimpleSSD Standalone v2.0" << std::endl;

  // Check argument
  if (argc != 4) {
    std::cerr << " Invalid number of argument!" << std::endl;
    std::cerr << "  Usage: simplessd-standalone <Simulation configuration "
                 "file> <SimpleSSD configuration file> <Output directory>"
              << std::endl;

    return 1;
  }

  // Install signal handler
  installSignalHandler(cleanup);

  // Read simulation config file
  simConfig.load(argv[1]);
  ssdConfig.load(argv[2]);

  // Log setting
  bool noLogPrintOnScreen = true;

  std::string logPath =
      simConfig.readString(Section::Simulation, Config::Key::StatFile);
  std::string debugLogPath =
      simConfig.readString(Section::Simulation, Config::Key::DebugFile);
  std::string latencyLogPath =
      simConfig.readString(Section::Simulation, Config::Key::Latencyfile);

  ssdConfig.writeString(SimpleSSD::Section::Simulation,
                        SimpleSSD::Config::Key::OutputDirectory, argv[3]);
  ssdConfig.writeString(SimpleSSD::Section::Simulation,
                        SimpleSSD::Config::Key::DebugFile, debugLogPath);
  ssdConfig.writeString(SimpleSSD::Section::Simulation,
                        SimpleSSD::Config::Key::OutputFile, debugLogPath);

  if (logPath.compare("STDOUT") == 0) {
    noLogPrintOnScreen = false;
    pLog = &std::cout;
  }
  else if (logPath.compare("STDERR") == 0) {
    noLogPrintOnScreen = false;
    pLog = &std::cerr;
  }
  else if (logPath.length() != 0) {
    std::string full(argv[3]);

    joinPath(full, logPath);
    logOut.open(full);

    if (!logOut.is_open()) {
      std::cerr << " Failed to open log file: " << full << std::endl;

      return 3;
    }

    pLog = &logOut;
  }

  if (debugLogPath.compare("STDOUT") == 0 ||
      debugLogPath.compare("STDERR") == 0) {
    noLogPrintOnScreen = false;
  }

  if (latencyLogPath.length() != 0) {
    std::string full(argv[3]);

    joinPath(full, latencyLogPath);
    latencyFile.open(full);

    if (!latencyFile.is_open()) {
      std::cerr << " Failed to open log file: " << full << std::endl;

      return 3;
    }

    pLatencyFile = &latencyFile;
  }

  // Store config file
  {
    std::string full(argv[3]);
    std::string name("standalone.xml");

    joinPath(full, name);

    simConfig.save(full);
  }
  {
    std::string full(argv[3]);
    std::string name("simplessd.xml");

    joinPath(full, name);

    ssdConfig.save(full);
  }

  // Initialize SimpleSSD
  simplessd.init(&engine, &ssdConfig);

  // Create Driver
  standaloneObject.engine = &engine;
  standaloneObject.config = &simConfig;
  standaloneObject.log = simplessd.getObject().log;

  switch ((Config::InterfaceType)simConfig.readUint(Section::Simulation,
                                                    Config::Key::Interface)) {
    case Config::InterfaceType::NVMe:
      pInterface = new SIL::NVMe::Driver(standaloneObject, simplessd);

      break;
    default:
      std::cerr << " Undefined interface specified." << std::endl;

      return 4;
  }

  // Create Block I/O Layer
  pBIOEntry = new BIL::BlockIOEntry(standaloneObject, pInterface, pLatencyFile);

  Event endCallback = engine.createEvent(
      [](uint64_t, uint64_t) {
        // If stat printout is scheduled, delete it
        if (simConfig.readUint(Section::Simulation, Config::Key::StatPeriod) >
            0) {
          engine.deschedule(statEvent);
        }

        // Stop simulation
        engine.stopEngine();
      },
      "endCallback");

  // Create I/O generator
  switch ((Config::ModeType)simConfig.readUint(Section::Simulation,
                                               Config::Key::Mode)) {
    case Config::ModeType::RequestGenerator:
      pIOGen =
          new IGL::RequestGenerator(standaloneObject, *pBIOEntry, endCallback);

      break;
    case Config::ModeType::TraceReplayer:
      pIOGen =
          new IGL::TraceReplayer(standaloneObject, *pBIOEntry, endCallback);

      break;
    default:
      std::cerr << " Undefined simulation mode specified." << std::endl;

      cleanup(0);

      return 5;
  }

  Event beginCallback = engine.createEvent(
      [](uint64_t, uint64_t) {
        uint64_t bytesize;
        uint32_t bs;

        pInterface->getInfo(bytesize, bs);
        pIOGen->init(bytesize, bs);
        pIOGen->begin();
      },
      "beginCallback");

  // Insert stat event
  pInterface->initStats(statList);

  if (simConfig.readUint(Section::Simulation, Config::Key::StatPeriod) > 0) {
    uint64_t period =
        simConfig.readUint(Section::Simulation, Config::Key::StatPeriod);

    statEvent = engine.createEvent(
        [period](uint64_t tick, uint64_t) {
          statistics(tick);

          engine.schedule(statEvent, 0ull, tick + period);
        },
        "statEvent");

    engine.schedule(statEvent, 0ull, period);
  }

  // Do Simulation
  std::cout << "********** Begin of simulation **********" << std::endl;

  pInterface->init(beginCallback);

  if (noLogPrintOnScreen) {
    int period = (int)simConfig.readUint(Section::Simulation,
                                         Config::Key::ProgressPeriod);

    if (period > 0) {
      pThread = new std::thread(threadFunc, period);
    }
  }

  while (engine.doNextEvent())
    ;

  cleanup(0);

  return 0;
}

void cleanup(int sig) {
  uint64_t tick;

  if (sig != 0) {
    std::cout << std::endl << "Simulation terminated with signal." << std::endl;
  }

  killLock.lock();

  tick = engine.getTick_unsafe();

  if (tick == 0) {
    // Exit program
    exit(0);
  }

  // Print last statistics
  statistics(tick);

  // Erase progress
  printf("\33[2K                                                           \r");

  pIOGen->printStats(std::cout);
  engine.printStats(std::cout);

  // Cleanup all here
  delete pInterface;
  delete pIOGen;

  if (pThread) {
    pThread->join();

    delete pThread;
  }

  killLock.unlock();

  delete pBIOEntry;  // Used by progress thread

  if (logOut.is_open()) {
    logOut.close();
  }
  if (debugLogOut.is_open()) {
    debugLogOut.close();
  }

  std::cout << "End of simulation @ tick " << tick << std::endl;

  // Exit program
  exit(0);
}

void statistics(uint64_t tick) {
  if (pLog == nullptr) {
    return;
  }

  std::ostream &out = *pLog;
  std::vector<double> stat;
  uint64_t count = 0;

  pInterface->getStats(stat);

  count = statList.size();

  if (count != stat.size()) {
    std::cerr << " Stat list length mismatch" << std::endl;

    std::terminate();
  }

  out << "Periodic log printout @ tick " << tick << std::endl;

  for (uint64_t i = 0; i < count; i++) {
    print(out, statList[i].name, 40);
    out << "\t";
    print(out, stat[i], 20);
    out << "\t" << statList[i].desc << std::endl;
  }

  out << "End of log @ tick " << tick << std::endl;
}

void threadFunc(int tick) {
  uint64_t current = 0.f;
  uint64_t old = 0;
  float progress = 0.f;
  auto duration = std::chrono::seconds(tick);
  BIL::Progress data;

  // Block SIGINT
  blockSIGINT();

  while (true) {
    std::this_thread::sleep_for(duration);

    if (killLock.try_lock()) {
      killLock.unlock();
    }
    else {
      break;
    }

    engine.getStat(current);
    pIOGen->getProgress(progress);
    pBIOEntry->getProgress(data);

    printf("\33[2K*** Progress: %.2f%% (%lf ops) IOPS: %" PRIu64 " BW: %" PRIu64
           " B/s Avg. Lat: %" PRIu64 " ps\r",
           progress * 100.f, (double)(current - old) / tick, data.iops,
           data.bandwidth, data.latency);
    fflush(stdout);

    old = current;
  }
}
