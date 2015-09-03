#include "Reference.h"
#include <sys/stat.h>
using namespace std;

static const int MAX_CHROMOSOME = 400 * MB;
extern bool optInvalidChr;

Reference::Reference (const string &filename) 
{
	chromosomes["*"] = 0;

	if (!IsWebFile(filename)) {
		struct stat st;
		lstat(filename.c_str(), &st);
		if (filename == "" || S_ISDIR(st.st_mode)) {
			directory = (filename == "") ? "." : filename;
			LOG("Using directory %s for searching chromosome FASTA files", directory.c_str());
			input = 0;
			return;
		}
	}
	else if (filename.size() > 0 && filename[filename.size() - 1] == '/') {
		directory = filename;
		LOG("Using web directory %s for searching chromosome FASTA files", directory.c_str());
		input = 0;
		return;
	}

	input = OpenFile(filename, "rb");
	DEBUG("Loaded reference file %s", filename.c_str());

	try {
		string faidx = filename + ".fai";
		FILE *fastaidx;
		if (IsWebFile(faidx)) 
			fastaidx = WebFile::Download(faidx);
		else
			fastaidx = fopen(faidx.c_str(), "rb");
		if (!fastaidx)
			throw DZException("Cannot open FASTA index %s", faidx.c_str());
		char chr[50];
		size_t loc;
		while (fscanf(fastaidx, "%s %*lu %lu %*lu %*lu", chr, &loc) != EOF) 
			chromosomes[chr] = loc;
		fclose(fastaidx);
	}
	catch (DZException &e) {
		LOG("%s...", e.what());
		FILE *fastaidx = 0;
		if (IsWebFile(filename)) {
			delete input;
			FILE *tmp = WebFile::Download(filename);
			input = new File(tmp);
		}
		else {
			LOG("FASTA index not found, creating one ...");
			fastaidx = fopen(string(filename + ".fai").c_str(), "wb");
			if (!fastaidx)
				throw DZException("Cannot create reference index for %s", filename.c_str());
		}

		string chr = "";
		size_t cnt = 0, cntfull = 0, pos;
		while ((c = input->getc()) != EOF) {
			if (c == '>') {
				if (chr != "" && fastaidx) 
					fprintf(fastaidx, "%s\t%lu\t%lu\t%lu\t%lu\n", chr.c_str(), cnt, pos, cnt, cntfull);

				chr = "";
				cnt = cntfull = 0;
				
				c = input->getc();
				while (!isspace(c) && c != EOF) 
					chr += c, c = input->getc();
				while (c != '\n' && c != EOF) 
					c = input->getc();
				
				pos = input->tell();
				chromosomes[chr] = pos;
				continue;
			}
			if (!isspace(c)) cnt++;
			cntfull++;
		}
		if (chr != "" && fastaidx) 
			fprintf(fastaidx, "%s\t%lu\t%lu\t%lu\t%lu\n", chr.c_str(), cnt, pos, cnt, cntfull);
		if (fastaidx)
			fclose(fastaidx);
		input->seek(0);
	}
}

Reference::~Reference (void) 
{
	if (input) delete input;
}

string Reference::getChromosomeName (void) const 
{
	return currentChr;
}

size_t Reference::getChromosomeLength(const std::string &s) const 
{
	auto it = chromosomes.find(s);
	if (it != chromosomes.end())
		return it->second;
	else if (optInvalidChr)
		return 0;
	else
		throw DZException("Chromosome %s not found", s.c_str());
}

std::string Reference::scanChromosome (const string &s) 
{
	if (s == "*")
		return currentChr = "*";
	if (directory == "") {
		map<string, size_t>::iterator it = chromosomes.find(s);
		if (it == chromosomes.end()) {
			if (optInvalidChr) 
				return currentChr = s;
			throw DZException("Chromosome %s not found in the reference", s.c_str());
		}
		input->seek(it->second);
	}
	else {
		string filename = directory + "/" + s + ".fa";
		if (directory.size() && directory[directory.size() - 1] == '/')
			filename = directory + s + ".fa";
		
		try {
			delete input;
			if (IsWebFile(filename)) {
				FILE *tmp = WebFile::Download(filename);
				input = new File(tmp);
			}
			else
				input = OpenFile(filename, "rb");
		}
		catch (DZException *e) {
			if (optInvalidChr) 
				return currentChr = s;
			throw;
		}
		DEBUG("Loaded reference file %s", filename.c_str());
		char c;
		while ((c = input->getc()) != EOF) {
			if (c == '>') {
				string chr;
				size_t pos = input->tell();
				c = input->getc();
				while (!isspace(c) && c != EOF) 
					chr += c, c = input->getc();
				chromosomes[chr] = pos;
			}
		}
		input->seek(0);
		input->getc();
	}
	
	currentChr = s;
	c = input->getc();
	if (c == '>') {
		currentChr = "";
		while (!isspace(c) && c != EOF) 
			currentChr += c, c = input->getc();
		// skip fasta comment
		while (c != '\n') 
			c = input->getc();
		// park at first nucleotide
		c = input->getc();
	}
	if (c == '>' || c == EOF) {
		if (optInvalidChr)
			return currentChr = s;
		else
			throw DZException("Empty chromosome %s", currentChr.c_str());
	}
	currentPos = 0;
	return currentChr;
}

void Reference::load (char *arr, size_t s, size_t e) 
{
	if (chromosomes.find(currentChr) == chromosomes.end()) {
		for (size_t i = 0; i < e - s; i++)
			arr[i] = 'N';
		return;
	}

	size_t i = 0;
	while (currentPos < e) {
		if (currentPos >= s) 
			arr[i++] = c;

		c = input->getc();
		while (isspace(c))
			c = input->getc();
		if (c == '>' || c == EOF) {
			while (i < e-s)
				arr[i++] = 'N';
			break;
		}
		currentPos++;
	}
	assert(i==e-s);
}
