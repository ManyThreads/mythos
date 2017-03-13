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
#include <sys/types.h>
#include <sys/stat.h>
#include <memory>
#include <signal.h>

using namespace mythos;
class mic_ctrl;

// Environmental variables set if program is called by sudo
static const constexpr char* ENV_UID = "SUDO_UID";
static const constexpr char* ENV_GID = "SUDO_GID";

struct sigaction sigact;
mic_ctrl *ctrl;

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

void drop_root() {
  if (getuid() != 0) {
    std::cerr << "Cannot drop root when not root.\n";
    return;
  }
  char *gid = std::getenv(ENV_GID);
  char *uid = std::getenv(ENV_UID);
  if (gid == nullptr || uid == nullptr) {
    std::cerr << "No environment variables. Application has to be run with sudo.\n";
    return;
  }
  if (setregid(-1, atoi(std::getenv(ENV_GID))) != 0) {
    std::cerr << "Could not restore Group ID.\n";
  }
  if (setreuid(-1, atoi(std::getenv(ENV_UID))) != 0) {
    std::cerr << "Could not restore User ID.\n";
  }
  if (geteuid() == 0) {
    std::cerr << "Dropping root unsuccessful\n";
  }
}

void restore_root() {
  if (seteuid(0) != 0) {
    std::cerr << "Could not restore root \n";
  }

  if (setegid(0) != 0) {
    std::cerr << "Could not restore root\n";
  }
}

struct InfoPtr {
  uint64_t info;
  uint64_t debug;
};

/// Class logs into files, opening new file for every sending channel id.
class file_logger {
  public:

    file_logger(const char *directory_)
      :directory(directory_) {
        std::stringstream s;
        s << getenv("PWD") << "/" << directory;
        if (mkdir(s.str().c_str(),
              S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
        {
          std::cerr << "Error creating directory for logs.\n";
        }
      }

    void log(uint16_t vchannel, const std::string &msg) {
      std::cout << vchannel << ": " << msg << "\n";

      auto &filestream = files[vchannel];
      if (!filestream) {
        std::stringstream s;
        s << directory << "/" << vchannel;
        filestream = std::unique_ptr<std::ofstream>(
            new std::ofstream(s.str(), std::ios::out | std::ios::trunc)
            );
        if (!filestream->is_open()) {
          std::cerr << "Could not open file " << (std::to_string(vchannel)) << "\n";
          return;
        }
      }
      *filestream << msg << "\n";
      filestream->flush();
    }

  private:
    const std::string directory;
    std::unordered_map<uint16_t, std::unique_ptr<std::ofstream>> files;
};

/// Wrapper to the micctrl application, that controls the xeon phi coprocessor
class mic_ctrl {
public:
    mic_ctrl(int adapter_)
        :adapter(adapter_) {
      }


    void boot() {
      std::stringstream ss;
      ss << "sudo sh -c \"echo \\\"boot:elf:`pwd`/boot64.elf\\\" > /sys/class/mic/mic";
      ss << adapter;
      ss << "/state\"";
      system(ss.str().c_str());
    }

    void reset_wait () {
      std::stringstream ss;
      ss << "sudo micctrl -r --wait mic";
      ss << adapter;
      ss << " mic";
      ss << adapter;
      system(ss.str().c_str());
    }

    void reset() {
      std::stringstream ss;
      ss << "sudo micctrl -r mic";
      ss << adapter;
      system(ss.str().c_str());
    }

    void status() {
      std::stringstream ss;
      ss << "sudo micctrl -s mic";
      ss << adapter;
      system(ss.str().c_str());
    }

private:
    int adapter;
};

void signal_handler(int sig_type) {
  if (sig_type == SIGINT) {
    restore_root();
    ctrl->reset();
    ctrl->status();
  }
  exit(0);
}


int main(int argc, char** argv)
{
  int adapter;
  if (argc < 2) {
    std::cerr << argv[0] << " adapter" << std::endl;
    return -1;
  }

  sigact.sa_handler = signal_handler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, nullptr);

  parsenum(argv[1], adapter);
  std::cerr << "will read from adapter " << adapter
    << " file " << deviceKNC(adapter) << std::endl;
  ctrl = new mic_ctrl(adapter);
  ctrl->reset_wait();
  ctrl->status();
  ctrl->boot();
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


  // open micctrl file before dropping root

  // drop root to allow file logger access to user specific directories
  drop_root();

  // collect messages until whole message arrived
  std::unordered_map<uint16_t, std::unique_ptr<std::stringstream> > messages;

  // log to directory debug
  file_logger logger("debug");

  while (true) {
    //typedef HostInfoTable::DebugChannel::handle_t handle_t;
    auto handle = debugOut.acquireRecv();
    auto& msg = debugOut.get<DebugMsg>(handle);

    auto &stream = messages[msg.vchannel];
    if (!stream) {
      stream = std::unique_ptr<std::stringstream>(new std::stringstream());
    }

    if (msg.msgbytes <= DebugMsg::PAYLOAD) {
      stream->write(msg.data, msg.msgbytes);
      auto str = stream->str();
      logger.log(msg.vchannel, str);
      stream->str(std::string());
      stream->clear();
    } else {
      stream->write(msg.data, DebugMsg::PAYLOAD);
    }
    debugOut.finishRecv(handle);
  }

  return 0;
}
