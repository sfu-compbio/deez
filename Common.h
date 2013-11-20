#ifndef Common_H
#define Common_H

#include <assert.h>
#include <inttypes.h>
#include <typeinfo>
#include "DZException.h"
#include "Logger.h"
#include "Array.h"
#include "CircularArray.h"

#define KB  1024
#define MB  KB * 1024
#define GB  MB * 1024

#define VERSION	0x10
#define MAGIC 	(0x07445A00 | VERSION)

double _zaman_ (void);
#define ZAMAN_START() {double _____zaman564265=_zaman_();
#define ZAMAN_END(s) DEBUGN("[%s] %.2lf ", s, _zaman_()-_____zaman564265);}

//#define DZ_EVAL

extern bool optStdout;

enum Fields {
	Sequence, 
	ReadName, 
	MappingFlag, 
	MappingLocation,
	MappingQuality, 
	QualityScore, 
	PairedEnd, 
	OptionalField
};

#endif
