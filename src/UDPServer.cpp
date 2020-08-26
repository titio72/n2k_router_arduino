#include "constants.h"

#ifndef ESP32_ARCH
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "UDPServer.h"
#include "Log.h"

#define MAXLINE 1024

struct sockaddr_in     servaddr;

UDPSrv::UDPSrv(int _port, const char* _net_dest) {
	sock_fd = 0;
	port = _port;
	net = strdup(_net_dest);
}

UDPSrv::~UDPSrv() {
	close_udp();
	delete net;
}

bool UDPSrv::send(const char* message) {
	if (sock_fd<=0) {
		if (trace) {
			Log::trace("Opening port {%d} ", port);
		}
		sock_fd = open_udp();
		if (trace) {
			Log::trace("Port opened {%d} {%s}", port, (sock_fd>0)?"OK":"KO");
		}
	}

	if (sock_fd>0) {
		if (trace) {
			Log::trace("Sending UDP message {%s} ", message);
		}
		int sent = send_udp(message)!=0;
		if (trace) {
			Log::trace("Message sent to UDP {%s}", (sent>0)?"OK":"KO");
		}
		return sent>0;

	}

	return false;
}


int UDPSrv::open_udp() {
    int sockfd;
    char buffer[MAXLINE];

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(net);

    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
    }

    return sockfd;
}

int UDPSrv::send_udp(const char* message) {
    int n, len;
    if (sock_fd) {
		return sendto(sock_fd, (const char *)message, strlen(message),
			MSG_CONFIRM, (const struct sockaddr *) &servaddr,
				sizeof(servaddr));
    } else {
    	return 0;
    }
}

void UDPSrv::close_udp() {
    if (sock_fd) {
    	close(sock_fd);
    }
}
#endif