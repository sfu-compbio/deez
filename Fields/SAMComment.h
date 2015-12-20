#ifndef SAMComment_H
#define SAMComment_H

#include "../Common.h"
#include <unordered_map>

struct SAMComment {
	std::vector<std::string> lines;
	std::unordered_map<std::string, int> PG, RG;
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> SQ;
	SAMComment(const std::string &comment);
};

#endif // SAMComment_H