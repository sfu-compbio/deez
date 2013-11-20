#include "Reference.h"
using namespace std;

static const int MAX_CHROMOSOME = 400 * MB;

Reference::Reference (const string &filename) {
	input = fopen(filename.c_str(), "rb");
	if (input == NULL)
		throw DZException("Cannot open the file %s", filename.c_str());
	//c = fgetc(input);
	//chromosome.reserve(MAX_CHROMOSOME);

	FILE *fastaidx = fopen(string(filename + ".dzrefidx").c_str(), "rb");
	if (fastaidx != 0) {
		char chr[50];
		size_t loc;
		while (fscanf(fastaidx, "%s %lu", chr, &loc) != EOF) 
			chromosomes[chr] = loc;
	}
	else {
		LOG("FASTA index not found, creating one ...");
		fastaidx = fopen(string(filename + ".dzrefidx").c_str(), "wb");
		while ((c = fgetc(input)) != EOF) {
			if (c == '>') {
				string chr;
				size_t pos = ftell(input);
				c = fgetc(input);
				while (!isspace(c) && c != EOF) 
					chr += c, c = fgetc(input);
			//	LOG("%s", chr.c_str());
				fprintf(fastaidx, "%s %lu\n", chr.c_str(), pos);
				chromosomes[chr] = pos;
			}
		}
		fseek(input, 0, SEEK_SET);
	}
	fclose(fastaidx);
	chromosomes["*"] = 0;
}

Reference::~Reference (void) {
	fclose(input);
}

string Reference::getChromosomeName (void) const {
	return currentChr;
}

std::string Reference::scanChromosome (const string &s) {
	map<string, size_t>::iterator it = chromosomes.find(s);
	if (it == chromosomes.end())
		throw DZException("Chromosome %s not found in the reference!", s.c_str());
	if (s == "*")
		return currentChr = "*";
	
	fseek(input, it->second, SEEK_SET);
	//c = fgetc(input);
	//if(c!='>') throw DZException("eeee %c", c);
	//assert(c == '>');
	
	currentChr = "";
	c = fgetc(input);
	while (!isspace(c) && c != EOF) 
		currentChr += c, c = fgetc(input);
	// skip fasta comment
	while (c != '\n') 
		c = fgetc(input);
	// park at first nucleotide
	c = fgetc(input);
	if (c == '>' || c == EOF)
		throw DZException("Empty chromosome %s", currentChr.c_str());
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
