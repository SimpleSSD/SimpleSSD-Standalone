#include <cinttypes>
#include <iostream>
#include <vector>

#include "main/config_reader.hh"

using namespace Standalone;

int main(int argc, char *argv[]) {
  std::string prefix("./test_");
  std::string input;

  switch (argc) {
    case 3:
      prefix = argv[2];
      /* fallthrough */
    case 2:
      input = argv[1];
      break;
    default:
      std::cerr << "Usage: " << argv[0]
                << "<template config file> [Output prefix]" << std::endl;
  }

  ConfigReader reader;

  reader.load(input);

  reader.writeUint(Section::Simulation, Config::Key::Mode,
                   (uint64_t)Config::ModeType::RequestGenerator);
  reader.writeUint(Section::RequestGenerator, IGL::RequestConfig::Key::Mode,
                   (uint64_t)IGL::RequestConfig::IOMode::Asynchronous);

  for (uint32_t qd = 1; qd <= 64; qd <<= 1) {
    reader.writeUint(Section::RequestGenerator, IGL::RequestConfig::Key::Depth,
                     qd);

    for (uint32_t bs = 4096; bs <= 1048576; bs <<= 1) {
      std::string filename(prefix);

      reader.writeUint(Section::RequestGenerator,
                       IGL::RequestConfig::Key::BlockSize, bs);

      filename += std::to_string(bs / 1024);
      filename += "K";
      filename += std::to_string(qd);
      filename += ".xml";

      reader.save(filename);
    }
  }

  return 0;
}
