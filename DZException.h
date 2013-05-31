#ifndef DZException_H
#define DZException_H

#include <stdio.h>
#include <stdarg.h>
#include <exception>

class DZException: public std::exception {
	char msg[256];

public:
	DZException (void) {}
	DZException (const char *s, ...) {
		va_list args;
		va_start(args, s);
		vsprintf(msg, s, args);
		va_end(args);
	}

	const char *what (void) const throw() {
		return msg;
	}
};

#endif
