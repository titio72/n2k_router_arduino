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

class Port {

public:
	Port(const char* port);
	virtual ~Port();

	void listen(unsigned int ms);
	void close();

	void set_handler(int (*fun)(const char*));

	void debug(bool dbg=true) { trace = dbg; }

	void reset_bytes();

	unsigned long get_bytes();

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

	int (*fun)(const char*);

	bool trace = false;

	unsigned long bytes_read_since_reset;
};

#endif // PORTS_H_
