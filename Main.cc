#include <string>
#include <stdio.h>
#include <locale.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/time.h>
#include <curl/curl.h>

#include "Common.h"
#include "Compress.h"
#include "Decompress.h"
#include "Sort.h"
#include "FileIO.h"
using namespace std;

bool optTest 	= false;
bool optSort 	= false;
bool optForce 	= false;
bool optStdout  = false;
bool optStats   = false;
bool optNoQual  = false;
bool optReadLossy = false;
bool optInvalidChr = false;
string optRef 	 = "";
vector<string> optInput;
string optRange  = "";
string optOutput = "";
// string optQuery  = "";
size_t optBlock = 1000000;
char optQuality = 0;
char optLossy   = 0;
int optThreads  = 4;
int optFlag     = 0;
int optLogLevel	= 0;
size_t optSortMemory = GB;

void parse_opt (int argc, char **argv) {
	int opt; 
	struct option long_opt[] = {
		{ "help",        0, NULL, 'h' },
		{ "reference",   1, NULL, 'r' },
		{ "force",       0, NULL, '!' },
		{ "test",        0, NULL, 'T' },
		{ "threads",     1, NULL, 't' },
		{ "stdout",      0, NULL, 'c' },
		{ "output",      1, NULL, 'o' },
		{ "read-lossy",  0, NULL, 'L' },
		{ "lossy",       1, NULL, 'l' },
		{ "sort",        0, NULL, 's' },
		{ "sortmem",     1, NULL, 'M' },
		{ "withoutflag", 1, NULL, 'F' },
		{ "withflag",    1, NULL, 'f' },
		{ "stats",       0, NULL, 'S' },
	//	{ "query",       1, NULL, 'w' },
		{ "noqual",      0, NULL, 'Q' },
		{ "quality",     1, NULL, 'q' },
	//	{ "block",       1, NULL, 'B' },
		{ "verbosity",   1, NULL, 'v' },
	//	{ "allow-invalid-ref", 0, NULL, 'I' },
		{ NULL, 0, NULL, 0 }
	};
	const char *short_opt = "hr:t:T!co:q:l:sM:Sf:F:QLv:" /*"w:IB:"*/;
	do {
		opt = getopt_long (argc, argv, short_opt, long_opt, NULL);
		switch (opt) {
			case 'h':
				WARN("Compression: deez -r [reference] [input.sam] -o [output]");
				WARN("Decompression: deez -r [reference] [input.dz] -o [output] ([region])");
				WARN("For additional parameter explanation, please consult the man page (less README.md or http://sfu-compbio.github.io/deez/)");
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
			case 'v':
				optLogLevel = atoi(optarg);
				break;
			case 's':
				optSort = true;
				break;
		//	case 'I':
		//		optInvalidChr = true;
		//		break;
			case 'Q':
				optNoQual = true;
				break;
			case 'M': {
				char c = optarg[strlen(optarg) - 1];
				optSortMemory = atol(optarg);
				if (c == 'K') optSortMemory *= KB;	
				if (c == 'M') optSortMemory *= MB;
				if (c == 'G') optSortMemory *= GB;
				break;
			}
			case '!':
				optForce = true;
				break;
			//case 'B':
			//	optBlock = atoi(optarg);
				//LOG("Block size changed to %'lu", optBlock);
			//	break;
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
			case 'L':
				optReadLossy = true;
				break;
			case 'S':
				optStats = true;
				break;
			case 'F':
				optFlag = -atoi(optarg);
				break;
			case 'f':
				optFlag = atoi(optarg);
				break;
		//	case 'w':
		//		optQuery = optarg;
		//		break;
			case -1:
				break;
			default: {
				exit(1);
			}
		}
	} while (opt != -1);
	while (optind < argc)
		optInput.push_back(argv[optind++]);
}

int64_t dz_time (void) {
	struct timeval t;
	gettimeofday(&t, 0);
	return (t.tv_sec * 1000000ll + t.tv_usec) / 1000000ll;
}

string remove_extension (const string &s) {
	int i = s.find_last_of(".");
	if (i == string::npos) 
		i = s.size();
	return s.substr(0, i);
}

bool is_dz_file (const string &s) {
	File *f = OpenFile(s.c_str(), "r");
	bool res = false;
	try {
		uint32_t magic = f->readU32();
		res = (magic >> 8) == (MAGIC >> 8);
	}
	catch (...) {}
	delete f;
	return res;
}

string sort (string output) {
	if (optInput.size() != 1)
		throw DZException("Only one file can be sorted per invocation.");
	if (optStdout)
		throw DZException("Sort mode cannot be used with stdout");
	if (IsWebFile(optInput[0]))
		throw DZException("Web locations are currently not supported for sorting");
    if (output == "")
		output = optInput[0] + ".sort";
	DEBUG("Sorting %s to %s witn %'lu memory...", optInput[0].c_str(), output.c_str(), optSortMemory);
    sortFile(optInput[0], output, optSortMemory);
    return output;
}

void compress (const vector<string> &in, const string &out) {
	for (int i = 0; i < in.size(); i++) {
	//	if (IsWebFile(in[i]))
	//		throw DZException("Web locations are currently not supported for compression");
		if (is_dz_file(in[i]))
			throw DZException("Cannot compress DeeZ file %s", in[i].c_str());
	}
	if (IsWebFile(out))
		throw DZException("Web locations are not supported as output");
	if (FileExists(out.c_str())) {
		if (!optForce)
			throw DZException("File %s already exists. Use -! to overwrite", out.c_str());
		else
			WARN("File %s already exists. Overwriting it.", out.c_str());
	}
	try {
		FileCompressor sc(out, in, optRef, optBlock);
		sc.compress();
	}
	catch (DZSortedException &s) {
		if (optForce) {
			optInput[0] = sort("");
			FileCompressor sc(out, optInput, optRef, optBlock);
			sc.compress();
		}
		else throw;
	}
}

void decompress (const vector<string> &in, const string &out) {
	if (in.size() > 2)
		throw DZException("Only one file can be decompressed per invocation.");
	if (!is_dz_file(in[0]))
		throw DZException("File %s is not DZ file", in[0].c_str());
	if (optStats) {
		FileDecompressor::printStats(in[0], optFlag);
		return;
	}

	if (IsWebFile(out))
		throw DZException("Web locations are not supported as output");
	if (!optStdout && FileExists(out.c_str())) {
		if (!optForce)
			throw DZException("File %s already exists. Use -! to overwrite", out.c_str());
		else
			WARN("File %s already exists. Overwriting it.", out.c_str());
	}
	if (!optStdout) 
		DEBUG("Using output file %s", out.c_str());
	LOG("Decompressing %s to %s ...", in[0].c_str(), optStdout ? "stdout" : out.c_str());
	FileDecompressor sd(in[0], out, optRef, optBlock);
	
	if (in.size() <= 1)
		sd.decompress(optFlag);
	else //if (optQuery == "")
		sd.decompress(in[1], optFlag);
	//else 
	//	sd.query(optQuery, in[1]);
}

void test (vector<string> s) {
	string tmp = s[0] + ".dztemp";
	//LOG("Test prefix %s", tmp.c_str());

	int64_t t = dz_time();
	compress(s, tmp + ".dz");
	WARN("Compression time: %'lld", dz_time() - t);

	t = dz_time();

	vector<string> tx;
	tx.push_back(tmp + ".dz");
	decompress(tx, tmp + ".sam");
	WARN("Decompression time: %'lld", dz_time() - t);

	string cmd = "cmp " + s[0] + " " + tmp + ".sam";
	system(cmd.c_str());
}

#ifndef DEEZLIB
int main (int argc, char **argv) {
	curl_global_init(CURL_GLOBAL_ALL);
    setlocale(LC_ALL, "");
    parse_opt(argc, argv);

	#ifdef VER
    LOG("DeeZ 0x%x (%s)", VERSION, VER);
	#endif

    try {
    	if (!optInput.size())
    		throw DZException("Input not specified. Please run deez --help for explanation");
    	for (int i = 0; i < optInput.size(); i++) if (!FileExists(optInput[i].c_str()) && i != optInput.size() - 1)
			throw DZException("File %s does not exist", optInput[i].c_str());
		
    	if (optSort)
    		sort(optOutput);
    	else if (optStats)
    		decompress(optInput, "");
    	else if (optTest)
            test(optInput);
        else {
	    //	if (!optInvalidChr && !FileExists(optRef.c_str()) && (optRef.size() == 0 || optRef[optRef.size() - 1] != '/'))
	    //		throw DZException("Reference file %s does not exist", optRef.c_str());
	    //	else if (optInvalidChr) LOG("No reference provided -- compression performance will be suboptimal");
	    //	else DEBUG("Using reference file %s", full_path(optRef).c_str());
		//	DEBUG("Using block size %'lu", optBlock);

            bool isCompress = !is_dz_file(optInput[0]);
            string output = optOutput;
            if (output == "" && !optStdout) {
                output = remove_extension(optInput[0]) + ".dz";
                if (!isCompress) output += ".sam";
            }
            WARN("%s",output.c_str());
            if (isCompress)
                compress(optInput, output);
            else
                decompress(optInput, output);
		}
	}
	catch (DZException &e) {
		ERROR("\nDeeZ error: %s!", e.what());
		exit(1);
	}
	catch (...) {
		ERROR("\nUnknown error ocurred!");	
		exit(1);
	}
	
	return 0;
}
#endif