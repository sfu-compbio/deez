#include "Reference.h"
using namespace std;

static const int MAX_CHROMOSOME = 400 * MB;

Reference::Reference (const string &filename) {
	input = fopen(filename.c_str(), "rb");
	if (input == NULL)
		throw DZException("Cannot open the file %s", filename.c_str());
	chromosome.reserve(MAX_CHROMOSOME);

    char chrName[100];
    int chrIdx = 0;
    while (fgets(chrName, 100, input))
        if (chrName[0] == '>') {
            string cn = string(chrName + 1);
            cn = cn.substr(0, cn.size() - 1);
            chrStr.push_back(cn);
            chrInt[cn] = chrIdx++;
        }
    chrStr.push_back("*");
    chrInt["*"] = chrIdx;
    fseek(input, 0, SEEK_SET);
}

Reference::~Reference (void) {
	fclose(input);
}

string Reference::getChromosomeName (void) const {
	return chromosomeName;
}

size_t Reference::getChromosomeLength (void) const {
	return chromosome.size();
}

size_t Reference::readNextChromosome (void) {
	if (feof(input)) {
		chromosomeName = "*";
		return 0;
	}
	chromosomeName = "";
	char c = fgetc(input); // avoid >
   if (c == '>')
		c = fgetc(input);
	while (c != '\r' && c != '\n' && c != EOF) {
		chromosomeName += c;
		c = fgetc(input);
	}
	while (!feof(input)) {
		if (c == '>')
			break;
		if (c != '\n' && c != '\r')
			chromosome.push_back(toupper(c));
		c = fgetc(input);
	}
	LOG("%s (%d) loaded", chromosomeName.c_str(), chromosome.size());

	return chromosome.size();
}

char Reference::operator[](size_t i) const {
    if (i > chromosome.size())
        throw DZException("Access outside chromosome range");
    return chromosome[i];
}

