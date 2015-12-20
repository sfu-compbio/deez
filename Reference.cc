#include "Reference.h"
#include <sstream>
#include <sys/stat.h>
using namespace std;

#ifdef OPENSSL
#include <openssl/md5.h>
#endif

// Load order:
// 	-r
//		if FASTA file, provided file
// 		if directory, <chr>.fa
//	SQ, if exists
//  No file

Reference::Reference (const string &fn):
	input(nullptr), bufferStart(0), bufferEnd(0)
{
	chromosomes["*"] = {"*", "", "", 0, 0};

	if (fn == "") { // Load filename later via SQ header, if applicable
		return;
	} else if (!File::IsWeb(fn)) { // If directory, load later
		struct stat st;
		lstat(fn.c_str(), &st);
		if (fn == "" || S_ISDIR(st.st_mode)) {
			directory = fn;
			LOG("Using directory %s for searching chromosome FASTA files", directory.c_str());
			return;
		}
	} else if (fn.size() > 0 && fn[fn.size() - 1] == '/') { // Web directory
		directory = fn;
		LOG("Using web directory %s for searching chromosome FASTA files", directory.c_str());
		return;
	}
	loadFromFASTA(fn);
}

Reference::~Reference (void) 
{
}

void Reference::loadFromFASTA (const string &fn)
{
	ZAMAN_START(Reference_LoadFASTA);
	input = File::Open(fn, "rb");
	LOG("Loaded reference file %s", fn.c_str());

	try { // Try loading .fai file
		string faidx = fn + ".fai";
		shared_ptr<File> fastaidx;
		if (File::IsWeb(faidx)) {
			fastaidx = WebFile::Download(faidx);
		} else {
			fastaidx = make_shared<File>(faidx, "rb");
		}
		if (fastaidx == nullptr)
			throw DZException("Cannot open FASTA index %s", faidx.c_str());
		char chr[50];
		size_t len, loc;
		while (fscanf((FILE*) fastaidx->handle(), "%s %lu %lu %*lu %*lu", chr, &len, &loc) != EOF) 
			chromosomes[chr] = {chr, "", fn, len, loc};
		LOG("Loaded reference index %s", faidx.c_str());
	} catch (DZException &e) {
		FILE *fastaidx = 0;
		if (File::IsWeb(fn)) {
			input = WebFile::Download(fn, true);
			currentWebFile = fn;
		} else {
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

	filename = File::FullPath(fn);
	ZAMAN_END(Reference_LoadFASTA);
}

string Reference::getChromosomeName (void) const 
{
	return currentChr;
}

size_t Reference::getChromosomeLength(const std::string &s) const 
{
	auto it = chromosomes.find(s);
	if (it != chromosomes.end()) {
		return it->second.len;
	} else {
		return 0;
	}
}

std::string Reference::scanChromosome (const string &s, const SAMComment &samComment) 
{
	buffer = "";
	bufferStart = bufferEnd = currentPos = 0;

	if (s == "*")
		return currentChr = s;

	try {
		if (directory == "") {
			auto it = chromosomes.find(s);
			if (it == chromosomes.end()) 
				throw DZException("Chromosome %s not found in the reference", s.c_str());
			input->seek(it->second.loc);
		} else {
			string fn = directory + "/" + s + ".fa";
			if (directory.size() && directory[directory.size() - 1] == '/')
				fn = directory + s + ".fa";
			if (File::IsWeb(fn)) {
				input = WebFile::Download(fn, true);
				currentWebFile = fn;
			} else {
				input = File::Open(fn, "rb");
			}
			LOG("Loaded reference file %s for chromosome %s", fn.c_str(), s.c_str());
			filename = File::FullPath(fn);
		}
	} catch (DZException &e) {
		// Try Doing Sth Else
		try {
			auto it = samComment.SQ.find(s);
			if (it == samComment.SQ.end()) 
				throw DZException("Cannot find reference in @SQ:UR");
	
			auto jt = it->second.find("UR");
			if (jt != it->second.end() && File::IsWeb(jt->second) && currentWebFile != jt->second) {
				LOG("Loaded reference file %s for chromosome %s via @SQ:UR field", jt->second.c_str(), s.c_str());
				input = WebFile::Download(currentWebFile = jt->second, true);
				filename = File::FullPath(jt->second);
			} else {
				throw DZException("Cannot find reference in @SQ:UR");
			}
		} catch (DZException &e) {
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
	if (chromosomes[currentChr].loc) 
		chromosomes[currentChr].loc--;

	#if 0 && defined OPENSSL // calculate MD5 of chromosome
	ZAMAN_START(Reference_MD5);
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
	ZAMAN_END(Reference_MD5);
	#endif

	if (c == '>' || c == EOF) 
		return currentChr = s; // Empty chromosome

	return currentChr;
}

void Reference::loadIntoBuffer(size_t end)
{
	assert(currentPos == bufferEnd);
	if (end < bufferEnd) return;

	DEBUG("Loading to %'lu", end);

	buffer.reserve(buffer.size() + (end - bufferEnd + 1));
	bufferEnd = end;

	auto it = chromosomes.find(currentChr);
	if (it != chromosomes.end() && it->second.len > 0) {
		while (currentPos < bufferEnd) {
			buffer += c;
			
			c = input->getc();
			while (isspace(c) && c != EOF) 
				c = input->getc();
			if (c == '>' || c == EOF) 
				break;
			currentPos++;
		}
	}
	while (currentPos < bufferEnd) 
		buffer += 'N', currentPos++;
}

// Assumes that everything is loaded
char Reference::operator[](size_t pos) const
{
	assert(pos >= bufferStart); // Access is sequential
	assert(pos < bufferEnd);

	return buffer[pos - bufferStart];
}

string Reference::copy(size_t start, size_t end)
{
	assert(start >= bufferStart);
	if (end >= bufferEnd) 
		loadIntoBuffer(end + 10 * MB);
	assert(end < bufferEnd);

	return buffer.substr(start - bufferStart, end - start);
}

void Reference::trim(size_t start) 
{
	if (start >= bufferEnd) {
		buffer = "";
		bufferStart = bufferEnd = currentPos;
	} else {
		buffer = buffer.substr(start - bufferStart);
		bufferStart = start;
	}
}
