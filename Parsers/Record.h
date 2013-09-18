
#ifndef Record_H
#define Record_H

#include <string>
#include <cstdlib>
#include "../Common.h"

const int MAXLEN = 8 * KB;
class Record {
    char line[MAXLEN];
    char *fields[12];

private:
    friend class SAMParser;

public:
    const char* getReadName() const { return fields[0]; }
    const int getMappingFlag() const { return std::atoi(fields[1]); }
    const char* getChromosome() const { return fields[2]; }
    const size_t getLocation() const { return std::atoll(fields[3]) - 1; }
    const char getMappingQuality() const { return atoi(fields[4]); }
    const char* getCigar() const { return fields[5]; }
    const char* getPairChromosome() const { return fields[6]; }
    const size_t getPairLocation() const { return std::atoll(fields[7]) - 1; }
    const int getTemplateLenght() const { return std::atoi(fields[8]); }
    const char* getSequence() const { return fields[9]; }
    const char* getQuality() const { return fields[10]; }
    const char* getOptional() const { return fields[11]; }
};

#endif // RECORD_H
