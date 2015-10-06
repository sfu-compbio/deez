#include "Reference.h"
#include <sstream>
#include <sys/stat.h>
using namespace std;

// Load order:
// 	-r
//		if FASTA file, provided file
// 		if directory, <chr>.fa
//	SQ, if exists
//  No file

Reference::Reference (const string &fn):
	input(0)
{
	chromosomes["*"] = {"*", "", "", 0, 0};

	if (fn == "") // Load filename later via SQ header, if applicable
		return;
	else if (!IsWebFile(fn)) { // If directory, load later
		struct stat st;
		lstat(fn.c_str(), &st);
		if (fn == "" || S_ISDIR(st.st_mode)) {
			directory = fn;
			LOG("Using directory %s for searching chromosome FASTA files", directory.c_str());
			return;
		}
	}
	else if (fn.size() > 0 && fn[fn.size() - 1] == '/') { // Web directory
		directory = fn;
		LOG("Using web directory %s for searching chromosome FASTA files", directory.c_str());
		return;
	}
	loadFromFASTA(fn);
}

Reference::~Reference (void) 
{
	if (input) delete input;
}

void Reference::loadFromFASTA (const string &fn)
{
	input = OpenFile(fn, "rb");
	LOG("Loaded reference file %s", fn.c_str());

	try { // Try loading .fai file
		string faidx = fn + ".fai";
		File *fastaidx;
		if (IsWebFile(faidx)) 
			fastaidx = WebFile::Download(faidx);
		else
			fastaidx = new File(faidx, "rb");
		if (!fastaidx)
			throw DZException("Cannot open FASTA index %s", faidx.c_str());
		char chr[50];
		size_t len, loc;
		while (fscanf((FILE*) fastaidx->handle(), "%s %lu %lu %*lu %*lu", chr, &len, &loc) != EOF) 
			chromosomes[chr] = {chr, "", fn, len, loc};
		delete fastaidx;
		LOG("Loaded reference index %s", faidx.c_str());
	}
	catch (DZException &e) {
		FILE *fastaidx = 0;
		if (IsWebFile(fn)) {
			delete input, input = 0;
			input = WebFile::Download(fn, true);
			currentWebFile = fn;
		}
		else {
			LOG("FASTA index for %s not found, creating one ...", fn.c_str());
			fastaidx = fopen(string(fn + ".fai").c_str(), "wb");
			if (!fastaidx)
				WARN("Cannot create reference index for %s", fn.c_str());
		}

		string chr = "";
		size_t cnt = 0, cntfull = 0, pos;
		while ((c = input->getc()) != EOF) {
			if (c == '>') {
				if (chr != "") {
					if (fastaidx) fprintf(fastaidx, "%s\t%lu\t%lu\t%lu\t%lu\t\n", chr.c_str(), cnt, pos, cnt, cntfull);
					chromosomes[chr].len = cnt;
				}

				chr = "";
				cnt = cntfull = 0;
				
				c = input->getc();
				while (!isspace(c) && c != EOF) 
					chr += c, c = input->getc();
				while (c != '\n' && c != EOF) {
					c = input->getc();
				}
				
				pos = input->tell();
				chromosomes[chr] = {chr, "", filename, 0, pos};
				continue;
			}
			if (!isspace(c)) 
				cnt++;
			cntfull++;
		}
		if (chr != "") {
			if (fastaidx) fprintf(fastaidx, "%s\t%lu\t%lu\t%lu\t%lu\n", chr.c_str(), cnt, pos, cnt, cntfull);
			chromosomes[chr].len = cnt;
		}
		if (fastaidx) fclose(fastaidx);
		input->seek(0);
	}

	filename = fullPath(fn);
}

string Reference::getChromosomeName (void) const 
{
	return currentChr;
}

size_t Reference::getChromosomeLength(const std::string &s) const 
{
	auto it = chromosomes.find(s);
	if (it != chromosomes.end())
		return it->second.len;
	else
		return 0;
}

void Reference::scanSAMComment (const string &comment) 
{	
	size_t pos;
	istringstream ss(comment);
	string l, lo;
	while (getline(ss, l)) {
		if (l.size() <= 4 || l.substr(0, 4) != "@SQ\t") continue;
		lo = l, l = l.substr(4);
		
		unordered_map<string, string> val;
		while (l.size() > 3 && (pos = l.find('\t')) != string::npos) {
			val[l.substr(0, 2)] = l.substr(3, pos - 3);
			l = l.substr(pos + 1);
		}
		if (l.size() > 3 && isalpha(l[0]))
			val[l.substr(0, 2)] = l.substr(3, pos - 3);
		//val["fullLine"] = lo;
		if (val.find("SN") != val.end()) 
			samCommentData[val["SN"]] = val;
	}

	for (auto &c: samCommentData) {
		DEBUGN("%s: ", c.first.c_str());
		for (auto &e: c.second)
			DEBUGN("%s->%s ", e.first.c_str(), e.second.c_str());
		DEBUG("");
	}
}

std::string Reference::scanChromosome (const string &s) 
{
	if (s == "*")
		return currentChr = s;

	try {
		if (directory == "") {
			auto it = chromosomes.find(s);
			if (it == chromosomes.end()) 
				throw DZException("Chromosome %s not found in the reference", s.c_str());
			input->seek(it->second.loc);
		}
		else {
			string fn = directory + "/" + s + ".fa";
			if (directory.size() && directory[directory.size() - 1] == '/')
				fn = directory + s + ".fa";
			delete input, input = 0;
			if (IsWebFile(fn)) {
				input = WebFile::Download(fn, true);
				currentWebFile = fn;
			}
			else
				input = OpenFile(fn, "rb");
			
			LOG("Loaded reference file %s for chromosome %s", fn.c_str(), s.c_str());
			/*char c;
			while ((c = input->getc()) != EOF) {
				if (c == '>') {
					string chr;
					size_t pos = input->tell();
					c = input->getc();
					while (!isspace(c) && c != EOF) 
						chr += c, c = input->getc();
					chromosomes[chr] = {chr, "", fn, 0, pos};
				}
			}
			input->seek(0);
			input->getc();*/

			filename = fullPath(fn);
		}
	}
	catch (DZException &e) {
		// Try Doing Sth Else
		try {
			auto it = samCommentData[s].find("UR");
			if (it != samCommentData[s].end() && IsWebFile(it->second) && currentWebFile != it->second) {
				LOG("Loaded reference file %s for chromosome %s via @SQ:UR field", it->second.c_str(), s.c_str());
				delete input, input = 0;
				input = WebFile::Download(currentWebFile = it->second, true);
				filename = fullPath(it->second);
			}
			else throw DZException("Cannot find reference in @SQ:UR");
		}
		catch (DZException &e) {
			WARN("Could not find reference file for chromosome %s, using no reference instead", s.c_str());
			filename = "";
			chromosomes[s] = {s, "", "<implicit>", 0, 0};
			return currentChr = s; // fill with Ns later
		}
	}
	
	currentChr = s;
	c = input->getc();
	if (c == '>') {
		currentChr = "";
		while (!isspace(c) && c != EOF) 
			currentChr += c, c = input->getc();
		currentChr = currentChr.substr(1);
		// skip fasta comment
		while (c != '\n') 
			c = input->getc();
		// park at first nucleotide
		c = input->getc();
	}
	chromosomes[currentChr].chr = currentChr;
	chromosomes[currentChr].filename = filename;
	chromosomes[currentChr].loc = input->tell();
	if (chromosomes[currentChr].loc) chromosomes[currentChr].loc--;

#ifdef OPENSSL // calculate MD5 of chromosome
	MD5_CTX ctx;
    MD5_Init(&ctx);
    size_t pos = input->tell(), cnt = 0;
    string buf = "";
    while (c != '>' && c != EOF) {
    //	LOGN("%c",c);
    	if (isalpha(c)) buf += c;
    	c = input->getc();
    	if (buf.size() == 1024 * 1024)
    		MD5_Update(&ctx, buf.c_str(), buf.size()), cnt += buf.size(), buf = "";
    }
	if (buf.size())
		MD5_Update(&ctx, buf.c_str(), buf.size()), cnt += buf.size(), buf = "";
	uint8_t md5[MD5_DIGEST_LENGTH];
	MD5_Final(md5, &ctx);
	string md5sum = "";
	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
		md5sum += S("%02x", md5[i]);
	chromosomes[currentChr].len = cnt;
	chromosomes[currentChr].md5 = md5sum;
	DEBUG("%s %s %d...",currentChr.c_str(),md5sum.c_str(),cnt);
	input->seek(pos);
#endif

	currentPos = 0;
	if (c == '>' || c == EOF) 
		return currentChr = s; // Empty chromosome
	return currentChr;
}

void Reference::load (char *arr, size_t s, size_t e) 
{
	auto it = chromosomes.find(currentChr);
	if (it == chromosomes.end() || it->second.len == 0) {
		for (size_t i = 0; i < e - s; i++)
			arr[i] = 'N';
		return;
	}

	size_t i = 0;
	while (currentPos < e) {
		if (currentPos >= s) 
			arr[i++] = c;

		c = input->getc();
		while (isspace(c) && c != EOF)
			c = input->getc();
		if (c == '>' || c == EOF) 
			break;
		currentPos++;
	}
	while (i < e - s)
		arr[i++] = 'N';
	assert(i == e - s);
}
