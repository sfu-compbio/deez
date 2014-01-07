#include <string>
#include <stdio.h>
#include <locale.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/time.h>

#include "Common.h"
#include "Compress.h"
#include "Decompress.h"
#include "Sort.h"
using namespace std;

bool optTest 	= false;
bool optSort 	= false;
bool optForce 	= false;
bool optStdout  = false;
string optRef 	 = "";
string optInput  = "";
string optRange  = "";
string optOutput = "";
size_t optBlock = 1000000;
char optQuality = 0;
char optLossy   = 0;
int optThreads    = 4;
size_t optSortMemory = GB;

void parse_opt (int argc, char **argv) {
	int opt; 
	struct option long_opt[] = {
		{ "help",      0, NULL, 'h' },
		{ "reference", 1, NULL, 'r' },
		{ "force",     0, NULL, 'f' },
		{ "test",      0, NULL, 'T' },
		{ "threads",   1, NULL, 't' },
		{ "stdout",    0, NULL, 'c' },
		{ "output",    1, NULL, 'o' },
		{ "lossy",     1, NULL, 'l' },
		{ "sort",      0, NULL, 's' },
		{ "sortmem",   1, NULL, 'M' },
		{ "quality",   1, NULL, 'q' },
		{ NULL, 0, NULL, 0 }
	};
	const char *short_opt = "hr:t:Tfco:q:l:sM:";
	do {
		opt = getopt_long (argc, argv, short_opt, long_opt, NULL);
		switch (opt) {
			case 'h':
				exit(0);
			case 'r':
				optRef = optarg;
				break;
			case 'T':
				optTest = true;
				break;
			case 't':
				optThreads = atoi(optarg);
				break;
			case 's':
				optSort = true;
				break;
			case 'M': {
				char c = optarg[strlen(optarg) - 1];
				optSortMemory = atol(optarg);
				if (c == 'K') optSortMemory *= KB;	
				if (c == 'M') optSortMemory *= MB;
				if (c == 'G') optSortMemory *= GB;
				break;
			}
			case 'f':
				optForce = true;
				break;
			case 'c':
				optStdout = true;
				break;
			case 'o':
				optOutput = optarg;
				break;
			case 'q':
				if (!strcmp(optarg, "samcomp") || atoi(optarg) == 1)
					optQuality = 1;
				else if (!strcmp(optarg, "test") || atoi(optarg) == 2)
					optQuality = 2;
				else 
					throw DZException("Unknown quality compression mode %s", optarg);
				break;
			case 'l':
				optLossy = atoi(optarg);
				if (optLossy < 0 || optLossy > 100)
					throw DZException("Invalid quality lossy sensitivity value %d", optLossy);
				break;
			case -1:
				break;
			default: {
				exit(1);
			}
		}
	} while (opt != -1);
	optInput = argv[optind];
	if (optind < argc - 1)
		optRange = argv[++optind];
}

int64_t dz_time (void) {
	struct timeval t;
	gettimeofday(&t, 0);
	return (t.tv_sec * 1000000ll + t.tv_usec) / 1000000ll;
}

bool file_exists (const string &s) {
	return access(s.c_str(), F_OK) != -1;
}

string full_path (const string &s) {
	char *pp = realpath(s.c_str(), 0);
	string p = pp;
	free(pp);
	return p;
}

string remove_extension (const string &s) {
	int i = s.find_last_of(".");
	if (i == string::npos) 
		i = s.size();
	return s.substr(0, i);
}

bool is_dz_file (const string &s) {
	FILE *fi = fopen(s.c_str(), "rb");
	uint32_t magic;
	if (fread(&magic, 4, 1, fi) != 1)
		throw DZException("Cannot detect input file type");
	fclose(fi);
	return (magic >> 8) == (MAGIC >> 8);
}

void compress (const string &in, const string &out) {
	if (is_dz_file(in))
		throw DZException("Cannot compress DZ file %s", in.c_str());
	if (file_exists(out)) {
		if (!optForce)
			throw DZException("File %s already exists. Use -f to overwrite", out.c_str());
		else
			LOG("File %s already exists. Overwriting it.", out.c_str());
	}
	DEBUG("Using output file %s", out.c_str());
	LOG("Compressing %s to %s ...", in.c_str(), out.c_str());
	FileCompressor sc(out, in, optRef, optBlock);
	sc.compress();
}

void decompress (const string &in, const string &out) {
	if (!is_dz_file(in))
		throw DZException("File %s is not DZ file", in.c_str());
	if (!optStdout && file_exists(out)) {
		if (!optForce)
			throw DZException("File %s already exists. Use -f to overwrite", out.c_str());
		else
			LOG("File %s already exists. Overwriting it.", out.c_str());
	}
	if (!optStdout) DEBUG("Using output file %s", out.c_str());
	LOG("Decompressing %s to %s ...", in.c_str(), optStdout ? "stdout" : out.c_str());
	FileDecompressor sd(in, out, optRef, optBlock);
	if (optRange == "")
		sd.decompress();
	else
		sd.decompress(in + "i", optRange);
}

void test (const string &s) {
	string tmp = s + ".dztemp";
	LOG("Test prefix %s", tmp.c_str());

	int64_t t = dz_time();
	compress(s, tmp + ".dz");
	LOG("Compression time: %'ld", dz_time() - t);

	t = dz_time();
	decompress(tmp + ".dz", tmp + ".sam");
	LOG("Decompression time: %'ld", dz_time() - t);

	string cmd = "cmp " + s + " " + tmp + ".sam";
	system(cmd.c_str());
}

int main (int argc, char **argv) {
    setlocale(LC_ALL, "");
    parse_opt(argc, argv);

    try {
    	if (!file_exists(optInput))
			throw DZException("File %s does not exist", optInput.c_str());
		DEBUG("Using input file %s", full_path(optInput).c_str());

    	if (optSort) {
    		if (optStdout)
    			throw DZException("Sort mode cannot be used with stdout");
    		string output = optOutput;
    		if (output == "")
				output = optInput + ".sort";
			DEBUG("Sorting %s to %s witn %'lu memory", optInput.c_str(), output.c_str(), optSortMemory);
    		sortFile(optInput, output, optSortMemory);
    	}
    	else {
	    	if (!file_exists(optRef))
	    		throw DZException("Reference file %s does not exist", optRef.c_str());
	    	else DEBUG("Using reference file %s", full_path(optRef).c_str());
			DEBUG("Using block size %'lu", optBlock);

			if (optTest)
				test(optInput);
			else {
				bool isCompress = !is_dz_file(optInput);
				string output = optOutput;
				if (output == "" && !optStdout) {
					output = remove_extension(optInput) + ".dz";
					if (!isCompress) output += ".sam";
				}
				
				if (isCompress) 
					compress(optInput, output);
				else 
					decompress(optInput, output);
			}
		}
	}
	catch (DZException &e) {
		ERROR("DZ error: %s!", e.what());
		exit(1);
	}
	catch (...) {
		ERROR("Unknow error ocurred!");	
		exit(1);
	}
	
	return 0;
}
