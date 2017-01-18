/* -*- mode:C++; -*- */
/* MyThOS: The Many-Threads Operating System
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Copyright 2016 Randolf Rotta, Maik Krüger, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#include "host/MemMapperPci.hh"
#include "mythos/HostInfoTable.hh"
#include "mythos/PciMsgQueueMPSC.hh"
#include "cpu/clflush.hh"
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <unordered_map>
#include <fstream>

using namespace mythos;


std::string DescribeIosFailure(const std::ios& stream)
{
  std::string result;

  if (stream.eof()) {
    result = "Unexpected end of file.";
  }  else if (errno) {
#if defined(__unix__)
    // We use strerror_r because it's threadsafe.
    // GNU's strerror_r returns a string and may ignore buffer completely.
    char buffer[255];
    result = std::string(strerror_r(errno, buffer, sizeof(buffer)));
#else
    result = std::string(strerror(errno));
#endif
  }

  else {
    result = "Unknown file error.";
  }
  return result;
}

template<typename T>
void parsenum(char const *str, T& val)
{
  std::stringstream s;
  if (str[0] == '0' && str[1] == 'x')
    s << std::hex << str;
  else
    s << str;
  s >> val;
}

std::string deviceKNC(int adapter)
{
  std::stringstream s;
  // choose the aperture memory file with the write combining enabled.
  s << "/sys/class/mic/mic" << adapter << "/device/resource0"; /// @todo or _wc ???
  return s.str();
}

struct InfoPtr {
  uint64_t info;
  uint64_t debug;
};

/// Class logs into files, opening new file for every sending channel id.
class file_logger {
public:
  ~file_logger() {
    for (auto f : files) {
      f.second->close();
      delete f.second;
    }
  }

  void log(uint16_t vchannel, std::string msg) {
    auto found = files.find(vchannel);
    if (found != files.end()) {
      *found->second << msg << "\n";
      found->second->flush();
    } else {
      std::ofstream *file_stream = new std::ofstream("..\\" + std::to_string(vchannel),
                                           std::ios::out | std::ios::trunc);
      if (!file_stream->is_open()) {
        std::cerr << "Could not open file " << ("..\\" + std::to_string(vchannel)) << "\n";
        std::cerr << DescribeIosFailure(*file_stream) << "\n";
        return;
      }
      files.insert({vchannel, file_stream});
      *file_stream << msg << "\n";
    }
  }
private:
  std::unordered_map<uint16_t, std::ofstream*> files;
};

int main(int argc, char** argv)
{
  int adapter;
  if (argc < 2) {
    std::cerr << argv[0] << " adapter" << std::endl;
    return -1;
  }
  parsenum(argv[1], adapter);
  std::cerr << "will read from adapter " << adapter
	   << " file " << deviceKNC(adapter) << std::endl;
  MemMapperPci micmem(deviceKNC(adapter));

  // wait until the _host_info_ptr pointer at address 2MiB actually contains something
  auto info_ptr = micmem.access(PhysPtr<InfoPtr>(0x200000ull));
  do {
    cpu::clflush(info_ptr.addr());
    std::cout << "info: " << (void*)info_ptr->info << " debug: " << (void*)info_ptr->debug << " " << info_ptr->debug << std::endl;
    if (info_ptr->info == 0) sleep(1);
  } while (info_ptr->info == 0);
  std::cerr << "host info is at physical address "
	    << (void*)info_ptr->info << std::endl;

  auto info = micmem.access(PhysPtr<HostInfoTable>(info_ptr->info));
  cpu::clflush(info.addr());
  std::cerr << "debug stream is at physical address "
	    << (void*)info->debugOut << std::endl;

  auto debugOutChannel = micmem.access(PhysPtr<HostInfoTable::DebugChannel>(info->debugOut));
  PCIeRingConsumer<HostInfoTable::DebugChannel> debugOut(debugOutChannel.addr());

  // collect messages until whole message arrived
  std::unordered_map<uint16_t, std::string> messages;

  // log to file
  file_logger logger;

  while (true) {
    //typedef HostInfoTable::DebugChannel::handle_t handle_t;
    /// @todo log messages into a file
    auto handle = debugOut.acquireRecv();
    auto& msg = debugOut.get<DebugMsg>(handle);

    auto found = messages.find(msg.vchannel);
    if (found != messages.end()) { // have to merge messages
      if (msg.msgbytes < DebugMsg::PAYLOAD) { //output merged messages
          std::string out(found->second + std::string(msg.data, msg.msgbytes));
          std::cout << found->first  << "(" << out.length() << "): ";
          std::cout.write(out.c_str(), out.length());
          std::cout << std::endl;
          logger.log(found->first, out);
          messages.erase(msg.vchannel);
      } else { // append another message
          std::string append(found->second + std::string(msg.data, msg.msgbytes));
          messages.insert({msg.vchannel, append});
      }
    } else {
        if (msg.msgbytes < DebugMsg::PAYLOAD) {
            std::cout << msg.vchannel << "(" << msg.msgbytes << "): ";
            std::cout.write(msg.data, msg.msgbytes);
            std::cout << std::endl;
            logger.log(msg.vchannel, {msg.data});
        } else {
            messages.insert({msg.vchannel, std::string(msg.data)});
        }
    }
    debugOut.finishRecv(handle);
  }

  return 0;
}
