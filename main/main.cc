// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 CAMELab
 *
 * Author: Donghyun Gouk <kukdh1@camelab.org>
 */

#include <fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "driver/none/none.hh"
#include "driver/nvme/nvme.hh"
#include "igl/request/request_generator.hh"
#include "igl/trace/trace_replayer.hh"
#include "main/engine.hh"
#include "main/object.hh"
#include "main/signal.hh"
#include "main/version.hh"
#include "simplessd/sim/simplessd.hh"
#include "simplessd/sim/version.hh"
#include "util/argparse.hh"
#include "util/print.hh"

using namespace Standalone;

static const std::regex regex_override("((?:\\[\\w+\\])+)(\\w+)=(.+)");

static const char *OPT_REDIRECT[2] = {"--redirect", "-r"};
static const char *OPT_CREATE_CKP[2] = {"--create-checkpoint", "-C"};
static const char *OPT_RESTORE_CKP[2] = {"--restore-checkpoint", "-R"};
static const char *OPT_CFG_OVERRIDE[2] = {"--config-override", "-O"};
static const char *OPT_VERSION[2] = {"--version", "-v"};
static const char *OPT_HELP[2] = {"--help", "-h"};

// Global objects
ObjectData standaloneObject;
EventEngine engine;
ConfigReader simConfig;
SimpleSSD::SimpleSSD simplessd;
SimpleSSD::ConfigReader ssdConfig;
Standalone::Driver::AbstractInterface *pInterface = nullptr;
IGL::BlockIOLayer *pBIOEntry = nullptr;
IGL::AbstractIOGenerator *pIOGen = nullptr;
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
int stdoutCopy = fileno(stdout);

// Declaration
void cleanup(int);
void progress(int);
void statistics(uint64_t, bool = false);
void threadFunc(int);

void printArg(const char *arg[], const char *optname = nullptr,
              const char *desc = nullptr) {
  int cnt = 4 + strlen(arg[SHORT_OPT]) + strlen(arg[LONG_OPT]);

  printf("  %s, %s", arg[SHORT_OPT], arg[LONG_OPT]);

  if (optname) {
    cnt += 1 + strlen(optname);

    printf("=%s", optname);
  }

  for (int i = cnt; i < 40; i++) {
    putchar(' ');
  }

  if (desc) {
    printf("%s\n", desc);
  }
}

void printHelp() {
  std::cout << " Usage: simplessd-standalone [options] <Simulation config> "
               "<SimpleSSD config> <Output directory>"
            << std::endl;
  std::cout << std::endl;

  std::cout << "Output control:" << std::endl;
  printArg(OPT_REDIRECT, nullptr, "Redirect stdout and stderr to simout file.");
  std::cout << std::endl;

  std::cout << "Checkpoint feature:" << std::endl;
  printArg(OPT_CREATE_CKP, "<dir>",
           "Create checkpoint to the directory right after initialization.");
  printArg(OPT_RESTORE_CKP, "<dir>",
           "Restore from checkpoint stored in directory.");
  std::cout << std::endl;

  std::cout << "Configuration override:" << std::endl;
  printArg(OPT_CFG_OVERRIDE, "<option>", "Override configuration.");
  std::cout << "    <option> should be formatted as:" << std::endl;
  std::cout << "       [sim/ssd][section]...config=value" << std::endl;
  std::cout << "    Example: [ssd][memory][system]BusClock=100m" << std::endl;
  std::cout << "                 to change system bus clock speed to 100MHz."
            << std::endl;
  std::cout << "             [sim][sim]Mode=1" << std::endl;
  std::cout << "                 to change simulation mode to TraceReplayer."
            << std::endl;
  std::cout << "    Wrap with \"\" if option contains any whitespaces."
            << std::endl;
  std::cout << std::endl;

  std::cout << "Miscellaneous:" << std::endl;
  printArg(OPT_VERSION, nullptr, "Print version.");
  printArg(OPT_HELP, nullptr, "Print this help.");
  std::cout << std::endl;
}

void printVersion() {
  std::cout << " SimpleSSD-Standalone version: " << SIMPLESSD_STANDALONE_VERSION
            << std::endl;
  std::cout << " SimpleSSD version: " << SimpleSSD::SIMPLESSD_VERSION
            << std::endl;
}

const char *getEndOfSectionName(const char *begin) {
  if (*begin != '[') {
    return nullptr;
  }

  while (*begin != ']') {
    begin++;
  }

  return begin;
}

bool overrideConfig(pugi::xml_node &root, const char *str, bool simcfg) {
  std::cmatch match;

  if ((simcfg && strncmp(str, "[ssd]", 5) == 0) ||
      (!simcfg && strncmp(str, "[sim]", 5) == 0)) {
    // Skip
    return true;
  }

  if ((simcfg && strncmp(str, "[sim]", 5) != 0) ||
      (!simcfg && strncmp(str, "[ssd]", 5) != 0)) {
    std::cerr << "Unexpected prefix. Option must start with [sim] or [ssd].";

    return false;
  }

  // Parse
  if (std::regex_match(str + 5, match, regex_override)) {
    auto &sections = match[1];
    auto &key = match[2];
    auto &value = match[3];

    // Find config node
    auto node = root;
    auto begin = sections.first;

    while (true) {
      if (*begin != '[') {
        break;
      }

      auto end = getEndOfSectionName(begin);

      if (end > sections.second) {
        break;
      }

      std::string section(begin + 1, end);
      begin = end + 1;

      node = node.find_child([&section](pugi::xml_node node) -> bool {
        if (strcmp(node.name(), CONFIG_SECTION_NAME) == 0 &&
            strcmp(node.attribute(CONFIG_ATTRIBUTE).value(), section.c_str()) ==
                0) {
          return true;
        }

        return false;
      });

      if (!node) {
        std::cerr << "Failed to find configuration with " << str << std::endl;
        std::cerr << " Failed to find <section name=\"" << section << "\">"
                  << std::endl;

        return false;
      }
    }

    // Check config name
    std::string keystr(key.first, key.second);
    node = node.find_child([&keystr](pugi::xml_node node) -> bool {
      if (strcmp(node.name(), CONFIG_KEY_NAME) == 0 &&
          strcmp(node.attribute(CONFIG_ATTRIBUTE).value(), keystr.c_str()) ==
              0) {
        return true;
      }

      return false;
    });

    if (!node) {
      std::cerr << "Failed to find configuration with " << str << std::endl;
      std::cerr << " Failed to find <config name=\"" << keystr << "\">"
                << std::endl;

      return false;
    }

    // Set value
    node.text().set(value.first);

    return true;
  }

  return false;
}

int main(int argc, char *argv[]) {
  ArgumentParser argparse(argc, argv, 3);  // Two positional arguments
  bool ckptAndTerminate = false;
  bool restoreFromCkpt = false;
  bool redirect = false;
  const char *pathCheckpoint = nullptr;
  const char *pathOutputDirectory = nullptr;

  std::cout << "SimpleSSD Standalone v2.1" << std::endl;

  // Check argument
  {
    if (!argparse.isValid()) {
      std::cerr << " Failed to parse argument!" << std::endl;

      printHelp();

      return 1;
    }

    // Validate
    if (argparse.getArgument(OPT_HELP)) {
      printHelp();

      return 0;
    }
    if (argparse.getArgument(OPT_VERSION)) {
      printVersion();

      return 0;
    }

    if (argparse.getPositionalArgument(2) == nullptr) {
      std::cerr << " Unexpected number of positional argument." << std::endl;

      printHelp();

      return 1;
    }

    if (argparse.getArgument(OPT_CREATE_CKP) &&
        argparse.getArgument(OPT_RESTORE_CKP)) {
      std::cerr << " You cannot specify both create and restore checkpoint."
                << std::endl;

      return 1;
    }

    if ((pathCheckpoint = argparse.getArgument(OPT_CREATE_CKP))) {
      ckptAndTerminate = true;

      std::cout << " Checkpoint will be stored to " << pathCheckpoint
                << std::endl;
    }
    else if ((pathCheckpoint = argparse.getArgument(OPT_RESTORE_CKP))) {
      restoreFromCkpt = true;

      std::cout << " Try to restore from checkpoint at " << pathCheckpoint
                << std::endl;
    }

    if (argparse.getArgument(OPT_REDIRECT)) {
      redirect = true;
    }
  }

  // Install signal handler
  installSignalHandler(cleanup, progress);

  // Prepare config override
  {
    auto configlist = argparse.getMultipleArguments(OPT_CFG_OVERRIDE);
    auto sim_cb = [&configlist](pugi::xml_node &root) -> bool {
      for (auto &iter : configlist) {
        if (!overrideConfig(root, iter, true)) {
          return false;
        }
      }

      return true;
    };
    auto ssd_cb = [&configlist](pugi::xml_node &root) -> bool {
      for (auto &iter : configlist) {
        if (!overrideConfig(root, iter, false)) {
          return false;
        }
      }

      return true;
    };

    // Read simulation config file
    simConfig.load(argparse.getPositionalArgument(0), sim_cb);
    ssdConfig.load(argparse.getPositionalArgument(1), ssd_cb);
    pathOutputDirectory = argparse.getPositionalArgument(2);
  }

  // Log setting
  bool noLogPrintOnScreen = true;

  std::string logPath =
      simConfig.readString(Section::Simulation, Config::Key::StatFile);
  std::string debugLogPath =
      simConfig.readString(Section::Simulation, Config::Key::DebugFile);
  std::string latencyLogPath =
      simConfig.readString(Section::Simulation, Config::Key::Latencyfile);
  auto type = (Config::InterfaceType)simConfig.readUint(Section::Simulation,
                                                        Config::Key::Interface);

  ssdConfig.writeString(SimpleSSD::Section::Simulation,
                        SimpleSSD::Config::Key::OutputDirectory,
                        pathOutputDirectory);
  ssdConfig.writeString(SimpleSSD::Section::Simulation,
                        SimpleSSD::Config::Key::DebugFile, debugLogPath);
  ssdConfig.writeString(SimpleSSD::Section::Simulation,
                        SimpleSSD::Config::Key::OutputFile, debugLogPath);

  if (restoreFromCkpt) {
    ssdConfig.writeBoolean(SimpleSSD::Section::Simulation,
                           SimpleSSD::Config::Key::RestoreFromCheckpoint, true);
  }

  // Set interface type
  switch (type) {
    case Config::InterfaceType::NVMe:
      ssdConfig.writeUint(SimpleSSD::Section::Simulation,
                          SimpleSSD::Config::Key::Controller,
                          (uint64_t)SimpleSSD::Config::Mode::NVMe);
      break;
    default:
      ssdConfig.writeUint(SimpleSSD::Section::Simulation,
                          SimpleSSD::Config::Key::Controller,
                          (uint64_t)SimpleSSD::Config::Mode::None);
      break;
  }

  // Handle log printout
  {
    if (logPath.compare("STDOUT") == 0) {
      noLogPrintOnScreen = false;
      pLog = &std::cout;
    }
    else if (logPath.compare("STDERR") == 0) {
      noLogPrintOnScreen = false;
      pLog = &std::cerr;
    }
    else if (logPath.length() != 0) {
      std::filesystem::path full(pathOutputDirectory);

      logOut.open(full / logPath);

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
  }

  // Open latency log if specified
  if (latencyLogPath.length() != 0) {
    std::filesystem::path full(pathOutputDirectory);

    latencyFile.open(full / latencyLogPath);

    if (!latencyFile.is_open()) {
      std::cerr << " Failed to open log file: " << full << std::endl;

      return 3;
    }

    pLatencyFile = &latencyFile;
  }

  // Store config files to output directory
  {
    std::filesystem::path full(pathOutputDirectory);
    std::string path = full / "standalone.xml";

    simConfig.save(path);
  }
  {
    std::filesystem::path full(pathOutputDirectory);
    std::string path = full / "simplessd.xml";

    ssdConfig.save(path);
  }

  // Initialize SimpleSSD
  simplessd.init(&engine, &ssdConfig);

  // Handle checkpoint
  if (ckptAndTerminate) {
    simplessd.createCheckpoint(pathCheckpoint);

    std::cout << " Checkpoint stored to " << pathCheckpoint << std::endl;

    return 0;
  }
  else if (restoreFromCkpt) {
    simplessd.restoreCheckpoint(pathCheckpoint);

    std::cout << " Restored from checkpoint" << std::endl;
  }

  // Create Driver
  standaloneObject.engine = &engine;
  standaloneObject.config = &simConfig;
  standaloneObject.log = simplessd.getObject().log;

  switch (type) {
    case Config::InterfaceType::None:
      pInterface = new Standalone::Driver::None::NoneInterface(standaloneObject,
                                                               simplessd);

      break;
    case Config::InterfaceType::NVMe:
      pInterface = new Standalone::Driver::NVMe::NVMeInterface(standaloneObject,
                                                               simplessd);

      break;
    default:
      std::cerr << " Undefined interface specified." << std::endl;

      return 4;
  }

  // Create Block I/O Layer
  pBIOEntry = new IGL::BlockIOLayer(standaloneObject, pInterface, pLatencyFile);

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
        pIOGen->initialize();
        pIOGen->begin();
      },
      "beginCallback");

  // Insert stat event
  pInterface->getStatList(statList);

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

  // Redirect stdout/err here, if enabled
  if (redirect) {
    std::filesystem::path full(pathOutputDirectory);

    // TODO: validate this code on Windows
    int fd =
        open((full / "simout").c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd < 0) {
      perror("Failed to open redirect file");
      std::cerr << "Continue without redirection." << std::endl;
    }
    else {
      // Progress will be printed to screen, not file
      stdoutCopy = dup(fileno(stdout));

      dup2(fd, fileno(stdout));
      dup2(fd, fileno(stderr));

      close(fd);
    }
  }

  // Do Simulation
  std::cout << "********** Begin of simulation **********" << std::endl;

  pInterface->initialize(pBIOEntry, beginCallback);

  if (noLogPrintOnScreen || redirect) {
    int period = (int)simConfig.readUint(Section::Simulation,
                                         Config::Key::ProgressPeriod);

    if (period > 0) {
      pThread = new std::thread(threadFunc, period);
    }
  }

  // Simulation loop
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

  if (pThread) {
    pThread->join();

    delete pThread;
  }

  killLock.unlock();

  // Now we only have main thread
  // Unlock mTick mutex - Only main thread can run SIGINT handler (cleanup)
  engine.forceUnlock();

  tick = engine.getTick();

  if (tick == 0) {
    // Exit program
    exit(0);
  }

  // Print last statistics
  statistics(tick, true);

  // Erase progress
  printf("\33[2K                                                           \r");

  pIOGen->printStats(std::cout);
  engine.printStats(std::cout);

  // Cleanup all here
  delete pInterface;
  delete pIOGen;

  delete pBIOEntry;  // Used by progress thread

  if (logOut.is_open()) {
    logOut.close();
  }
  if (debugLogOut.is_open()) {
    debugLogOut.close();
  }

  std::cout << "End of simulation @ tick " << tick << std::endl;

  simplessd.deinit();

  // Close copied fd
  if (stdoutCopy != 1) {
    dprintf(stdoutCopy, "\nEnd of simulation @ tick %" PRIu64 "\n", tick);

    close(stdoutCopy);
  }

  // Exit program
  exit(0);
}

void progress(int) {
  uint64_t tick;
  IGL::Progress data;
  float progress;

  tick = engine.getTick();
  pIOGen->getProgress(progress);
  pBIOEntry->getProgress(data);

  dprintf(stdoutCopy,
          "\nSimTick: %" PRIu64 ", Progress: %.2f%%, IOPS: %" PRIu64 ", BW: ",
          tick, progress * 100.f, data.iops);
  printBandwidth(stdoutCopy, data.bandwidth);
  dprintf(stdoutCopy, ", Lat: ");
  printLatency(stdoutCopy, data.latency);
  dprintf(stdoutCopy, "\n");
}

void statistics(uint64_t tick, bool last) {
  if (pLog == nullptr && !last) {
    return;
  }

  if (UNLIKELY(last && pLog == nullptr)) {
    pLog = &std::cout;
  }

  std::ostream &out = *pLog;
  std::vector<double> stat;
  uint64_t count = 0;

  pInterface->getStatValues(stat);

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
  uint64_t current = 0;
  uint64_t old = 0;
  float progress = 0.f;
  auto duration = std::chrono::seconds(tick);
  IGL::Progress data;

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

    dprintf(stdoutCopy,
            "\33[2K*** Progress: %.2f%% (%lf ops) IOPS: %" PRIu64 " BW: ",
            progress * 100.f, (double)(current - old) / tick, data.iops);
    printBandwidth(stdoutCopy, data.bandwidth);
    dprintf(stdoutCopy, " Avg. Lat: ");
    printLatency(stdoutCopy, data.latency);
    dprintf(stdoutCopy, "\r");

    old = current;
  }
}
