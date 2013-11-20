#include "Compress.h"
using namespace std;

#include <sys/time.h>
#include <time.h>
double _zaman_ (void) {
	struct timeval t;
	gettimeofday(&t, 0);
	return (t.tv_sec * 1000000ll + t.tv_usec) / 1000000.0;
}
static const char *NAMES[8] = {
	"SQ","RN","MF","ML", 
	"MQ","QQ","PE","OF" 
};

FileCompressor::FileCompressor (const string &outFile, const string &samFile, const string &genomeFile, int bs): 
	blockSize(bs) 
{
//	parser
	FILE *fi = fopen(samFile.c_str(), "rb");
	char mc[2];
	fread(mc, 1, 2, fi);
	fclose(fi);
	if (mc[0] == char(0x1f) && mc[1] == char(0x8b)) {
		LOG("Using BAM file");
		parser = new BAMParser(samFile);
	}
	else
		parser = new SAMParser(samFile);

	compressor[Sequence] = new SequenceCompressor(genomeFile, bs);
	compressor[ReadName] = new ReadNameCompressor(bs);
	compressor[MappingFlag] = new MappingFlagCompressor(bs);
	compressor[MappingLocation] = new MappingLocationCompressor(bs);
	compressor[MappingQuality] = new MappingQualityCompressor(bs);
	compressor[QualityScore] = new QualityScoreCompressor(bs);
	compressor[PairedEnd] = new PairedEndCompressor(bs);
	compressor[OptionalField] = new OptionalFieldCompressor(bs);

	outputFile = fopen(outFile.c_str(), "wb");
	if (outputFile == NULL)
		throw DZException("Cannot open the file %s", outFile.c_str());

	string idxFile = outFile + "i";
	indexFile = gzopen(idxFile.c_str(), "wb6");
	if (indexFile == NULL)
		throw DZException("Cannot open the file %s", idxFile.c_str());
}

FileCompressor::~FileCompressor (void) {
	for (int i = 0; i < 8; i++)
		delete compressor[i];
	delete parser;
	fclose(outputFile);
	gzclose(indexFile);
}

void FileCompressor::compress (void) {
	outputMagic();
	outputComment();
	outputRecords();
}

void FileCompressor::outputMagic (void) {
	uint32_t magic = MAGIC;
	fwrite(&magic, 4, 1, outputFile);
}

void FileCompressor::outputComment (void) {
	string comment = parser->readComment();
	size_t arcsz = comment.size();
	fwrite(&arcsz, sizeof(size_t), 1, outputFile);
	if (arcsz) {
		GzipCompressionStream<6> gzc;
		Array<uint8_t> arc;
		gzc.compress((uint8_t*)comment.c_str(), comment.size(), arc, 0);
		arcsz = arc.size();
		fwrite(&arcsz, sizeof(size_t), 1, outputFile);
		fwrite(arc.data(), 1, arcsz, outputFile);
	}
}

void FileCompressor::outputRecords (void) {
	// SAM file
	size_t lastStart;
	int64_t total = 0;
	int64_t bsc = blockSize;
	int64_t blockCount = 0;
	Array<uint8_t> outputBuffer(0, MB);

	SequenceCompressor* seq = (SequenceCompressor*)(compressor[Sequence]);
	
	while (parser->hasNext()) {
		char op = 0;
		string chr = parser->head();
		while (seq->getChromosome() != parser->head())
			seq->scanChromosome(parser->head()), op = 1;

		//LOG("Loading records ...");
		for (; seq->size() < blockSize
							&& parser->hasNext() 
							&& parser->head() == seq->getChromosome(); total++) {
			const Record &rc = parser->next();
			//rc.testRecords();

			size_t loc = rc.getLocation();
			if (loc == (size_t)-1) loc = 0; // fix for 0 locations in *

			seq->addRecord(
				loc, 
				rc.getSequence(), 
				rc.getCigar()
			);
			((ReadNameCompressor*)compressor[ReadName])->addRecord(rc.getReadName());
			((MappingFlagCompressor*)compressor[MappingFlag])->addRecord(rc.getMappingFlag());
			((MappingLocationCompressor*)compressor[MappingLocation])->addRecord(loc);
			((MappingQualityCompressor*)compressor[MappingQuality])->addRecord(rc.getMappingQuality());
			((QualityScoreCompressor*)compressor[QualityScore])->addRecord(rc.getQuality()/*, rc.getSequence()*/, rc.getMappingFlag());
			
			size_t p_loc = rc.getPairLocation();
			if (p_loc == (size_t)-1) p_loc = 0;
			((PairedEndCompressor*)compressor[PairedEnd])->addRecord(PairedEndInfo(
				rc.getPairChromosome(),
				p_loc, 
				rc.getTemplateLenght()
			));
			((OptionalFieldCompressor*)compressor[OptionalField])->addRecord(rc.getOptional());

			lastStart = loc;
			parser->readNext();		
		}
		SCREEN("\r   Chr %-6s %5.2lf%% [%ld]", seq->getChromosome().c_str(), (100.0 * parser->fpos()) / parser->fsize(), blockCount + 1);		

		size_t 	currentBlockCount, 
				currentBlockFirstLoc, 
				currentBlockLastLoc, 
				currentBlockLastEndLoc,
				fixedStartPos, fixedEndPos;
	//	ZAMAN_START();
		currentBlockCount = seq->applyFixes(!parser->hasNext() 
							|| parser->head() != seq->getChromosome() 
							|| parser->head() == "*" 
						? (size_t)-1 : lastStart - 1, 
						currentBlockFirstLoc, currentBlockLastLoc, currentBlockLastEndLoc,
						fixedStartPos, fixedEndPos);	
		if (currentBlockCount == 0) {
			blockSize += bsc;
			continue;
		}
		ERROR("Block size %lld...", blockSize);

		// write chromosome id
		fwrite(&op, sizeof(char), 1, outputFile);
		if (op) 
			fwrite(chr.c_str(), chr.size() + 1, 1, outputFile);

		size_t zpos = ftell(outputFile);
		gzwrite(indexFile, &zpos, sizeof(size_t));		
	//	DEBUG("\n%s:%'lu-%'lu..%'lu\tfx %'lu-%'lu\t%'lu", seq->getChromosome().c_str(), 
	//		currentBlockFirstLoc+1, currentBlockLastLoc+1, currentBlockLastEndLoc, 
	//		fixedStartPos, fixedEndPos, currentBlockCount);
		// cnt
		gzwrite(indexFile, &currentBlockCount, sizeof(size_t));
		// chr		
		gzwrite(indexFile, seq->getChromosome().c_str(), seq->getChromosome().size() + 1);
		// start pos		
		gzwrite(indexFile, &currentBlockFirstLoc, sizeof(size_t));
		// end pos
		gzwrite(indexFile, &currentBlockLastLoc, sizeof(size_t));
		// end cover pos
		// gzwrite(indexFile, &currentBlockLastEndLoc, sizeof(size_t));
		// fixed start pos
		gzwrite(indexFile, &fixedStartPos, sizeof(size_t));
		// fixed end pos
		gzwrite(indexFile, &fixedEndPos, sizeof(size_t));
	//	ZAMAN_END("FIX");
		for (int i = 0; i < 8; i++) {
	//	ZAMAN_START();
			compressor[i]->outputRecords(outputBuffer, 0, currentBlockCount);
			
			// dz file
			size_t out_sz = outputBuffer.size();
			fwrite(&out_sz, sizeof(size_t), 1, outputFile);
			if (out_sz) 
				fwrite(outputBuffer.data(), 1, out_sz, outputFile);
			compressor[i]->compressedCount += out_sz + sizeof(size_t);

			// index file
			compressor[i]->getIndexData(outputBuffer);
			out_sz = outputBuffer.size();
			gzwrite(indexFile, &out_sz, sizeof(size_t));
			if (out_sz) 
				gzwrite(indexFile, outputBuffer.data(), out_sz);
	//	ZAMAN_END(NAMES[i]);
		}
	//	DEBUGN("\n");
		blockCount++;
		fflush(stdout);
	}
	SCREEN("\nWritten %'lu lines\n", total);
	for (int i = 0; i < 8; i++)
		DEBUG("%s: %'15lu", NAMES[i], compressor[i]->compressedCount);
}

