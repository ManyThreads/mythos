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

#include "host/InitChannel.hh"
#include "util/FDSender.hh"
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
#include <fcntl.h>
#include <thread>

using namespace mythos;
class mic_ctrl;
class file_logger;

// Environmental variables set if program is called by sudo
static const constexpr char* ENV_UID = "SUDO_UID";
static const constexpr char* ENV_GID = "SUDO_GID";

struct sigaction sigact;
mic_ctrl *ctrl;
file_logger *logger;

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

    ~file_logger() {
      for (auto &c : files) {
        c.second.reset();
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
    }

  private:
    const std::string directory;
    std::unordered_map<uint16_t, std::unique_ptr<std::ofstream>> files;
};

void shareDeviceMemoryFD(int fd){
  FDSender sender;

  if(sender.awaitConnection("../mythos_device_memory.ds")){
    printf("Domain socket connected\n");
  }else{
	printf("Domain socket connection failed!");
  }

  sender.sendFD(fd);
}

/// Wrapper to the micctrl application, that controls the xeon phi coprocessor
class mic_ctrl {
public:
    mic_ctrl(int adapter_, char *kernel_image_path_)
        :adapter(adapter_), kernel_image_path(kernel_image_path_) {
          std::stringstream ss;
          ss << "/sys/class/mic/mic" << adapter << "/state";
          fd = open("/sys/class/mic/mic0/state", O_RDWR);
          if (fd < 0) {
            std::cerr << "Could not open state file\n";
          }
    }

    ~mic_ctrl() {
      close(fd);
    }


    void boot() {
      std::stringstream ss;
      ss << "boot:linux:" << kernel_image_path;
      if (write_state(ss.str().c_str()) < 0) {
        std::cout << "Cannot write to state file for booting.\n";
        exit(-1);
      }
    }

    void reset_wait () {
      std::cout << "Reset and wait for getting ready.\n";
      char buf[255];
      read_state(buf, sizeof(buf));
      if (strcmp(buf, "ready") == 0) {
        return;
      }
      write_state("reset");
      time_t start = time(nullptr);
      do {

        if (time(nullptr) > start + 5 || read_state(buf, sizeof(buf)) < 0) {
          break;
        }
      } while(strcmp(buf, "ready") != 0 && strcmp(buf, "boot") != 0);
    }

    void reset() {
      write_state("reset");
    }

    void status() {
      char buf[255];
      read_state(buf, 255);
      printf("%s\n", &buf[0]);
    }

private:

    ssize_t write_state(const char *cmd) {
      ssize_t ret = -1;
      if ((ret = pwrite(fd, cmd, strlen(cmd), 0)) < 0) {
        perror("Could not write state file:");
      }
      fsync(fd);
      return ret;
    }

    ssize_t read_state(char *buf, int len) {
      ssize_t ret = -1;
      if ((ret = pread(fd, buf, len, 0)) < 0) {
        perror("Could not read state file:");
      }
      buf[ret] = 0;
      return ret;
    }

    int adapter;
    char *kernel_image_path;
    int fd;
};

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

void signal_handler(int sig_type) {
  if (sig_type == SIGINT) {
    ctrl->reset_wait();
  }
  delete ctrl;
  delete logger;
  exit(0);
}


int main(int argc, char** argv)
{
  int adapter;
  if (argc < 3) {
    std::cerr << argv[0] << " adapter path/to/kernel/image.elf" << std::endl;
    return -1;
  }
  sigact.sa_handler = signal_handler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, nullptr);

  parsenum(argv[1], adapter);
  std::cerr << "will read from adapter " << adapter
    << " file " << deviceKNC(adapter) << std::endl;
  ctrl = new mic_ctrl(adapter, argv[2]);
  ctrl->reset_wait();
  ctrl->boot();
  auto deviceFile = deviceKNC(adapter);
  InitChannel micmem(deviceFile.c_str());

  // wait until the host_info_ptr structure contains something
  do {
    auto info = micmem.getHITptr();
    std::cerr << "info: " << (void*)info->info
             << " debug: " << (void*)info->debug << " " << info->debug << std::endl;
    if (info->info == 0) sleep(1);
    else std::cerr << "host info is at physical address " << (void*)info->info << std::endl;
  } while (!micmem.getHIT());
  std::cerr << "debug stream is at physical address "
           << (void*)micmem.getHIT()->debugOut << std::endl;
  PCIeRingConsumer<HostInfoTable::DebugChannel> debugOut(micmem.getDebugOut());

  // drop root to allow file logger access to user specific directories
  drop_root();

  std::thread fdThread(shareDeviceMemoryFD, micmem.getFD());

  // auto data_ptr = (char*)micmem.map(0x0, 8ull*1024*1024*1024);
  // std::ofstream data_file("memdump.bin", std::ios::out | std::ios::trunc);
  // data_file.write(data_ptr, 8ull*1024*1024*1024);
  // data_file.close();

  // collect messages until whole message arrived
  std::unordered_map<uint16_t, std::unique_ptr<std::stringstream> > messages;

  // log to directory debug
  logger = new file_logger("debug");

  while (true) {
    auto handle = debugOut.acquireRecv();
    auto& msg = debugOut.get<DebugMsg>(handle);

    auto &stream = messages[msg.vchannel];
    if (!stream) {
      stream = std::unique_ptr<std::stringstream>(new std::stringstream());
    }

    if (msg.msgbytes <= DebugMsg::PAYLOAD) {
      stream->write(msg.data, msg.msgbytes);
      auto str = stream->str();
      logger->log(msg.vchannel, str);
      stream->str(std::string());
      stream->clear();
    } else {
      stream->write(msg.data, DebugMsg::PAYLOAD);
    }
    debugOut.finishRecv(handle);
  }

  return 0;
}
