/*

Copyright (c) 2015 Christoph Husse

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <stddef.h>
#include "ProgramOptions.h"
#include "ILogger.h"
#include <iostream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"

#include <boost/program_options.hpp>

#pragma clang diagnostic pop

namespace po = boost::program_options;

namespace DirectRemote {

bool ProgramOptions::parse(int argc, const char *const *argv,
                           bool printHelpOnFailure) {
  std::vector<std::string> cmdArgs;
  for (int i = 0; i < argc; i++) {
    cmdArgs.push_back(argv[i]);
  }

  return parse(cmdArgs, printHelpOnFailure);
}

bool ProgramOptions::parse(std::vector<std::string> cmdArgs,
                           bool printHelpOnFailure) {
  po::options_description desc("The following parameters are supported:");
  desc.add_options()("help", "Show a list of supported parameters.")(
      "protocol",
      po::value<std::string>()->default_value("UdpProxy://127.0.0.1:41988"),
      "A protocol string. This is completely protocol dependent. The "
      "requirement is for the parameter to start with '[a-z0-9_]+\\://'. The "
      "alphanumeric characters shall refer to a protocol plugin name, %name%. "
      "The lookup will then search for a module named 'NetworkProtocol_%name%' "
      "in the application's directory and pass the remainder of the parameter "
      "to it.")(

      "disable-input", "If specified, the host will ignore remote input.")(

      "show-console",
      "If specified, the application will open a console to print log output.")(

      "sessionId", po::value<uint64_t>()->default_value(0),
      "The session Id used to pair with a specific peer.")(

      "peerTimeout", po::value<int32_t>()->default_value(30),
      "The timeout in seconds after which waiting for a remote peer is "
      "terminated and the app closed with an error code.")(

      "keyFrameDistance", po::value<int32_t>()->default_value(3),
      "The number of non-keyframes to insert between two keyframes. The "
      "higher, the less bandwidth with same quality, but also much less error "
      "tolerant.")(

      "targetBitrateKbps", po::value<int32_t>()->default_value(10000),
      "The desired bitrate for video encoding in kilo bits per second.");

  po::variables_map vm;
  std::vector<const char *> cmdStrings;

  for (auto &s : cmdArgs) {
    cmdStrings.push_back(s.c_str());
  }

  try {
    po::store(po::parse_command_line(static_cast<int>(cmdArgs.size()),
                                     cmdStrings.data(), desc),
              vm);
    po::notify(vm);
  } catch (...) {
    if (printHelpOnFailure) {
      std::cout << desc << std::endl;
    }
    return false;
  }

  if (vm.count("help")) {
    if (printHelpOnFailure) {
      std::cout << desc << std::endl;
    }
    return false;
  }

  if (vm.count("protocol")) {
    auto str = vm["protocol"].as<std::string>();
    size_t pos = str.find("://");
    if (pos == std::string::npos) {
      m_protocolPlugin = str;
    } else {
      m_protocolPlugin = str.substr(0, pos);
      m_protocolString = str.substr(pos + 3);
    }
  }

  m_enableInput = vm.count("disable-input") == 0;
  m_showConsole = vm.count("show-console") > 0;

  if (vm.count("sessionId")) {
    m_sessionId = vm["sessionId"].as<uint64_t>();
  }

  if (vm.count("peerTimeout")) {
    m_peerTimeout = std::max(0, vm["peerTimeout"].as<int32_t>());
  }

  if (vm.count("keyFrameDistance")) {
    m_keyFrameDistance = std::max(1, vm["keyFrameDistance"].as<int32_t>());
  }

  if (vm.count("targetBitrateKbps")) {
    m_targetBitrateKbps =
        std::max(100, std::min(50000, vm["targetBitrateKbps"].as<int32_t>()));
  }

  return true;
}
}  // namespace DirectRemote
