#pragma once

#include <sys/socket.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>

class FDSender{
	struct sockaddr_un addr;
	int sock, connection;


public:
	FDSender()
		: sock(-1)
		, connection(-1)
	{
	}

	~FDSender(){
		closeConnection();
	}

	void closeConnection(){
		if(connection >= 0)
			close(connection);
		if(sock >= 0)
			close(sock);
	}

	bool awaitConnection(const char *sockPath){
		closeConnection();

		unlink(sockPath);

		sock = socket(PF_LOCAL, SOCK_STREAM, 0);	
		if (sock == -1){
			std::cerr << "error: unable to open socket" << std::endl;
			return false; 
		}

		addr.sun_family = AF_LOCAL;
		strcpy(addr.sun_path, sockPath);
		
		if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
			std::cerr << "error: binding domain socket failed" << std::endl;
			closeConnection();
			return false;
		}
		
		if (listen(sock, 0) == -1){
			std::cerr << "error: listen failed" << std::endl;
			closeConnection();
			return false;
		}
		
		if ((connection = accept(sock, NULL, 0)) == -1){
			std::cerr << "error: accept failed" << std::endl;
			closeConnection();
			return false;
		}

		return true;
	}

	void sendFD(int fd){
		if(sock < 0)
			return;

		struct msghdr msg;

		char buf[CMSG_SPACE(sizeof fd)];
		memset(buf, 0, sizeof(buf));
		struct iovec io;

		char c[] = {'\0', '\0', '\0'};
		io.iov_base = c;
		io.iov_len = sizeof(c);

		msg.msg_iov = &io;
		msg.msg_iovlen = 1;
		msg.msg_control = buf;
		msg.msg_controllen = sizeof buf;

		struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof fd);

		*((int *) CMSG_DATA(cmsg)) = fd;

		msg.msg_controllen = cmsg->cmsg_len; 

		if(sendmsg(connection, &msg, 0) == -1)
			std::cerr << "sendmsg failed" << std::endl;
	}

};
