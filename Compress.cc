#include <thread>
#include "Compress.h"
using namespace std;

#include <fcntl.h>
#include <sys/time.h>
#include <time.h>

FileCompressor::FileCompressor (const string &outFile, const vector<string> &samFiles, const string &genomeFile, int bs): 
	blockSize(bs) 
{
	for (int f = 0; f < samFiles.size(); f++) {
		auto fi = File::Open(samFiles[f].c_str(), "rb");
		char mc[2];
		fi->read(mc, 2);
		if (mc[0] == char(0x1f) && mc[1] == char(0x8b)) 
			parsers.push_back(make_shared<BAMParser>(samFiles[f]));
		else
			parsers.push_back(make_shared<SAMParser>(samFiles[f]));
		sequence.push_back(make_shared<SequenceCompressor>(genomeFile, bs));
		editOp.push_back(make_shared<EditOperationCompressor>(bs, (*sequence.back())));
		if (optReadLossy)
			readName.push_back(make_shared<ReadNameLossyCompressor>(bs));
		else 
			readName.push_back(make_shared<ReadNameCompressor>(bs));
		mapFlag.push_back(make_shared<MappingFlagCompressor>(bs));
		mapQual.push_back(make_shared<MappingQualityCompressor>(bs));
		quality.push_back(make_shared<QualityScoreCompressor>(bs));
		pairedEnd.push_back(make_shared<PairedEndCompressor>(bs));
		optField.push_back(make_shared<OptionalFieldCompressor>(bs));
	}
	if (outFile == "")
		outputFile = stdout;
	else {
		outputFile = fopen(outFile.c_str(), "wb");
		if (outputFile == NULL)
			throw DZException("Cannot open the file %s", outFile.c_str());
	}

	FILE *tmp = tmpfile();
	int tmpFile = dup(fileno(tmp));
	indexTmp = fdopen(tmpFile, "rb");
	indexFile = gzdopen(fileno(tmp), "wb6");
	if (indexFile == Z_NULL)	
		throw DZException("Cannot open temporary file");
}

FileCompressor::~FileCompressor (void) 
{
	fclose(outputFile);
}

void FileCompressor::compress (void) 
{
	ZAMAN_START_P(Compress);
	outputMagic();
	outputComment();
	outputRecords();
	ZAMAN_END_P(Compress);
}

void FileCompressor::outputMagic (void) 
{
	uint32_t magic = MAGIC;
	fwrite(&magic, 4, 1, outputFile);
	fwrite(&optQuality, 1, 1, outputFile);

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
}

void FileCompressor::outputComment (void) 
{
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

template<typename Compressor, typename... ExtraParams>
void FileCompressor::compressBlock (Array<uint8_t>& out, Array<uint8_t>& idxOut, size_t k, Compressor *c, ExtraParams... params) 
{
	out.resize(0);
	c->outputRecords(out, 0, k, params...);
	idxOut.resize(0);
	c->getIndexData(idxOut);
}

void FileCompressor::outputBlock (Array<uint8_t> &out, Array<uint8_t> &idxOut) 
{
	size_t out_sz = out.size();
	fwrite(&out_sz, sizeof(size_t), 1, outputFile);
	if (out_sz) 
		fwrite(out.data(), 1, out_sz, outputFile);
	
	out_sz = idxOut.size();
	gzwrite(indexFile, &out_sz, sizeof(size_t));
	if (out_sz) 
		gzwrite(indexFile, idxOut.data(), out_sz);
}

void FileCompressor::parser(size_t f, size_t start, size_t end)
{
	array<size_t, 3> stringEstimates { 0, 0, 0 };
	size_t maxEnd = 0;
	//auto t=zaman();
	for (size_t i= start; i < end; i++) {
		const Record &rc = recordsQueue[i];
		size_t loc = rc.getLocation();
		if (loc == (size_t)-1) loc = 0; 
		size_t p_loc = rc.getPairLocation();
		if (p_loc == (size_t)-1) p_loc = 0;

		ZAMAN_START(EO);
		EditOperation eo(loc, rc.getSequence(), rc.getCigar());
		maxEnd = max(maxEnd, eo.end);
		(*editOp[f])[i] = eo;
		ZAMAN_END(EO);

		ZAMAN_START(RN);
		(*readName[f])[i] = rc.getReadName();
		stringEstimates[0] += (*readName[f])[i].size() + 1;
		ZAMAN_END(RN);

		ZAMAN_START(MFMQPE);
		(*mapFlag[f])[i] = rc.getMappingFlag();
		(*mapQual[f])[i] = rc.getMappingQuality();

		PairedEndInfo pe(
			rc.getPairChromosome(), p_loc, rc.getTemplateLenght(), 
			eo.start, eo.end - eo.start, rc.getMappingFlag() & 0x10);
		(*pairedEnd[f])[i] = pe;
		ZAMAN_END(MFMQPE);

		ZAMAN_START(Q);
		(*quality[f])[i] = quality[f]->shrink(rc.getQuality(), rc.getMappingFlag());
		stringEstimates[1] += (*quality[f])[i].size() + 1;
		ZAMAN_END(Q);

		ZAMAN_START(OF);
		(*optField[f])[i] = rc.getOptional();
		stringEstimates[2] += (*optField[f])[i].size() + 1;
		ZAMAN_END(OF);
	}
	//LOG("done %'lu-%'lu in %.2lf %.2lf",start,end,(zaman()-t)/1000000.0, eox/1000000.0);
	std::unique_lock<std::mutex> lock(queueMutex);
	readName[f]->increaseTotalSize(stringEstimates[0]);
	quality[f]->increaseTotalSize(stringEstimates[1]);
	optField[f]->increaseTotalSize(stringEstimates[2]);
	sequence[f]->updateBoundary(maxEnd);
}

size_t FileCompressor::currentMemUsage(size_t f){
	return 0; 
		sizeInMemory(recordsQueue)+
		readName[f]->currentMemoryUsage()+
		quality[f]->currentMemoryUsage()+
		mapFlag[f]->currentMemoryUsage()+
		mapQual[f]->currentMemoryUsage()+
		editOp[f]->currentMemoryUsage()+
		sequence[f]->currentMemoryUsage()+
		pairedEnd[f]->currentMemoryUsage();
}

void FileCompressor::outputRecords (void) 
{
	vector<Stats> stats(parsers.size());
	for (int16_t f = 0; f < parsers.size(); f++) 
		stats[f].fileName = parsers[f]->fileName();

	vector<size_t> lastStart(parsers.size(), 0);
	vector<int64_t> bsc(parsers.size(), blockSize);
	int64_t total = 0;

	int64_t blockCount = 0;
	vector<int64_t> currentSize(parsers.size(), 0);
	Array<uint8_t> outputBuffer[8];
	Array<uint8_t> idxBuffer[8];
	for (int i = 0; i < 8; i++) {
		outputBuffer[i].set_extend(MB);
		idxBuffer[i].set_extend(MB);
	}

	vector<size_t> prevLoc(parsers.size(), 0);
	vector<thread> threads(optThreads);
	
	int totalMatchedMates = 0;

	int fileAliveCount = parsers.size();
	while (fileAliveCount) { 
		LOGN("\r");
		for (int16_t f = 0; f < parsers.size(); f++) {
			if (!parsers[f]->hasNext()) {
				fileAliveCount--;
				continue;
			}

			ZAMAN_START_P(SeekChromosome);
			char op = 0;
			string chr = parsers[f]->head();
			while (sequence[f]->getChromosome() != parsers[f]->head()) 
				sequence[f]->scanChromosome(parsers[f]->head()), op = 1, prevLoc[f] = 0;
			ZAMAN_END_P(SeekChromosome);

			int readInBlock = 0, matchedMates = 0;

		ZAMAN_START_P(ParseBlock);
			ZAMAN_START_P(InitThreads);
			readName[f]->resize(bsc[f]);
			editOp[f]->resize(bsc[f]);
			mapQual[f]->resize(bsc[f]);
			mapFlag[f]->resize(bsc[f]);
			optField[f]->resize(bsc[f]);
			quality[f]->resize(bsc[f]);
			pairedEnd[f]->resize(bsc[f]);
			recordsQueue.resize(bsc[f]+1);
			canAccept = true;
			ZAMAN_END_P(InitThreads);

			ZAMAN_START_P(ReadIO);
			if (!blockCount)
				recordsQueue[0] = parsers[f]->next(); // TODO Check Bounds
			for (; currentSize[f] < bsc[f] && parsers[f]->hasNext() 
					&& parsers[f]->head() == sequence[f]->getChromosome(); currentSize[f]++, readInBlock++) 
			{
				Record &rc = recordsQueue[readInBlock];
				
				size_t loc = rc.getLocation();
				if (loc == (size_t)-1) 
					loc = 0; 
				if (loc < prevLoc[f])
					throw DZSortedException("%s is not sorted. Please sort it with 'dz --sort' before compressing it", parsers[f]->fileName().c_str());
				prevLoc[f] = loc;
				lastStart[f] = loc;
				ZAMAN_START_P(Stats);
				stats[f].addRecord(rc.getMappingFlag());
				ZAMAN_END_P(Stats);
				((SAMParser*)parsers[f].get())->readNextTo(recordsQueue[readInBlock + 1]);		
			}
			ZAMAN_END_P(ReadIO);	
			LOG("Memory after reading: %'lu", currentMemUsage(f));
	
			// WAIT FOR THREADS
			ZAMAN_START_P(ParseRecords);
			//qp = 0; qend = readInBlock;
			readInBlock++;
			size_t threadSz = readInBlock / optThreads + 1;
			for (int i = 0; i < optThreads; i++)
				threads[i] = thread([=](int ti, size_t start, size_t end) {
					ZAMAN_START(Thread);
					this->parser(f, start, end);
					ZAMAN_END(Thread);
				}, i, threadSz * i, min(size_t(readInBlock), (i + 1) * threadSz));
			for (int i = 0; i < optThreads; i++)
				threads[i].join();
			recordsQueue[0] = recordsQueue[readInBlock];
			ZAMAN_END_P(ParseRecords);
			LOG("Memory after parsing: %'lu", currentMemUsage(f));

			total += currentSize[f];
			LOGN("  %5.1lf%% [%s,%zd]", (100.0 * parsers[f]->fpos()) / parsers[f]->fsize(), 
				sequence[f]->getChromosome().c_str(), blockCount + 1);
		
		ZAMAN_END_P(ParseBlock);

		ZAMAN_START_P(CheckMate);
			unordered_map<string, int> readNames; 
			for (size_t i = 0; i < readInBlock; i++) {
				auto &rn = (*readName[f])[i];
				auto it = readNames.find(rn);
				if (it == readNames.end()) {
					readNames[rn] = i;
				} else {
					// Check...
					EditOperation &eo = (*editOp[f])[i];
					PairedEndInfo &pe = (*pairedEnd[f])[i];
					EditOperation &peo = (*editOp[f])[it->second];
					PairedEndInfo &ppe = (*pairedEnd[f])[it->second];
					if (pe.tlen == -ppe.tlen // TLEN can be calculated
						&& eo.start == ppe.pos
						&& pe.pos == peo.start // POS can be calculated
						&& (ppe.chr == "=" || chr == ppe.chr) // CHR can be calculated as well
						&& ppe.tlen >= 0 
						&& ppe.tlen == eo.start + (peo.end - peo.start) - peo.start
					)
					{
						ppe.bit = PairedEndInfo::Bits::LOOK_AHEAD; 
						pe.bit = PairedEndInfo::Bits::LOOK_BACK; // DO NOT ADD!
						pe.tlen = i - it->second;
						//addReadName = false;
						matchedMates++;
						rn = "";
					} else { // Replace with current read
						readNames[rn] = i;
					}
				}
			}
		ZAMAN_END_P(CheckMate);

		ZAMAN_START_P(CalculateMD);
			ZAMAN_START_P(Load);
			sequence[f]->getReference().loadIntoBuffer(sequence[f]->getBoundary());
			ZAMAN_END_P(Load);
			threadSz = currentSize[f] / optThreads + 1;
			for (int i = 0; i < optThreads; i++) {
				threads[i] = thread([&](int ti, size_t start, size_t end) {
					ZAMAN_START(Thread);
					for (int i = start; i < end; i++) 
						(*editOp[f])[i].calculateTags(sequence[f]->getReference());
					ZAMAN_END(Thread);
				}, i, threadSz * i, min(size_t(currentSize[f]), (i + 1) * threadSz));
			}
			for (int i = 0; i < optThreads; i++)
				threads[i].join();
		ZAMAN_END_P(CalculateMD);
			//LOG("Memory after calculating: %'lu", currentMemUsage(f));
			//LOG("Completed, processed %d", queuePosition[f]);

		ZAMAN_START_P(FixGenome);
			size_t currentBlockCount, 
				currentBlockFirstLoc, 
				currentBlockLastLoc, 
				currentBlockLastEndLoc,
				fixedStartPos, fixedEndPos;
			size_t zpos;
			editOp[f]->resize(currentSize[f]); // HACK
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
				ZAMAN_END_P(FixGenome);
				continue;
			}
			currentSize[f] -= currentBlockCount;
		ZAMAN_END_P(FixGenome);
			//LOG("Memory after fixing: %'lu", currentMemUsage(f));

		ZAMAN_START_P(WriteIndex);
			zpos = ftell(outputFile);

			fwrite(&op, sizeof(char), 1, outputFile);
			if (op) 
				fwrite(chr.c_str(), chr.size() + 1, 1, outputFile);

			gzwrite(indexFile, &f, sizeof(int16_t));
			gzwrite(indexFile, &zpos, sizeof(size_t));		
			gzwrite(indexFile, &currentBlockCount, sizeof(size_t));
			gzwrite(indexFile, sequence[f]->getChromosome().c_str(), sequence[f]->getChromosome().size() + 1);
			gzwrite(indexFile, &currentBlockFirstLoc, sizeof(size_t));
			gzwrite(indexFile, &currentBlockLastLoc, sizeof(size_t));
			gzwrite(indexFile, &fixedStartPos, sizeof(size_t));
			gzwrite(indexFile, &fixedEndPos, sizeof(size_t));
		ZAMAN_END_P(WriteIndex);

		ZAMAN_START_P(CompressBlocks);
			Compressor *ci[] = { sequence[f].get(), editOp[f].get(), readName[f].get(), mapFlag[f].get(), mapQual[f].get(), quality[f].get(), pairedEnd[f].get(), optField[f].get() };
			ctpl::thread_pool threadPool(optThreads);
			for (size_t ti = 0; ti < 7; ti++) 
				threadPool.push([&](int t, int ti) {
					ZAMAN_START(Thread);
					compressBlock(outputBuffer[ti], idxBuffer[ti], currentBlockCount, ci[ti]);
					ZAMAN_END(Thread);
				//	LOG("Compressor %d done", ti);
				}, ti);
			threadPool.push([&](int t) {
				ZAMAN_START(Thread);
				compressBlock(outputBuffer[7], idxBuffer[7], currentBlockCount, optField[f].get(), editOp[f].get());
				ZAMAN_END(Thread);
			//	LOG("Compressor %d done", 7);
			});
			threadPool.stop(true);
			editOp[f]->records.remove_first_n(currentBlockCount);
		ZAMAN_END_P(CompressBlocks);
			//LOG("Memory after compressing: %'lu", currentMemUsage(f));

		ZAMAN_START_P(OutputBlocks);
			for (int ti = 0; ti < 8; ti++)
				outputBlock(outputBuffer[ti], idxBuffer[ti]);
		ZAMAN_END_P(OutputBlocks);

			totalMatchedMates += matchedMates;
			blockCount++;
		}
	}
	LOGN("\nWritten %'zd lines\n", total);
	fflush(outputFile);
	
	ZAMAN_START_P(WriteIndex);
	size_t posStats = ftell(outputFile);
	fwrite("DZSTATS", 1, 7, outputFile);
	for (int i = 0; i < stats.size(); i++) {
		stats[i].writeStats(outputBuffer[0], sequence[i].get());
		size_t sz = outputBuffer->size();
		fwrite(&sz, 8, 1, outputFile);
		fwrite(outputBuffer->data(), 1, outputBuffer->size(), outputFile);
	}
	
	int gzst = gzclose(indexFile);
	fwrite("DZIDX", 1, 5, outputFile);
	char *buffer = (char*)malloc(MB);
	fseek(indexTmp, 0, SEEK_SET);
	while (size_t sz = fread(buffer, 1, MB, indexTmp))
		fwrite(buffer, 1, sz, outputFile);
	free(buffer);
	fclose(indexTmp);
	fwrite(&posStats, sizeof(size_t), 1, outputFile);
	ZAMAN_END_P(WriteIndex);
	
	LOG("Paired %'d out of %'d reads", totalMatchedMates, total);
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

