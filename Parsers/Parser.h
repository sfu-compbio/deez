#ifndef Parser_H
#define Parser_H

#include <stdio.h>

#include "../Common.h"

class Parser {
protected:
	FILE *input;

public:
	virtual bool hasNext (void) = 0;
	virtual bool readNext (void) = 0;
};

#endif
