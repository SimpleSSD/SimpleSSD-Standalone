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

#include "bil/entry.hh"
#include "sim/engine.hh"
#include "sim/signal.hh"
#include "simplessd/util/simplessd.hh"

void cleanup(int) {
  // Cleanup all here
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

  // Initialize SimpleSSD
  auto ssdConfig = initSimpleSSDEngine(&engine, std::cout, std::cerr, argv[2]);

  // Read simulation config file
  if (!simConfig.init(argv[1])) {
    std::cerr << " Failed to open simulation configuration file!" << std::endl;

    return 2;
  }

  // Create Block I/O Layer
  BIL::BlockIOEntry bioEntry(simConfig, engine);

  // Create I/O generator
  // TODO: fill here

  // Do Simulation
  while (engine.doNextEvent()) {
    // Do nothing
  }

  std::cout << "End of simulation @ tick " << engine.getCurrentTick()
            << std::endl;

  cleanup(0);

  return 0;
}
