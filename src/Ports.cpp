#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifndef ESP32_ARCH
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#else
#include <Arduino.h>
#endif

#include "Constants.h"
#include "Ports.h"
#include "Log.h"
#include "Utils.h"


Port::Port(const char *port_name, unsigned long *bytes_read): bytes(bytes_read)
{
	port = strdup(port_name);
	listener = NULL;
	tty_fd = 0;
}

Port::~Port()
{
	delete port;
}

void Port::set_handler(PortListener* l)
{
	listener = l;
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
		if (listener)
		{
			listener->on_line_read(read_buffer);
		}
		if (trace) {
			Log::trace("Read {%s}\n", read_buffer);
		}
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
	if (tty_fd) ::close(tty_fd);
#endif
	tty_fd = 0;
}

int Port::open()
{
#ifdef ESP32_ARCH
	Log::trace("[GPS] Opening serial port at {%d} BPS ");
	Serial2.begin(speed, SERIAL_8N1, RXD2, TXD2);
	tty_fd = 1;
	Log::trace("Ok\n");
#else
	if (tty_fd==0)
	{
		struct termios tio;

		memset(&tio, 0, sizeof(tio));
		tio.c_iflag = 0;
		tio.c_oflag = 0;
		tio.c_cflag = CS8 | CREAD | CLOCAL; // 8n1, see termios.h for more information
		tio.c_lflag = 0;
		tio.c_cc[VMIN] = 1;
		tio.c_cc[VTIME] = 5;

		Log::trace("[GPS] Opening port {%s} at {%d} BPS\n", port, speed);

		tty_fd = ::open(port, O_RDONLY | O_NONBLOCK); // O_NONBLOCK might override VMIN and VTIME, so read() may return immediately.
		Log::trace("Err opening port {%d} {%s} {%d} {%s}\n", tty_fd, port, errno, strerror(errno));
		if (tty_fd > 0)
		{
			speed_t bps;
			switch (speed)
			{
				case 4800: bps = B4800; break;
				case 9600: bps = B9600; break;
				case 19200: bps = B19200; break;
				case 38400: bps = B38400; break;
				case 57600: bps = B57600; break;
				case 115200: bps = B115200; break;
				default:
				bps = B38400;
			}
			fd_set_blocking(tty_fd, 0);
			Log::trace("Err opening port {%s} {%d} {%s}\n", port, errno, strerror(errno));
			cfsetospeed(&tio, bps);
			Log::trace("Err opening port {%s} {%d} {%s}\n", port, errno, strerror(errno));
			cfsetispeed(&tio, bps);
			Log::trace("Err opening port {%s} {%d} {%s}\n", port, errno, strerror(errno));
			tcsetattr(tty_fd, TCSANOW, &tio);
			Log::trace("Err opening port {%s} {%d} {%s}\n", port, errno, strerror(errno));
		}
	}
#endif
	return tty_fd != 0;
}

void Port::listen(uint ms)
{
	static unsigned int last_speed = speed;

	unsigned char c;
	ulong t0 = _millis();

	if (last_speed != speed && tty_fd != 0) {
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
			int nothing_to_read = (_c != -1) ? 0 : 11; // simulate
			c = (unsigned char)_c;
#else
			ssize_t bread = read(tty_fd, &c, 1);
			int nothing_to_read = (errno==11); // simulate
#endif
			if (bread > 0)
			{
				(*bytes) = (*bytes)+1;
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
				if (nothing_to_read)
				{
					// nothing to read
					return;
				}
				else
				{
					//Log::trace("Err reading port {%s} {%d} {%s}\n", port, errno, strerror(errno));
					close();
					return;
				}
			}
		}
	}
}
