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

using namespace mythos;

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

  while (true) {
    //typedef HostInfoTable::DebugChannel::handle_t handle_t;
    /// @todo merge multi-package messages per vchannel before printing
    /// @todo log messages into a file
    auto handle = debugOut.acquireRecv();
    auto& msg = debugOut.get<DebugMsg>(handle);
    std::cout << msg.vchannel << "(" << msg.msgbytes << "): ";
    std::cout.write(msg.data, msg.msgbytes>DebugMsg::PAYLOAD?DebugMsg::PAYLOAD:msg.msgbytes);
    std::cout << std::endl;
    debugOut.finishRecv(handle);
  }
  
  return 0;
}
