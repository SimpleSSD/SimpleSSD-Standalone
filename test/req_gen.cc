#include <cinttypes>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>

int main(int argc, char *argv[]) {
  std::ifstream input;

  if (argc == 2) {
    input.open(argv[1]);
  }

  if (!input.is_open()) {
    std::cerr << "Failed to open file." << std::endl;

    return 1;
  }

  std::regex regsub(
      "(\\d+): HIL::NVMe::Command: CTRL 0   \\| SQ  1:(\\d+) +\\|.*h");
  std::regex regcmp("(\\d+): HIL::NVMe::Command: CTRL 0   \\| SQ  1:(\\d+) "
                    "+\\|.*\\((\\d+)\\)");
  std::unordered_map<uint16_t, uint64_t> queue;
  std::string line;
  std::smatch match;

  uint16_t head;
  uint64_t tick;

  while (!input.eof()) {
    std::getline(input, line);

    if (std::regex_match(line, match, regsub)) {
      head = (uint16_t)strtoul(match[2].str().c_str(), nullptr, 10);
      tick = (uint64_t)strtoull(match[1].str().c_str(), nullptr, 10);

      queue.emplace(std::make_pair(head, tick));

      continue;
    }

    if (std::regex_match(line, match, regcmp)) {
      head = (uint16_t)strtoul(match[2].str().c_str(), nullptr, 10);
      tick = (uint64_t)strtoull(match[3].str().c_str(), nullptr, 10);

      auto iter = queue.find(head);

      if (iter != queue.end()) {
        printf("%" PRIu64 ", %" PRIu64 "\n", iter->second, tick);

        queue.erase(iter);
      }
      else {
        std::cerr << "ERROR" << std::endl;
      }
    }
  }

  return 0;
}
