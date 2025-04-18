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
#define DEFAULT_PORT_SPEED 38400

class PrivatePort;

class PortListener
{
public:
	virtual void on_line_read(const char* line) {}
	virtual void on_partial(const char* line) {}
	virtual void on_partial_x(const char* line, int len) {}
};

class Port {

public:
	virtual ~Port();

	void listen(unsigned int ms);
	void close();

	void set_handler(PortListener* listener);

	void debug(bool dbg=true) { trace = dbg; }

	void set_speed(unsigned int requested_speed) { speed = requested_speed; }

protected:
	Port(const char* name);

	virtual void _open() = 0;
	virtual void _close() = 0;
	virtual int _read(bool &nothing_to_read, bool &error) = 0;
	virtual bool is_open() = 0;

	unsigned int speed;

	char port_name[16];

private:
	int open();
	int process_char(char c);

	char read_buffer[PORT_BUFFER_SIZE];
	unsigned int pos;

	bool trace = false;

	PortListener* listener;

	unsigned long bytes;

	unsigned int last_speed;

	unsigned long last_open_try;
};

#endif // PORTS_H_
