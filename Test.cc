#include <string>
#include <stdio.h>
#include <locale.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/time.h>

#include "Common.h"
#include "Parsers/BAMParser.h"
#include "Parsers/SAMParser.h"
#include "Fields/Sequence.h"
#include "Fields/ReadName.h"
#include "Fields/MappingFlag.h"
#include "Fields/EditOperation.h"
#include "Fields/MappingQuality.h"
#include "Fields/QualityScore.h"
#include "Fields/PairedEnd.h"
#include "Fields/OptionalField.h"
using namespace std;

const size_t optBlock = 1000000;
bool optStdout  = false;
static const char *NAMES[8] = {
	"SQ","RN","MF","ML", 
	"MQ","QQ","PE","OF" 
};

int main (int argc, char **argv) {
	SAMParser *parser = new SAMParser(argv[1]);
	QualityScoreCompressor *sq = new QualityScoreCompressor(optBlock);
	
	string comment = parser->readComment();

	int64_t blockCount = 0;
	Array<uint8_t> outputBuffer(0, MB);
	while (parser->hasNext()) {
		string chr = parser->head();
		size_t _i;
		for (_i = 0; _i < optBlock
							&& parser->hasNext() 
							&& parser->head() == chr; _i++) {
			const Record &rc = parser->next();
			sq->addRecord(rc.getQuality()/*, rc.getSequence()*/, rc.getMappingFlag());
			parser->readNext();		
		}
		LOGN("\r   Chr %-6s %5.2lf%% [%ld]", chr.c_str(), (100.0 * parser->fpos()) / parser->fsize(), blockCount + 1);
		sq->outputRecords(outputBuffer, 0, _i);			
		// dz file
		size_t out_sz = outputBuffer.size();
		fwrite(&out_sz, sizeof(size_t), 1, stdout);
		if (out_sz) 
			fwrite(outputBuffer.data(), 1, out_sz, stdout);
		blockCount++;
	}
	LOGN("\n");
	LOGN("QUAL: %'15lu\n", sq->compressedSize());

	delete sq;
	delete parser;

	return 0;
}
