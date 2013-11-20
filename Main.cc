#include <string>
#include <stdio.h>
#include <locale.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/time.h>

#include "Common.h"
#include "Compress.h"
#include "Decompress.h"
using namespace std;

bool optTest 	= false;
bool optForce 	= false;
bool optStdout  = false;
string optRef 	= "";
string optInput = "";
string optRange = "";
size_t optBlock = 1000000;

void parse_opt (int argc, char **argv) {
	int opt; 
	struct option long_opt[] = {
		{ "help",      0, NULL, 'h' },
		{ "reference", 1, NULL, 'r' },
		{ "force",     0, NULL, 'f' },
		{ "test",      0, NULL, 't' },
		{ "stdout",    0, NULL, 's' },
		{ NULL, 0, NULL, 0 }
	};
	const char *short_opt = "hr:tfs";
	do {
		opt = getopt_long (argc, argv, short_opt, long_opt, NULL);
		switch (opt) {
			case 'h':
				exit(0);
			case 'r':
				optRef = optarg;
				break;
			case 't':
				optTest = true;
				break;
			case 'f':
				optForce = true;
				break;
			case 's':
				optStdout = true;
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
			WARN("File %s already exists. Overwriting it.", out.c_str());
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
			WARN("File %s already exists. Overwriting it.", out.c_str());
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
	/*char *t = new char[s.size() + 1];
	memcpy(t, s.c_str(), s.size() + 1);
	string dir(dirname(t));
	memcpy(t, s.c_str(), s.size() + 1);
	string file(basename(t)); 
	delete[] t;

	file += ".dztmp_";
	string tmp = string(tempnam(dir.c_str(), file.c_str()));
	WARN(">>>>> %s %s %s\n",dir.c_str(),file.c_str(),tmp.c_str());
	exit(1);*/

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
	    if (!file_exists(optRef))
	    	throw DZException("Reference file %s does not exist", optRef.c_str());
	    DEBUG("Using reference file %s", full_path(optRef).c_str());
		DEBUG("Using block size %'lu", optBlock);

		if (!file_exists(optInput))
			throw DZException("File %s does not exist", optInput.c_str());
		DEBUG("Using input file %s", full_path(optInput).c_str());

		if (optTest)
			test(optInput);
		else {
			string output = remove_extension(optInput);	
			if (!is_dz_file(optInput))
				compress(optInput, output + ".dz");
			else 
				decompress(optInput, output + ".dz.sam");
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
