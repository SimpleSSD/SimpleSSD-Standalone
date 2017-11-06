/**
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
 *
 * Authors: Donghyun Gouk <kukdh1@camelab.org>
 */

#include "config.hh"

Config::Config(std::string path) : BaseConfig(path) {
  // Parse configuration file
  if (ini_parse(path.c_str(), parserHandler, this) < 0) {
    fatal("config: Cannot open configuration file: %s\n", path.c_str());
  }
}

int Config::parserHandler(void *context, const char* section, const char* name, const char *value) {
  Config *pThis = (Config *)context;

  if (MATCH_SECTION("rgen")) {
    if (MATCH_NAME("MaxRequest")) {
      pThis->MaxRequest = toInt(value);
    }
    else if (MATCH_NAME("StartPPN")) {
      pThis->StartPPN = toInt(value);
    }
    else if (MATCH_NAME("RequestSize")) {
      pThis->RequestSize = toInt(value);
    }
    else if (MATCH_NAME("ReadFraction")) {
      pThis->ReadFraction = toInt(value);
    }
    else if (MATCH_NAME("ReadDenominator")) {
      pThis->ReadDenominator = toInt(value);
    }
    else if (MATCH_NAME("RandomFraction")) {
      pThis->RandomFraction = toInt(value);
    }
    else if (MATCH_NAME("RandomDenominator")) {
      pThis->RandomDenominator = toInt(value);
    }
    else if (MATCH_NAME("RandomSeed")) {
      pThis->RandomSeed = toInt(value);
    }
    else if (MATCH_NAME("IOGEN")) {
      pThis->IOGEN = toInt(value);
    }
    else if (MATCH_NAME("QueueDepth")) {
      pThis->QueueDepth = toInt(value);
    }
  }
  else if (MATCH_SECTION("trace")) {
    if (MATCH_NAME("Enable")) {
      pThis->Enable = toInt(value);
    }
    else if (MATCH_NAME("TraceFile")) {
      pThis->TraceFile = value;
    }
  }

  return 1;
}
