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

#include <iostream>
#include <fstream>

#include "bil/entry.hh"
#include "drivers/none/none.hh"
#include "sim/engine.hh"
#include "sim/signal.hh"
#include "simplessd/util/simplessd.hh"

// Global objects
BIL::DriverInterface *pInterface = nullptr;

void cleanup(int) {
  // Cleanup all here
  delete pInterface;
}

int main(int argc, char *argv[]) {
  Engine engine;
  ConfigReader simConfig;

  std::cout << "SimpleSSD Standalone v2.1" << std::endl;

  // Check argument
  if (argc != 3) {
    std::cerr << " Invalid number of argument!" << std::endl;
    std::cerr << "  Usage: simplessd-standalone <Simulation configuration "
                 "file> <SimpleSSD configuration file>"
              << std::endl;

    return 1;
  }

  // Install signal handler
  installSignalHandler(cleanup);

  // Read simulation config file
  if (!simConfig.init(argv[1])) {
    std::cerr << " Failed to open simulation configuration file!" << std::endl;

    return 2;
  }

  // Log setting
  std::ostream *pLog;
  std::ostream *pDebugLog;
  std::ofstream logOut;
  std::ofstream debugLogOut;
  std::string logPath = simConfig.readString(CONFIG_GLOBAL, GLOBAL_LOG_FILE);
  std::string debugLogPath =
      simConfig.readString(CONFIG_GLOBAL, GLOBAL_DEBUG_LOG_FILE);

  if (logPath.compare("STDOUT") == 0) {
    pLog = &std::cout;
  }
  else if (logPath.compare("STDERR") == 0) {
    pLog = &std::cerr;
  }
  else {
    logOut.open(logPath);

    if (~logOut.is_open()) {
      std::cerr << " Failed to open log file: " << logPath << std::endl;

      return 3;
    }

    pLog = &logOut;
  }

  if (debugLogPath.compare("STDOUT") == 0) {
    pDebugLog = &std::cout;
  }
  else if (debugLogPath.compare("STDERR") == 0) {
    pDebugLog = &std::cerr;
  }
  else {
    debugLogOut.open(debugLogPath);

    if (~debugLogOut.is_open()) {
      std::cerr << " Failed to open log file: " << debugLogPath << std::endl;

      return 3;
    }

    pDebugLog = &debugLogOut;
  }

  // Initialize SimpleSSD
  auto ssdConfig =
      initSimpleSSDEngine(&engine, *pDebugLog, *pDebugLog, argv[2]);

  // Create Driver
  switch (simConfig.readUint(CONFIG_GLOBAL, GLOBAL_INTERFACE)) {
    case INTERFACE_NONE:
      pInterface = new SIL::NoneDriver(engine, ssdConfig);

      break;
    default:
      std::cerr << " Undefined interface specified." << std::endl;

      return 4;
  }

  // Create Block I/O Layer
  BIL::BlockIOEntry bioEntry(simConfig, engine, pInterface);

  // Create I/O generator
  // TODO: fill here

  // Do Simulation
  while (engine.doNextEvent());

  std::cout << "End of simulation @ tick " << engine.getCurrentTick()
            << std::endl;

  cleanup(0);

  return 0;
}
