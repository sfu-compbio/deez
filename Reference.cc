#include "Reference.h"
#include <sys/stat.h>
using namespace std;

static const int MAX_CHROMOSOME = 400 * MB;

string full_path (const string &s);

Reference::Reference (const string &filename) {
	chromosomes["*"] = 0;

	struct stat st;
	lstat(filename.c_str(), &st);
	if (filename == "" || S_ISDIR(st.st_mode)) {
		directory = (filename == "") ? "." : filename;
		LOG("Using directory %s for searching chromosome FASTA files", directory.c_str());
		input = 0;
		return;
	}

	input = fopen(filename.c_str(), "rb");
	if (input == NULL)
		throw DZException("Cannot open the file %s", filename.c_str());
	DEBUG("Loaded reference file %s", full_path(filename).c_str());
	//c = fgetc(input);
	//chromosome.reserve(MAX_CHROMOSOME);

	FILE *fastaidx = fopen(string(filename + ".fai").c_str(), "rb");
	if (fastaidx != 0) {
		char chr[50];
		size_t loc;
		while (fscanf(fastaidx, "%s %*lu %lu %*lu %*lu", chr, &loc) != EOF) 
			chromosomes[chr] = loc;
	}
	else {
		LOG("FASTA index not found, creating one ...");
		fastaidx = fopen(string(filename + ".fai").c_str(), "wb");
		string chr = "";
		size_t cnt = 0, cntfull = 0, pos;
		while ((c = fgetc(input)) != EOF) {
			if (c == '>') {
				if (chr != "") fprintf(fastaidx, "%s\t%lu\t%lu\t%lu\t%lu\n", chr.c_str(), cnt, pos, cnt, cntfull);

				chr = "";
				cnt = cntfull = 0;
				
				c = fgetc(input);
				while (!isspace(c) && c != EOF) 
					chr += c, c = fgetc(input);
				while (c != '\n' && c != EOF) 
					c = fgetc(input);
			//	LOG("%s", chr.c_str());
				
				pos = ftell(input);
				chromosomes[chr] = pos;
				continue;
			}
			if (!isspace(c)) cnt++;
			cntfull++;
		}
		if (chr != "") fprintf(fastaidx, "%s\t%lu\t%lu\t%lu\t%lu\n", chr.c_str(), cnt, pos, cnt, cntfull);
		fseek(input, 0, SEEK_SET);
	}
	fclose(fastaidx);
}

Reference::~Reference (void) {
	if (input) fclose(input);
}

string Reference::getChromosomeName (void) const {
	return currentChr;
}

size_t Reference::getChromosomeLength(const std::string &s) const {
	auto it = chromosomes.find(s);
	if (it != chromosomes.end())
		return it->second;
	else
		throw DZException("Chromosome %s not found", s.c_str());
}

std::string Reference::scanChromosome (const string &s) {
	if (s == "*")
		return currentChr = "*";
	if (directory == "") {
		map<string, size_t>::iterator it = chromosomes.find(s);
		if (it == chromosomes.end())
			throw DZException("Chromosome %s not found in the reference!", s.c_str());
		fseek(input, it->second, SEEK_SET);
	}
	else {
		string filename = directory + "/" + s + ".fa";
		if (input) fclose(input);
		input = fopen(filename.c_str(), "rb");
		if (input == NULL)
			throw DZException("Cannot open chromosome file %s", filename.c_str());
		DEBUG("Loaded reference file %s", full_path(filename).c_str());

		char c;
		while ((c = fgetc(input)) != EOF) {
			if (c == '>') {
				string chr;
				size_t pos = ftell(input);
				c = fgetc(input);
				while (!isspace(c) && c != EOF) 
					chr += c, c = fgetc(input);
			//	LOG("%s", chr.c_str());
				chromosomes[chr] = pos;
			}
		}
		fseek(input, 0, SEEK_SET);
		fgetc(input);
	}
	//c = fgetc(input);
	//if(c!='>') throw DZException("eeee %c", c);
	//assert(c == '>');
	
	currentChr = s;
	c = fgetc(input);
	if (c == '>') {
		currentChr = "";
		while (!isspace(c) && c != EOF) 
			currentChr += c, c = fgetc(input);
		// skip fasta comment
		while (c != '\n') 
			c = fgetc(input);
		// park at first nucleotide
		c = fgetc(input);
	}
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
