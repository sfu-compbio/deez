#include <thread>
#include "Compress.h"
using namespace std;

#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
double _zaman_ (void) {
	struct timeval t;
	gettimeofday(&t, 0);
	return (t.tv_sec * 1000000ll + t.tv_usec) / 1000000.0;
}
static const char *NAMES[8] = {
	"SQ","EO","RN","MF",
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
		//LOG("Using BAM file");
		parser = new BAMParser(samFile);
	}
	else
		parser = new SAMParser(samFile);

	sequence = new SequenceCompressor(genomeFile, bs);
	editOp = new EditOperationCompressor(bs);
	readName = new ReadNameCompressor(bs);
	mapFlag = new MappingFlagCompressor(bs);
	mapQual = new MappingQualityCompressor(bs);
	quality = new QualityScoreCompressor(bs);
	pairedEnd = new PairedEndCompressor(bs);
	optField = new OptionalFieldCompressor(bs);

	if (outFile == "")
		outputFile = stdout;
	else {
		outputFile = fopen(outFile.c_str(), "wb");
		if (outputFile == NULL)
			throw DZException("Cannot open the file %s", outFile.c_str());
	}

	string idxFile = outFile + "idx";
//	indexFile = gzopen(idxFile.c_str(), "wb6");

	FILE *tmp = tmpfile();
	
	// char filePath[1024] = {0};
	// char _x[200]; sprintf(_x,"/proc/self/fd/%d",fileno(tmp));
	// readlink(_x, filePath, 1024);
	// LOG("Index file %s", filePath);
	
	int tmpFile = dup(fileno(tmp));
	indexTmp = fdopen(tmpFile, "rb");

	indexFile = gzdopen(fileno(tmp), "wb6");
	if (indexFile == Z_NULL)	
		throw DZException("Cannot open temporary file");
	//gzwrite(indexFile, "HAMO", 5);
}

FileCompressor::~FileCompressor (void) {
	delete sequence;
	delete editOp;
	delete readName;
	delete mapFlag;
	delete mapQual;
	delete quality;
	delete pairedEnd;
	delete optField;
	delete parser;
	fclose(outputFile);
}

void FileCompressor::compress (void) {
	outputMagic();
	outputComment();
	outputRecords();
}

void FileCompressor::outputMagic (void) {
	uint32_t magic = MAGIC;
	fwrite(&magic, 4, 1, outputFile);
	fwrite(&optQuality, 1, 1, outputFile);
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

void FileCompressor::compressBlock (Compressor *c, Array<uint8_t> &out, Array<uint8_t> &idxOut, size_t count) {
	ZAMAN_START();
	out.resize(0);
	c->outputRecords(out, 0, count);
	idxOut.resize(0);
	c->getIndexData(idxOut);
	ZAMAN_END(NAMES[int64_t(c->debugStream)]);
}

void FileCompressor::outputBlock (Array<uint8_t> &out, Array<uint8_t> &idxOut) {
	size_t out_sz = out.size();
	fwrite(&out_sz, sizeof(size_t), 1, outputFile);
	if (out_sz) 
		fwrite(out.data(), 1, out_sz, outputFile);
	
	out_sz = idxOut.size();
	//LOG("---%d", idxOut.size());
	gzwrite(indexFile, &out_sz, sizeof(size_t));
	if (out_sz) 
		gzwrite(indexFile, idxOut.data(), out_sz);
}

void FileCompressor::outputRecords (void) {
	Stats stats;

	size_t lastStart = 0;
	int64_t total = 0;
	int64_t bsc = blockSize;

	int64_t blockCount = 0;
	int64_t currentSize = 0;
	Array<uint8_t> outputBuffer[8];
	Array<uint8_t> idxBuffer[8];
	for (int i = 0; i < 8; i++) {
		outputBuffer[i].set_extend(MB);
		idxBuffer[i].set_extend(MB);
	}

	size_t prev_loc = 0;
	while (parser->hasNext()) {
		char op = 0;
		string chr = parser->head();
		while (sequence->getChromosome() != parser->head()) 
			sequence->scanChromosome(parser->head()), op = 1, prev_loc = 0;
		if (op)
			stats.addChromosome(sequence->getChromosome(), sequence->getChromosomeLength());

		ZAMAN_START();
		for (; currentSize < blockSize && parser->hasNext() 
				&& parser->head() == sequence->getChromosome(); currentSize++) 
		{
			const Record &rc = parser->next();
			// rc.testRecords();

			// fix for 0 locations in *
			size_t loc = rc.getLocation();
			if (loc == (size_t)-1) loc = 0; 
			size_t p_loc = rc.getPairLocation();
			if (p_loc == (size_t)-1) p_loc = 0;

			if (loc < prev_loc)
				throw DZSortedException("Input is not sorted. Please sort it with 'dz --sort' before compressing it");
			prev_loc = loc;

			EditOperation eo(loc, rc.getSequence(), rc.getCigar());
			sequence->updateBoundary(eo.end);
			editOp->addRecord(eo);
			readName->addRecord(rc.getReadName());
			mapFlag->addRecord(rc.getMappingFlag());
			mapQual->addRecord(rc.getMappingQuality());
			quality->addRecord(rc.getQuality()/*, rc.getSequence()*/, rc.getMappingFlag());
			pairedEnd->addRecord(PairedEndInfo(rc.getPairChromosome(), p_loc, rc.getTemplateLenght(), 
				sequence->getChromosome(), rc.getLocation())); //, strlen(rc.getSequence())));
			optField->addRecord(rc.getOptional());

			stats.addRecord(rc.getMappingFlag());

			lastStart = loc;
			parser->readNext();		
		}
		total += currentSize;
		LOGN("\r   Chr %-6s %5.2lf%% [%ld]", sequence->getChromosome().c_str(), (100.0 * parser->fpos()) / parser->fsize(), blockCount + 1);		

		//LOGN("\n# ");
		ZAMAN_END("REQ");
		size_t currentBlockCount, 
			currentBlockFirstLoc, 
			currentBlockLastLoc, 
			currentBlockLastEndLoc,
			fixedStartPos, fixedEndPos;
		size_t zpos;
		ZAMAN_START();
		currentBlockCount = sequence->applyFixes(
		 	!parser->hasNext() || parser->head() != sequence->getChromosome() || parser->head() == "*" 
		 		? (size_t)-1 : lastStart - 1, 
		 	*editOp,
		 	currentBlockFirstLoc, currentBlockLastLoc, currentBlockLastEndLoc,
		 	fixedStartPos, fixedEndPos
		);
		if (currentBlockCount == 0) {
			blockSize += bsc;
			LOG("Retrying...");
			continue;
		}
		currentSize -= currentBlockCount;

		// ERROR("Block size %lld...", blockSize);
		// write chromosome id
		//LOG("> %lu",ftell(outputFile));
		zpos = ftell(outputFile);

		fwrite(&op, sizeof(char), 1, outputFile);
		if (op) 
			fwrite(chr.c_str(), chr.size() + 1, 1, outputFile);

		gzwrite(indexFile, &zpos, sizeof(size_t));		
		//	DEBUG("\n%s:%'lu-%'lu..%'lu\tfx %'lu-%'lu\t%'lu", sequence->getChromosome().c_str(), 
		//		currentBlockFirstLoc+1, currentBlockLastLoc+1, currentBlockLastEndLoc, 
		//		fixedStartPos, fixedEndPos, currentBlockCount);
		// cnt
		gzwrite(indexFile, &currentBlockCount, sizeof(size_t));
		// chr		
		gzwrite(indexFile, sequence->getChromosome().c_str(), sequence->getChromosome().size() + 1);
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
		ZAMAN_END("FIX");

		//LOG("%d %d %s %d %d %d %d",zpos, currentBlockCount,sequence->getChromosome().c_str(),
		//	currentBlockFirstLoc,currentBlockLastLoc,fixedStartPos,fixedEndPos);

		ZAMAN_START();
		Compressor *ci[] = { sequence, editOp, readName, mapFlag, mapQual, quality, pairedEnd, optField };
		thread t[8];
		for (size_t ti = 0; ti < 8; ti++) {
			ci[ti]->debugStream = (FILE*)ti;
			t[ti] = thread(compressBlock, ci[ti], ref(outputBuffer[ti]), ref(idxBuffer[ti]), currentBlockCount);
		}
		for (int ti = 0; ti < 8; ti++)
			t[ti].join();
		ZAMAN_END("CALC");

		ZAMAN_START();
		for (int ti = 0; ti < 8; ti++)
			outputBlock(outputBuffer[ti], idxBuffer[ti]);
		//LOG("ZP %'lu ", zpos);
		ZAMAN_END("IO");

		//fflush(outputFile);
		
		//LOG("");
		blockCount++;
	}
	LOGN("\nWritten %'lu lines\n", total);
	fflush(outputFile);
	
	size_t posStats = ftell(outputFile);
	fwrite("DZSTATS", 1, 7, outputFile);
	stats.writeStats(outputBuffer[0]);
	size_t sz = outputBuffer->size();
	fwrite(&sz, 8, 1, outputFile);
	fwrite(outputBuffer->data(), 1, outputBuffer->size(), outputFile);
	
	//gzclose(indexFile);
	//LOG("gzstatus %d", gzclose(indexFile));
	fwrite("DZIDX", 1, 5, outputFile);
	char *buffer = (char*)malloc(MB);

	fseek(indexTmp, 0, SEEK_END);
	//LOG("Index gz'd sz=%'lu", ftell(indexTmp));
	fseek(indexTmp, 0, SEEK_SET);
	while (sz = fread(buffer, 1, MB, indexTmp))
		fwrite(buffer, 1, sz, outputFile);
	free(buffer);
	fclose(indexTmp);
	
	fwrite(&posStats, sizeof(size_t), 1, outputFile);
	
	#define VERBOSE(x) // LOG("%s: %lu", #x, x->compressedSize())
	VERBOSE(sequence);
	VERBOSE(editOp);
	VERBOSE(readName);
	VERBOSE(mapFlag);
	VERBOSE(mapQual);
	VERBOSE(quality);
	VERBOSE(pairedEnd);
	VERBOSE(optField);
}

