#include <iostream>
#include <string>

#include "main/config_reader.hh"

using namespace Standalone;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << " Usage: " << argv[0] << " <output xml file> <input xml file>"
              << std::endl;
  }

  {
    ConfigReader reader;

    // Change all values to <int: 55> <float: 55.55> <bool: true> <string: test>
    {
      const uint64_t utest = 55;
      const float ftest = 55.55f;
      const bool btest = true;
      const std::string stest("test");

      // Section::Simulation
      reader.writeUint(Section::Simulation, Config::Key::Mode, utest);
      reader.writeUint(Section::Simulation, Config::Key::StatPeriod, utest);
      reader.writeString(Section::Simulation, Config::Key::OutputDirectory,
                         stest);
      reader.writeString(Section::Simulation, Config::Key::StatFile, stest);
      reader.writeString(Section::Simulation, Config::Key::DebugFile, stest);
      reader.writeString(Section::Simulation, Config::Key::Latencyfile, stest);
      reader.writeUint(Section::Simulation, Config::Key::ProgressPeriod, utest);
      reader.writeUint(Section::Simulation, Config::Key::Interface, utest);
      reader.writeUint(Section::Simulation, Config::Key::Scheduler, utest);
      reader.writeUint(Section::Simulation, Config::Key::SubmissionLatency,
                       utest);
      reader.writeUint(Section::Simulation, Config::Key::CompletionLatency,
                       utest);

      // Section::RequestGenerator
      reader.writeUint(Section::RequestGenerator, IGL::RequestConfig::Key::Size,
                       utest);
      reader.writeUint(Section::RequestGenerator, IGL::RequestConfig::Key::Type,
                       utest);
      reader.writeFloat(Section::RequestGenerator,
                        IGL::RequestConfig::Key::ReadMix, ftest);
      reader.writeUint(Section::RequestGenerator,
                       IGL::RequestConfig::Key::BlockSize, utest);
      reader.writeUint(Section::RequestGenerator,
                       IGL::RequestConfig::Key::BlockAlign, utest);
      reader.writeUint(Section::RequestGenerator, IGL::RequestConfig::Key::Mode,
                       utest);
      reader.writeUint(Section::RequestGenerator,
                       IGL::RequestConfig::Key::Depth, utest);
      reader.writeUint(Section::RequestGenerator,
                       IGL::RequestConfig::Key::Offset, utest);
      reader.writeUint(Section::RequestGenerator,
                       IGL::RequestConfig::Key::Limit, utest);
      reader.writeUint(Section::RequestGenerator,
                       IGL::RequestConfig::Key::ThinkTime, utest);
      reader.writeUint(Section::RequestGenerator,
                       IGL::RequestConfig::Key::RandomSeed, utest);
      reader.writeBoolean(Section::RequestGenerator,
                          IGL::RequestConfig::Key::TimeBased, btest);
      reader.writeUint(Section::RequestGenerator,
                       IGL::RequestConfig::Key::RunTime, utest);

      // Section::TraceReplayer
      reader.writeString(Section::TraceReplayer, IGL::TraceConfig::Key::File,
                         stest);
      reader.writeUint(Section::TraceReplayer,
                       IGL::TraceConfig::Key::TimingMode, utest);
      reader.writeUint(Section::TraceReplayer, IGL::TraceConfig::Key::Depth,
                       utest);
      reader.writeUint(Section::TraceReplayer, IGL::TraceConfig::Key::Limit,
                       utest);
      reader.writeString(Section::TraceReplayer, IGL::TraceConfig::Key::Regex,
                         stest);
      reader.writeUint(Section::TraceReplayer,
                       IGL::TraceConfig::Key::GroupOperation, utest);
      reader.writeUint(Section::TraceReplayer,
                       IGL::TraceConfig::Key::GroupByteOffset, utest);
      reader.writeUint(Section::TraceReplayer,
                       IGL::TraceConfig::Key::GroupByteLength, utest);
      reader.writeUint(Section::TraceReplayer,
                       IGL::TraceConfig::Key::GroupLBAOffset, utest);
      reader.writeUint(Section::TraceReplayer,
                       IGL::TraceConfig::Key::GroupLBALength, utest);
      reader.writeUint(Section::TraceReplayer,
                       IGL::TraceConfig::Key::GroupSecond, utest);
      reader.writeUint(Section::TraceReplayer,
                       IGL::TraceConfig::Key::GroupMiliSecond, utest);
      reader.writeUint(Section::TraceReplayer,
                       IGL::TraceConfig::Key::GroupMicroSecond, utest);
      reader.writeUint(Section::TraceReplayer,
                       IGL::TraceConfig::Key::GroupNanoSecond, utest);
      reader.writeUint(Section::TraceReplayer,
                       IGL::TraceConfig::Key::GroupPicoSecond, utest);
      reader.writeUint(Section::TraceReplayer, IGL::TraceConfig::Key::LBASize,
                       utest);
      reader.writeBoolean(Section::TraceReplayer,
                          IGL::TraceConfig::Key::UseHexadecimal, btest);
    }

    // Write test
    reader.save(argv[1]);
  }

  {
    ConfigReader reader;

    reader.load(argv[1]);
    reader.save(argv[2]);
  }

  return 0;
}
