/*
 * Ports.h
 *
 *  Created on: Sep 29, 2018
 *      Author: aboni
 */

#ifndef PORTS_H_
#define PORTS_H_

#include <stdlib.h>

#define PORT_BUFFER_SIZE 1024

class PortListener
{
public:
	virtual int on_line_read(const char* line) = 0;
};

class Port {

public:
	Port(const char* port, unsigned long *bytes_read);
	virtual ~Port();

	void listen(unsigned int ms);
	void close();

	//void set_handler(int (*fun)(const char*));
	void set_handler(PortListener* listener);

	void debug(bool dbg=true) { trace = dbg; }

	void set_speed(unsigned int requested_speed) { speed = requested_speed; }

private:

	int open();
	int process_char(unsigned char c);

	int tty_fd = 0;

	char read_buffer[PORT_BUFFER_SIZE];
	unsigned int pos = 0;

	const char* port = NULL;
	unsigned int speed = 38400;

	bool stop = false;

	PortListener* listener;

	bool trace = false;

	unsigned long *bytes;
};

#endif // PORTS_H_
