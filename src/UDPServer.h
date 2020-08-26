/*
 * UDPServer.h
 *
 *  Created on: Oct 1, 2018
 *      Author: aboni
 */

#ifndef UDPSERVER_H_
#define UDPSERVER_H_

class UDPSrv {
public:
	UDPSrv(int port, const char* net_dest);
	virtual ~UDPSrv();

	bool send(const char* message);

	void debug(bool dbg=true) { trace = dbg; };

private:
	int open_udp();
	int send_udp(const char* message);
	void close_udp();

	int sock_fd;
	int port;
	const char* net;

	bool trace = false;
};

#endif /* UDPSERVER_H_ */
