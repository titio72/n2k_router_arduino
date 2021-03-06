/*
 * Ports.c
 *
 *  Created on: Sep 29, 2018
 *      Author: aboni
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "constants.h"

#ifndef ESP32_ARCH
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#else
#include <Arduino.h>
#endif

#include "Ports.h"
#include "Log.h"
#include "Utils.h"

Port::Port(const char *port_name)
{
	port = strdup(port_name);
	fun = NULL;
	tty_fd = 0;
}

Port::~Port()
{
	delete port;
}

void Port::set_handler(int (*fun)(const char *))
{
	Port::fun = fun;
}

int Port::process_char(unsigned char c)
{
	int res = 0;
	if (c != 10 && c != 13)
	{
		read_buffer[pos] = c;
		pos++;
		pos %= PORT_BUFFER_SIZE; // avoid buffer overrun
	}
	else if (pos != 0)
	{
		if (fun)
		{
			(*fun)(read_buffer);
		}
		//if (trace) {
		//	Log::trace("Read {%s}\n", read_buffer);
		//}
		pos = 0;
		res = 1;
	}
	read_buffer[pos] = 0;
	return res;
}

#ifndef ESP32_ARCH
int fd_set_blocking(int fd, int blocking)
{
	// Save the current flags
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return 0;

	if (blocking)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;
	return fcntl(fd, F_SETFL, flags) != -1;
}
#endif

void Port::close()
{
#ifdef ESP32_ARCH
	Serial2.end();
#else
	::close(tty_fd);
#endif
	tty_fd = 0;
}

int Port::open()
{
#ifdef ESP32_ARCH
	Log::trace("[GPS] Opening serial port ");
	Serial2.begin(speed, SERIAL_8N1, RXD2, TXD2);
	tty_fd = 1;
	Log::trace("Ok\n");
#else
	struct termios tio;

	memset(&tio, 0, sizeof(tio));
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag = CS8 | CREAD | CLOCAL; // 8n1, see termios.h for more information
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 5;

	Log::trace("Opening port {%s}\n", port);

	tty_fd = ::open(port, O_RDONLY | O_NONBLOCK); // O_NONBLOCK might override VMIN and VTIME, so read() may return immediately.
	if (tty_fd > 0)
	{
		fd_set_blocking(tty_fd, 0);
		cfsetospeed(&tio, B38400);
		cfsetispeed(&tio, B38400);
		tcsetattr(tty_fd, TCSANOW, &tio);
	}

	Log::trace("Err opening port {%s} {%d} {%s}\n", port, errno, strerror(errno));

#endif
	return tty_fd != 0;
}

unsigned long Port::get_bytes()
{
	return bytes_read_since_reset;
}

void Port::reset_bytes()
{
	bytes_read_since_reset = 0;
}

void Port::listen(uint ms)
{
	static unsigned int last_speed = speed;

	unsigned char c;
	ulong t0 = _millis();

	if (last_speed != speed && tty_fd == 0) {
		Log::trace("[GPS] Resetting speed {%d}\n", speed);
		close();
		last_speed = speed;
	}

	while (tty_fd == 0 && (_millis() - t0) < ms)
	{
		if (!open())
		{
			sleep(250);
		}
	}

	if (tty_fd)
	{
		while (!stop)
		{
#ifdef ESP32_ARCH
			int _c = Serial2.read();
			int bread = (_c != -1) ? 1 : 0;
			int errno = (_c != -1) ? 0 : 11; // simulate
			c = (unsigned char)_c;
#else
			ssize_t bread = read(tty_fd, &c, 1);
#endif
			if (bread > 0)
			{
				bytes_read_since_reset += bread;
				if (process_char(c))
				{
					if ((_millis() - t0) > ms)
					{
						stop = true;
					}
				}
			}
			else
			{
				if (errno == 11)
				{
					// nothing to read
					return;
				}
				else
				{
					Log::trace("Err reading port {%s} {%d} {%s}\n", port, errno, strerror(errno));
					close();
					return;
				}
			}
		}
	}
}
