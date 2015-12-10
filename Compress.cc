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

FileCompressor::FileCompressor (const string &outFile, const vector<string> &samFiles, const string &genomeFile, int bs): 
	blockSize(bs) 
{
	for (int f = 0; f < samFiles.size(); f++) {
		File *fi = OpenFile(samFiles[f].c_str(), "rb");
		char mc[2];
		fi->read(mc, 2);
		delete fi;
		if (mc[0] == char(0x1f) && mc[1] == char(0x8b)) 
			parsers.push_back(new BAMParser(samFiles[f]));
		else
			parsers.push_back(new SAMParser(samFiles[f]));
		sequence.push_back(new SequenceCompressor(genomeFile, bs));
		editOp.push_back(new EditOperationCompressor(bs));
		if (optReadLossy)
			readName.push_back(new ReadNameLossyCompressor(bs));
		else 
			readName.push_back(new ReadNameCompressor(bs));
		mapFlag.push_back(new MappingFlagCompressor(bs));
		mapQual.push_back(new MappingQualityCompressor(bs));
		quality.push_back(new QualityScoreCompressor(bs));
		pairedEnd.push_back(new PairedEndCompressor(bs));
		optField.push_back(new OptionalFieldCompressor(bs));
	}
	if (outFile == "")
		outputFile = stdout;
	else {
		outputFile = fopen(outFile.c_str(), "wb");
		if (outputFile == NULL)
			throw DZException("Cannot open the file %s", outFile.c_str());
	}

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
}

FileCompressor::~FileCompressor (void) {
	for (int f = 0; f < parsers.size(); f++) {
		delete parsers[f];
		delete sequence[f];
		delete editOp[f];
		delete readName[f];
		delete mapFlag[f];
		delete mapQual[f];
		delete quality[f];
		delete pairedEnd[f];
		delete optField[f];
	}
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

// BEGIN_V2
	uint16_t numFiles = parsers.size();
	fwrite(&numFiles, 2, 1, outputFile);

	string files;
	for (int f = 0; f < parsers.size(); f++)
		files += parsers[f]->fileName() + "\0";

	size_t arcsz = files.size();
	fwrite(&arcsz, sizeof(size_t), 1, outputFile);
	GzipCompressionStream<6> gzc;
	Array<uint8_t> arc;
	gzc.compress((uint8_t*)files.c_str(), files.size(), arc, 0);
	arcsz = arc.size();
	fwrite(&arcsz, sizeof(size_t), 1, outputFile);
	fwrite(arc.data(), 1, arcsz, outputFile);
// END_V2
}

void FileCompressor::outputComment (void) {
	for (int f = 0; f < parsers.size(); f++) {
		string comment = parsers[f]->readComment();
		sequence[f]->scanSAMComment(comment);
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
	vector<Stats> stats(parsers.size());
	for (int16_t f = 0; f < parsers.size(); f++) 
		stats[f].fileName = parsers[f]->fileName();

	vector<size_t> lastStart(parsers.size(), 0);
	vector<int64_t> bsc(parsers.size(), blockSize);
	int64_t total = 0;
	//int64_t bsc = blockSize;

	int64_t blockCount = 0;
	vector<int64_t> currentSize(parsers.size(), 0);
	Array<uint8_t> outputBuffer[8];
	Array<uint8_t> idxBuffer[8];
	for (int i = 0; i < 8; i++) {
		outputBuffer[i].set_extend(MB);
		idxBuffer[i].set_extend(MB);
	}

	vector<size_t> prev_loc(parsers.size(), 0);

	int fileAliveCount = parsers.size();
	while (fileAliveCount) { 
		//ZAMAN_START();
		LOGN("\r");
		for (int16_t f = 0; f < parsers.size(); f++) {
			if (!parsers[f]->hasNext()) {
				fileAliveCount--;
				continue;
			}
			char op = 0;
			string chr = parsers[f]->head();
			while (sequence[f]->getChromosome() != parsers[f]->head()) 
				sequence[f]->scanChromosome(parsers[f]->head()), op = 1, prev_loc[f] = 0;
			//if (op)
			//	stats[f].addChromosome(sequence[f]->getChromosome(), sequence[f]->getChromosomeLength());

			for (; currentSize[f] < bsc[f] && parsers[f]->hasNext() 
					&& parsers[f]->head() == sequence[f]->getChromosome(); currentSize[f]++) 
			{
				const Record &rc = parsers[f]->next();
				
				size_t loc = rc.getLocation();
				if (loc == (size_t)-1) loc = 0; 
				size_t p_loc = rc.getPairLocation();
				if (p_loc == (size_t)-1) p_loc = 0;

				if (loc < prev_loc[f])
					throw DZSortedException("%s is not sorted. Please sort it with 'dz --sort' before compressing it", parsers[f]->fileName().c_str());
				prev_loc[f] = loc;

				EditOperation eo(loc, rc.getSequence(), rc.getCigar());
				sequence[f]->updateBoundary(eo.end);
				editOp[f]->addRecord(eo);
				if (optReadLossy)
					((ReadNameLossyCompressor*)readName[f])->addRecord(rc.getReadName());
				else
					((ReadNameCompressor*)readName[f])->addRecord(rc.getReadName());
				mapFlag[f]->addRecord(rc.getMappingFlag());
				mapQual[f]->addRecord(rc.getMappingQuality());
				quality[f]->addRecord(rc.getQuality()/*, rc.getSequence()*/, rc.getMappingFlag());
				pairedEnd[f]->addRecord(PairedEndInfo(rc.getPairChromosome(), p_loc, rc.getTemplateLenght(), 
					eo.start, eo.end - eo.start, rc.getMappingFlag() & 0x10));
				optField[f]->addRecord(rc.getOptional());

				stats[f].addRecord(rc.getMappingFlag());

				lastStart[f] = loc;
				parsers[f]->readNext();		
			}
			total += currentSize[f];
			LOGN("  %5.1lf%% [%s,%zd]", (100.0 * parsers[f]->fpos()) / parsers[f]->fsize(), 
				sequence[f]->getChromosome().c_str(), blockCount + 1);

			//LOGN("\n# ");
			//ZAMAN_END("REQ");
			size_t currentBlockCount, 
				currentBlockFirstLoc, 
				currentBlockLastLoc, 
				currentBlockLastEndLoc,
				fixedStartPos, fixedEndPos;
			size_t zpos;
			//ZAMAN_START();
			currentBlockCount = sequence[f]->applyFixes(
			 	!parsers[f]->hasNext() || parsers[f]->head() != sequence[f]->getChromosome() || parsers[f]->head() == "*" 
			 		? (size_t)-1 : lastStart[f] - 1, 
			 	*(editOp[f]),
			 	currentBlockFirstLoc, currentBlockLastLoc, currentBlockLastEndLoc,
			 	fixedStartPos, fixedEndPos
			);
			if (currentBlockCount == 0) {
				bsc[f] += blockSize;
				LOG("Retrying...");
				continue;
			}
			currentSize[f] -= currentBlockCount;

			// ERROR("Block size %lld...", blockSize);
			// write chromosome id
			//LOG("> %lu",ftell(outputFile));
			zpos = ftell(outputFile);

			fwrite(&op, sizeof(char), 1, outputFile);
			if (op) 
				fwrite(chr.c_str(), chr.size() + 1, 1, outputFile);

// BEGIN V2
			gzwrite(indexFile, &f, sizeof(int16_t));
// END V2
			gzwrite(indexFile, &zpos, sizeof(size_t));		
			//	DEBUG("\n%s:%'lu-%'lu..%'lu\tfx %'lu-%'lu\t%'lu", sequence->getChromosome().c_str(), 
			//		currentBlockFirstLoc+1, currentBlockLastLoc+1, currentBlockLastEndLoc, 
			//		fixedStartPos, fixedEndPos, currentBlockCount);
			// cnt
			gzwrite(indexFile, &currentBlockCount, sizeof(size_t));
			// chr		
			gzwrite(indexFile, sequence[f]->getChromosome().c_str(), sequence[f]->getChromosome().size() + 1);
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
			//ZAMAN_END("FIX");

			//LOG("%d %d %s %d %d %d %d",zpos, currentBlockCount,sequence->getChromosome().c_str(),
			//	currentBlockFirstLoc,currentBlockLastLoc,fixedStartPos,fixedEndPos);

			//ZAMAN_START();
			Compressor *ci[] = { sequence[f], editOp[f], readName[f], mapFlag[f], mapQual[f], quality[f], pairedEnd[f], optField[f] };
			thread t[8];
			for (size_t ti = 0; ti < 8; ti++) {
				ci[ti]->debugStream = (FILE*)ti;
				t[ti] = thread(compressBlock, ci[ti], ref(outputBuffer[ti]), ref(idxBuffer[ti]), currentBlockCount);
			}
			for (int ti = 0; ti < 8; ti++)
				t[ti].join();
			//ZAMAN_END("CALC");

			//ZAMAN_START();
			for (int ti = 0; ti < 8; ti++)
				outputBlock(outputBuffer[ti], idxBuffer[ti]);
			//LOG("ZP %'lu ", zpos);
			//ZAMAN_END("IO");

			//fflush(outputFile);
			
			//LOG("");
			blockCount++;
		}
	}
	LOGN("\nWritten %'zd lines\n", total);
	fflush(outputFile);
	
	size_t posStats = ftell(outputFile);
	fwrite("DZSTATS", 1, 7, outputFile);
	for (int i = 0; i < stats.size(); i++) {
		stats[i].writeStats(outputBuffer[0], sequence[i]);
		size_t sz = outputBuffer->size();
		fwrite(&sz, 8, 1, outputFile);
		fwrite(outputBuffer->data(), 1, outputBuffer->size(), outputFile);
	}
	
	//gzclose(indexFile);
	int gzst = gzclose(indexFile);
	DEBUG("gzstatus %d", gzst);
	//gzclose(indexFile);
	fwrite("DZIDX", 1, 5, outputFile);
	char *buffer = (char*)malloc(MB);

	//indexTmp = fopen((xoutfile + ".dzi").c_str(), "rb");
	//fseek(indexTmp, 0, SEEK_END);
	//LOG("Index gz'd sz=%'lu", ftell(indexTmp));
	fseek(indexTmp, 0, SEEK_SET);
	while (size_t sz = fread(buffer, 1, MB, indexTmp))
		fwrite(buffer, 1, sz, outputFile);
	free(buffer);
	fclose(indexTmp);
	
	fwrite(&posStats, sizeof(size_t), 1, outputFile);
	
	#define VERBOSE(x) \
		LOG("%-12s: %'20lu", #x, x->compressedSize()); x->printDetails();
	for (int f = 0; f < parsers.size(); f++) {
		VERBOSE(sequence[f]);
		VERBOSE(editOp[f]);
		VERBOSE(readName[f]);
		VERBOSE(mapFlag[f]);
		VERBOSE(mapQual[f]);
		VERBOSE(quality[f]);
		VERBOSE(pairedEnd[f]);
		VERBOSE(optField[f]);
	}
}

