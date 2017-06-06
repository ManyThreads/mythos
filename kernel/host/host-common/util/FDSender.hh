/* -*- mode:C++; indent-tabs-mode:nil; -*- */
/* MIT License -- MyThOS: The Many-Threads Operating System
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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
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
