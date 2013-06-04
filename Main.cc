#include <string>
#include <stdio.h>
#include <locale.h>

#include "Common.h"
#include "SAMFile.h"
using namespace std;

#include "Streams/GzipStream.h"

int main (int argc, char **argv) {
    setlocale(LC_ALL, "");

	if (argc == 1) {
		fprintf(stderr, "dz c reference input.sam  output\n");
		fprintf(stderr, "dz d reference input  output.sam\n");
		return 0;
	}

	int blockSize = 1000000;
	string referenceF(argv[1]);
	string inputF(argv[2]);
	string outputF(argv[3]);
	string samOutF(argv[4]);

	SAMFileCompressor *sc = new SAMFileCompressor(outputF, inputF, referenceF, blockSize);
	sc->compress();
	delete sc;

	SAMFileDecompressor *sd = new SAMFileDecompressor(outputF, samOutF, referenceF, blockSize);
	sd->decompress();
	delete sd;

//	Logger::initialize(stdout);
/*	int blockSize = 1000000;

	if (argc == 5 && argv[1][0] == 'c') {
		string referenceF(argv[2]);
		string inputF(argv[3]);
		string outputF(argv[4]);

		SAMFileCompressor sc(outputF, inputF, referenceF, blockSize);
		sc.compress();
	}

	if (argc == 5 && argv[1][0] == 'd') {
		string reference(argv[2]);
		string inputF(argv[3]);
		string outputF(argv[4]);

		SAMFileDecompressor sd(inputF, outputF, reference, blockSize);
		sd.decompress();
	}
*/

	return 0;
}
