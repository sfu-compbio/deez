#include "Reference.h"
using namespace std;

static const int MAX_CHROMOSOME = 400 * MB;

Reference::Reference (const string &filename) {
	input = fopen(filename.c_str(), "rb");
	if (input == NULL)
		throw DZException("Cannot open the file %s", filename.c_str());
	c = fgetc(input);
	//chromosome.reserve(MAX_CHROMOSOME);

    // char chrName[100];
    // int chrIdx = 0;
    // while (fgets(chrName, 100, input))
    //     if (chrName[0] == '>') {
    //         string cn = string(chrName + 1);
    //         cn = cn.substr(0, cn.size() - 1);
				// size_t comment = cn.find(' ');
				// if (comment != string::npos)
				// 	cn = cn.substr(0, comment);
    //         chrStr.push_back(cn);
    //         chrInt[cn] = chrIdx++;
    //     }
    // chrStr.push_back("*");
    // chrInt["*"] = chrIdx;
    // fseek(input, 0, SEEK_SET);
}

Reference::~Reference (void) {
	fclose(input);
}

string Reference::getChromosomeName (void) const {
	return currentChr;
}

std::string Reference::scanNextChromosome (void) {
	while (c != '>' && c != EOF)
		c = fgetc(input);
	
	// special case
	if (feof(input))
		return currentChr = "*";

	currentChr = "";
	c = fgetc(input);
	while (!isspace(c)) 
		currentChr += c, c = fgetc(input);
	// skip fasta comment
	while (c != '\n') 
		c = fgetc(input);

	// park at first nucleotide
	c = fgetc(input);
	if (c == '>' || c == EOF)
		throw "empty chromosome";
	currentPos = 0;

	return currentChr;
}

void Reference::load (char *arr, size_t s, size_t e) {
	//DEBUG("LOAD %'lu-%'lu",s,e);
	size_t i = 0;
	while (currentPos < e) {
		if (currentPos >= s) 
			arr[i++] = c;

		c = fgetc(input);
		while (isspace(c))
			c = fgetc(input);
		if (c == '>' || c == EOF) {
			while (i < e-s)
				arr[i++] = 'N';
			break;
			// throw "outside chromosome range"; // fill with Ns?
		}

		currentPos++;
	}
	assert(i==e-s);
}
