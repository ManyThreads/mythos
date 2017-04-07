#pragma once

#include <sys/socket.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>

class FDReceiver{
	int sock;
	struct sockaddr_un addr;


public:
	FDReceiver()
		: sock(-1)
	{
	}

	~FDReceiver(){
		disconnect();
	}

	bool disconnect(){
		if(sock >= 0)
			close(sock);
	}

	bool connectToSocket(const char *sockPath){
		disconnect();

		sock = socket(PF_LOCAL, SOCK_STREAM, 0);	
		if (sock == -1){
			std::cerr << "error: unable to open socket" << std::endl;
			return false; 
		}

		addr.sun_family = AF_LOCAL;
		strcpy(addr.sun_path, sockPath);
		
		if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
			std::cerr << "error: connecting to domain socket failed" << std::endl;
			disconnect();
			return false;
		}

		return true;
	}

	int recvFD(){
		if(sock < 0)
			return (-1);

		struct msghdr msg = {0};

		char m_buffer[256];
		struct iovec io = { .iov_base = m_buffer, .iov_len = sizeof(m_buffer) };
		msg.msg_iov = &io;
		msg.msg_iovlen = 1;

		char c_buffer[256];
		msg.msg_control = c_buffer;
		msg.msg_controllen = sizeof(c_buffer);

		int recvBytes = 0;
		
		do{
			recvBytes = recvmsg(sock, &msg, 0);
		}while(recvBytes == 0);

		if (recvBytes < 0)
			return (-1);

		struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);

		unsigned char * data = CMSG_DATA(cmsg);

		int fd = *((int*) data);

		return fd;
	}

};
