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
		sequence.push_back(make_shared<SequenceCompressor>(genomeFile));
		editOp.push_back(make_shared<EditOperationCompressor>(*sequence.back()));
		readName.push_back(make_shared<ReadNameCompressor>());
		mapFlag.push_back(make_shared<MappingFlagCompressor>());
		mapQual.push_back(make_shared<MappingQualityCompressor>());
		quality.push_back(make_shared<QualityScoreCompressor>());
		pairedEnd.push_back(make_shared<PairedEndCompressor>());
		optField.push_back(make_shared<OptionalFieldCompressor>());
	}
	if (outFile == "") {
		outputFile = stdout;
	} else {
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
		samComment.push_back(SAMComment(comment));
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
void FileCompressor::compressBlock (int f, Array<uint8_t>& out, Array<uint8_t>& idxOut, size_t k, shared_ptr<Compressor> c, ExtraParams&... params) 
{
	out.resize(0);
	c->outputRecords(records[f], out, 0, k, params...);
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
	for (size_t i= start; i < end; i++) {
		const Record &rc = records[f][i];
		size_t loc = rc.getLocation();
		if (loc == (size_t)-1) loc = 0; 
		size_t p_loc = rc.getPairLocation();
		if (p_loc == (size_t)-1) p_loc = 0;

		editOps[f][i] = EditOperation(loc, rc.getSequence(), rc.getCigar());

		auto &eo = editOps[f][i];
		assert(eo.ops.size());
		maxEnd = max(maxEnd, eo.end);
		pairedEndInfos[f][i] = PairedEndInfo(
			rc.getPairChromosome(), p_loc, rc.getTemplateLenght(), 
			eo.start, eo.end - eo.start, rc.getMappingFlag() & 0x10);

		size_t est = quality[f]->shrink((char*)rc.getQuality(), strlen(rc.getQuality()), rc.getMappingFlag());
		stringEstimates[0] += rc.getReadNameSize() + 1;
		stringEstimates[1] += est + 1;
		stringEstimates[2] += rc.getOptionalSize() + 1;
	}
	std::unique_lock<std::mutex> lock(queueMutex);
	readName[f]->increaseTotalSize(stringEstimates[0]);
	quality[f]->increaseTotalSize(stringEstimates[1]);
	optField[f]->increaseTotalSize(stringEstimates[2]);
	sequence[f]->updateBoundary(maxEnd);
}

size_t FileCompressor::currentMemUsage(size_t f)
{
	return 0; //sizeInMemory(records) + sizeInMemory(editOps) + sizeInMemory(pairedEndInfos); 
}

void FileCompressor::outputRecords (void) 
{
	vector<Stats> stats(parsers.size());
	vector<size_t> lastStart(parsers.size(), 0);
	vector<int64_t> currentBlockSize(parsers.size(), blockSize);
	vector<size_t> prevLoc(parsers.size(), 0);
	vector<int64_t> currentSize(parsers.size(), 0);
	for (int16_t f = 0; f < parsers.size(); f++) {
		stats[f].fileName = parsers[f]->fileName();
		records.push_back(Array<Record>(blockSize));
		editOps.push_back(Array<EditOperation>(blockSize));
		pairedEndInfos.push_back(Array<PairedEndInfo>(blockSize));
	}
	int64_t total = 0;
	int64_t blockCount = 0;
	int totalMatchedMates = 0;

	Array<uint8_t> outputBuffer[8];
	Array<uint8_t> idxBuffer[8];
	for (int i = 0; i < 8; i++) {
		outputBuffer[i].set_extend(MB);
		idxBuffer[i].set_extend(MB);
	}
	vector<thread> threads(optThreads);

	for (int fileAliveCount = 0; fileAliveCount < parsers.size();) { 
		LOGN("\r");
		for (int16_t f = 0; f < parsers.size(); f++) {
			if (!parsers[f]->hasNext()) {
				fileAliveCount++;
				continue;
			}

		ZAMAN_START_P(Seek);
			char op = 0;
			string chr = parsers[f]->head();
			while (sequence[f]->getChromosome() != parsers[f]->head()) 
				sequence[f]->scanChromosome(parsers[f]->head(), samComment[f]), op = 1, prevLoc[f] = 0;
		ZAMAN_END_P(Seek);

		ZAMAN_START_P(Parse);
			ZAMAN_START_P(GetReads);
			size_t blockOffset = currentSize[f];
			for (; currentSize[f] < currentBlockSize[f] && parsers[f]->hasNext() && parsers[f]->head() == sequence[f]->getChromosome(); currentSize[f]++) {
				records[f].add();
				pairedEndInfos[f].add();
				editOps[f].add();
				records[f][records[f].size() - 1] = std::move(parsers[f]->next());
				const Record &rc = records[f][records[f].size() - 1];
				
				size_t loc = rc.getLocation();
				if (loc == (size_t)-1) 
					loc = 0;
				if (loc < prevLoc[f]) {
					throw DZSortedException("%s is not sorted. Please sort it with 'dz --sort' before compressing it", parsers[f]->fileName().c_str());
				}
				prevLoc[f] = loc;
				lastStart[f] = loc;
				ZAMAN_START_P(Stats);
				stats[f].addRecord(rc.getMappingFlag());
				ZAMAN_END_P(Stats);

				parsers[f]->readNext();
				//LOG("%s %s %d %d %d", parsers[f]->head().c_str(), sequence[f]->getChromosome().c_str(), parsers[f]->hasNext(),
				//	currentBlockSize[f], currentSize[f]);
			}
			ZAMAN_END_P(GetReads);	
			//LOG("Memory after reading: %'lu", currentMemUsage(f));
	
			ZAMAN_START_P(ParseRecords);
			assert(records[f].size() == currentSize[f]);
			size_t threadSz = (currentSize[f] - blockOffset) / optThreads + 1;
			for (int i = 0; i < optThreads; i++)
				threads[i] = thread([=](int ti, size_t start, size_t end) {
						ZAMAN_START(Thread);
						this->parser(f, start, end);
						ZAMAN_END(Thread);
						ZAMAN_THREAD_JOIN();
					}, 
					i, 
					blockOffset + threadSz * i, 
					blockOffset + min(size_t(currentSize[f] - blockOffset), (i + 1) * threadSz)
				);
			for (int i = 0; i < optThreads; i++)
				threads[i].join();
			ZAMAN_END_P(ParseRecords);
			
			ZAMAN_START_P(ParseQualities);
			quality[f]->calculateOffset(records[f]);
			quality[f]->offsetRecords(records[f]);
			ZAMAN_END_P(ParseQualities);

			ZAMAN_START_P(Load);
			sequence[f]->getReference().loadIntoBuffer(sequence[f]->getBoundary());
			ZAMAN_END_P(Load);

			ZAMAN_START_P(ParseMDandNM);
			threadSz = (currentSize[f] - blockOffset) / optThreads + 1;
			for (int i = 0; i < optThreads; i++) 
				threads[i] = thread([&](int ti, size_t start, size_t end) {
						ZAMAN_START(Thread);
						for (int i = start; i < end; i++) 
							editOps[f][i].calculateTags(sequence[f]->getReference());
						ZAMAN_END(Thread);
						ZAMAN_THREAD_JOIN();
					}, 
					i, 
					blockOffset + threadSz * i, 
					blockOffset + min(size_t(currentSize[f] - blockOffset), (i + 1) * threadSz)
				);
			for (int i = 0; i < optThreads; i++)
				threads[i].join();
			//LOG("Memory after calculating: %'lu", currentMemUsage(f));
			ZAMAN_END_P(ParseMDandNM);

			total += currentSize[f] - blockOffset;
			LOGN("  %5.1lf%% [%s,%zd]", (100.0 * parsers[f]->fpos()) / parsers[f]->fsize(), 
				sequence[f]->getChromosome().c_str(), blockCount + 1);
		ZAMAN_END_P(Parse);

		ZAMAN_START_P(Fix);
			size_t 
				currentBlockCount, 
				currentBlockFirstLoc, 
				currentBlockLastLoc, 
				currentBlockLastEndLoc,
				fixedStartPos, 
				fixedEndPos,
				zpos;

			size_t fixingEnd = lastStart[f] - 1;
			if (!parsers[f]->hasNext() 
				|| parsers[f]->head() != sequence[f]->getChromosome() 
				|| parsers[f]->head() == "*") 
			{
				fixingEnd = (size_t)-1;
			}

			currentBlockCount = sequence[f]->applyFixes(
				fixingEnd, records[f], editOps[f],
				currentBlockFirstLoc, currentBlockLastLoc, currentBlockLastEndLoc, fixedStartPos, fixedEndPos
			);
			if (currentBlockCount == 0) {
				currentBlockSize[f] += blockSize;
				LOG("Retrying...");
				ZAMAN_END_P(Fix);
				continue;
			}
			currentSize[f] -= currentBlockCount;
		ZAMAN_END_P(Fix);
			//LOG("Memory after fixing: %'lu", currentMemUsage(f));

		ZAMAN_START_P(CheckMate);
			int matchedMates = 0;
			unordered_map<string, int> readNames; 
			for (size_t i = 0; i < currentBlockCount; i++) {
				string rn = records[f][i].getReadName();
				auto it = readNames.find(rn);
				if (it == readNames.end()) {
					readNames[rn] = i;
				} else { // Check can we calculate back the necessary values
					EditOperation &eo = editOps[f][i];
					PairedEndInfo &pe = pairedEndInfos[f][i];
					EditOperation &peo = editOps[f][it->second];
					PairedEndInfo &ppe = pairedEndInfos[f][it->second];
				//	LOG("%s %s %s", rn.c_str(), eo.op.c_str(), peo.op.c_str());
				//	LOG("%d %d %d", pe.tlen, ppe.tlen, eo.start + (peo.end - peo.start) - peo.start);
				//	LOG("%d %d (%d) %d %d (%d)", eo.start, eo.end, pe.pos, peo.start, peo.end, ppe.pos);
					uint32_t off;
					if (pe.tlen == -ppe.tlen // TLEN can be calculated
						&& eo.start == ppe.pos
						&& pe.pos == peo.start // POS can be calculated
						&& (ppe.chr == "=" || chr == ppe.chr) // CHR can be calculated as well
						&& ppe.tlen >= 0 
						&& (off = eo.start + (peo.end - peo.start) - peo.start - ppe.tlen) <= 0)
					{
						ppe.bit = PairedEndInfo::Bits::LOOK_AHEAD + off; 
						pe.bit = PairedEndInfo::Bits::LOOK_BACK;
						pe.tlen = i - it->second;
						LOG("%d...", pe.tlen);
						matchedMates++;
					} else { // Replace with current read
						readNames[rn] = i;
					}
				}
			}
			totalMatchedMates += matchedMates;
		ZAMAN_END_P(CheckMate);

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

		ZAMAN_START_P(Compress);
			ctpl::thread_pool threadPool(optThreads);
			threadPool.push([&](int t, int ti) {
				compressBlock(f, outputBuffer[ti], idxBuffer[ti], currentBlockCount, sequence[f]);
				ZAMAN_THREAD_JOIN();
			//	LOG("seq done");
			}, 0);
			threadPool.push([&](int t, int ti) {
			 	compressBlock(f, outputBuffer[ti], idxBuffer[ti], currentBlockCount, editOp[f], editOps[f]);
			 	ZAMAN_THREAD_JOIN();
			// 	LOG("eo done");
			}, 1);
			threadPool.push([&](int t, int ti) {
				compressBlock(f, outputBuffer[ti], idxBuffer[ti], currentBlockCount, readName[f], pairedEndInfos[f]);
				ZAMAN_THREAD_JOIN();
			//	LOG("rn done");
			}, 2);
			threadPool.push([&](int t, int ti) {
				compressBlock(f, outputBuffer[ti], idxBuffer[ti], currentBlockCount, mapFlag[f]);
				ZAMAN_THREAD_JOIN();
			//	LOG("mf done");
			}, 3);
			threadPool.push([&](int t, int ti) {
				compressBlock(f, outputBuffer[ti], idxBuffer[ti], currentBlockCount, mapQual[f]);
				ZAMAN_THREAD_JOIN();
			//	LOG("mq done");
			}, 4);
			threadPool.push([&](int t, int ti) {
				compressBlock(f, outputBuffer[ti], idxBuffer[ti], currentBlockCount, quality[f]);
			//compressBlock(f, outputBuffer[5], idxBuffer[5], currentBlockCount, quality[f]);
				ZAMAN_THREAD_JOIN();
			}, 5);
			threadPool.push([&](int t, int ti) {
				compressBlock(f, outputBuffer[ti], idxBuffer[ti], currentBlockCount, pairedEnd[f], pairedEndInfos[f]);
				ZAMAN_THREAD_JOIN();
			//	LOG("pe done");
			}, 6);
			threadPool.push([&](int t, int ti) {
				compressBlock(f, outputBuffer[ti], idxBuffer[ti], currentBlockCount, optField[f], editOps[f]);
				ZAMAN_THREAD_JOIN();
			//	LOG("of done");
			}, 7);
			threadPool.stop(true);
			records[f].removeFirstK(currentBlockCount);
			editOps[f].removeFirstK(currentBlockCount);
			pairedEndInfos[f].removeFirstK(currentBlockCount);
			// LOG("Memory after compressing: %'lu", currentMemUsage(f));
		ZAMAN_END_P(Compress);

		ZAMAN_START_P(Output);
			for (int ti = 0; ti < 8; ti++)
				outputBlock(outputBuffer[ti], idxBuffer[ti]);
		ZAMAN_END_P(Output);

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
	#define VERBOSE(y,x) \
		LOG("%-12s  %'20lu", y, x->compressedSize()); x->printDetails();
	for (int f = 0; f < parsers.size(); f++) {
		VERBOSE("Reference", sequence[f]);
		VERBOSE("Sequences", editOp[f]);
		VERBOSE("Read Names", readName[f]);
		VERBOSE("Flags", mapFlag[f]);
		VERBOSE("Map. Quals", mapQual[f]);
		VERBOSE("Qualities", quality[f]);
		VERBOSE("Paired End", pairedEnd[f]);
		VERBOSE("Optionals", optField[f]);
	}
}

