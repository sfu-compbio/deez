#include "Decompress.h"
#include <thread>
using namespace std;

void FileDecompressor::printStats (const string &path, int filterFlag) 
{
	initCache();
	
	auto inFile = File::Open(path.c_str(), "rb");

	if (inFile == NULL)
		throw DZException("Cannot open the file %s", path.c_str());

	size_t inFileSz = inFile->size();
	uint32_t magic = inFile->readU32();
	uint8_t optQuality = inFile->readU8();
	uint16_t numFiles = 1;
	if ((magic & 0xff) < 0x20)
		throw DZException("Old DeeZ files are not supported");
	if ((magic & 0xff) >= 0x11) // DeeZ v1.1
		numFiles = inFile->readU16();

	// seek to index
	inFile->seek(inFileSz - sizeof(size_t));
	size_t statPos = inFile->readU64();

	char statmagic[10] = {0};
	inFile->read(statmagic, 7, statPos);
	if (strcmp(statmagic, "DZSTATS"))
		throw DZException("Stats are corrupted ...%s", statmagic);

	vector<shared_ptr<Stats>> stats(numFiles);
	for (int i = 0; i < numFiles; i++) {
		size_t sz = inFile->readU64();
		Array<uint8_t> in(sz);
		in.resize(sz);
		inFile->read(in.data(), sz);
		stats[i] = make_shared<Stats>(in, magic);
	}

	WARN("Index size %'lu bytes", inFileSz - statPos);
	for (int f = 0; f < numFiles; f++) {
		if ((magic & 0xff) >= 0x11) // DeeZ v1.1
			WARN("=== File %s ===", stats[f]->fileName.c_str());
		WARN("%'16lu reads", stats[f]->getReadCount());
		WARN("%'16lu mapped reads", stats[f]->getStats(-4));
		WARN("%'16lu unmapped reads", stats[f]->getStats(4));
		WARN("%'16lu chromosomes in reference file:", stats[f]->getChromosomeCount());

		if ((magic & 0xff) >= 0x11) // DeeZ v1.1
			foreach (chr, stats[f]->chromosomes) 
				WARN("\t%s: size %'lu, file %s, MD5 %s",
					chr->first.c_str(), 
					stats[f]->chromosomes[chr->first].len,
					stats[f]->chromosomes[chr->first].filename == "" ? "<n/a>" : stats[f]->chromosomes[chr->first].filename.c_str(),
					stats[f]->chromosomes[chr->first].md5 == "" ? "<n/a>" : stats[f]->chromosomes[chr->first].md5.c_str());

		if (filterFlag) {
			size_t p = stats[f]->getStats(filterFlag);
			if (filterFlag > 0)
				WARN("%'16lu records with flag %d(0x%x)", p, filterFlag, filterFlag);
			else
				WARN("%'16lu records without flag %d(0x%x)", p, -filterFlag, -filterFlag);
		} else {
			for (int i = 0; i < Stats::FLAGCOUNT; i++) {
				size_t p = stats[f]->getFlagCount(i);
				if (p) WARN("%4d 0x%04x: %'16lu", i, i, p);
			}
		}
	}
}

FileDecompressor::FileDecompressor (const string &inFilePath, const string &outFile, const string &genomeFile, int bs, bool isAPI): 
	blockSize(bs), genomeFile(genomeFile), outFile(outFile), finishedRange(false), isAPI(isAPI)
{
	initCache();

	string name1 = inFilePath;
	this->inFile = File::Open(name1.c_str(), "rb");
	if (this->inFile == NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	inFileSz = inFile->size();
	magic = inFile->readU32();

	if ((magic & 0xff) < 0x20)
		throw DZException("Old DeeZ files are not supported");

	// seek to index
	inFile->seek(inFileSz - sizeof(size_t));
	size_t statPos = inFile->readU64();
	char keymagic[10] = {0};
	inFile->read(keymagic, 7, statPos);
	if (strcmp(keymagic, "DZSTATS"))
		throw DZException("Stats are corrupted ...%c%c%c%c%c%c%c", keymagic[0], keymagic[1], keymagic[2], keymagic[3], keymagic[4], keymagic[5], keymagic[6]);

	size_t sz = inFile->readU64();
	Array<uint8_t> in(sz);
	in.resize(sz);
	inFile->read(in.data(), sz);
	stats = make_shared<Stats>(in, magic);

	keymagic[5] = 0;
	inFile->read(keymagic, 5);
	if (strcmp(keymagic, "DZIDX"))
		throw DZException("Index is corrupted ...%s", keymagic);

	size_t idxToRead = inFileSz - inFile->tell() - sizeof(size_t);
	FILE *tmp = tmpfile();
	char *buffer = (char*)malloc(MB);

	while (idxToRead && (sz = inFile->read(buffer, min(uint64_t(MB), (uint64_t)idxToRead)))) {
		fwrite(buffer, 1, sz, tmp);
		idxToRead -= sz;
	}
	free(buffer);
	
	int idx = dup(fileno(tmp));
	fseek(tmp, 0, SEEK_SET);
	lseek(idx, 0, SEEK_SET); // needed for gzdopen
	idxFile = gzdopen(idx, "rb");
	if (idxFile == Z_NULL)
		throw DZException("Cannot open the index");
		
	inFile->seek(0);

	getMagic();
	getComment();
	loadIndex();
}

FileDecompressor::~FileDecompressor (void) 
{
	for (int f = 0; f < fileNames.size(); f++) {
		if (samFiles[f]) 
			fclose(samFiles[f]);
	}
	if (idxFile) 
		gzclose(idxFile);
}

void FileDecompressor::getMagic (void) 
{
	magic = inFile->readU32();
	LOG("File format: %c%c v%d.%d",
		(magic >> 16) & 0xff,
	 	(magic >> 8) & 0xff,
	 	(magic >> 4) & 0xf,
	 	magic & 0xf
	);
	optQuality = inFile->readU8();
	optBzip = inFile->readU8();
	if (optBzip) LOG("Using bzip decoding");

	uint16_t numFiles = 1;
	if ((magic & 0xff) >= 0x11) // DeeZ v1.1
		numFiles = inFile->readU16();

	for (int f = 0; f < numFiles; f++) {
		sequence.push_back(make_shared<SequenceDecompressor>(genomeFile, blockSize));
		editOp.push_back(make_shared<EditOperationDecompressor>(blockSize, (*sequence.back())));
		readName.push_back(make_shared<ReadNameDecompressor>(blockSize));
		mapFlag.push_back(make_shared<MappingFlagDecompressor>(blockSize));
		mapQual.push_back(make_shared<MappingQualityDecompressor>(blockSize));
		pairedEnd.push_back(make_shared<PairedEndDecompressor>(blockSize));
		optField.push_back(make_shared<OptionalFieldDecompressor>(blockSize));
		quality.push_back(make_shared<QualityScoreDecompressor>(blockSize));
	}
	fileNames.resize(numFiles);

	if ((magic & 0xff) >= 0x11) { // DeeZ v1.1 file name information
		size_t arcsz = inFile->readU64();
		if (arcsz) {
			Array<uint8_t> arc;
			arc.resize(arcsz);
			arcsz = inFile->readU64();
			Array<uint8_t> files;
			files.resize(arcsz);
			inFile->read(files.data(), arcsz);
			
			GzipDecompressionStream gzc;
			gzc.decompress(files.data(), files.size(), arc, 0);

			arcsz = 0;
			for (int f = 0; f < fileNames.size(); f++) {
				fileNames[f] = string((const char*)(arc.data()) + arcsz);
				arcsz += fileNames[f].size();
			}
		}
	}
	samFiles.resize(numFiles);
	for (int f = 0; f < numFiles; f++) {
		if (optStdout) {
			samFiles[f] = stdout;
		} else if (outFile != "") {
			string fn = outFile;
			if (numFiles > 1) fn += S("_%d", f + 1);
			samFiles[f] = fopen(fn.c_str(), "wb");
			if (samFiles[f] == NULL)
				throw DZException("Cannot open the file %s", fn.c_str());
		}
	}
}

void FileDecompressor::getComment (void) 
{
	comments.resize(fileNames.size());
	for (int f = 0; f < fileNames.size(); f++) {
		size_t arcsz = inFile->readU64();
		string comment = "";
		if (arcsz) {
			Array<uint8_t> arc;
			arc.resize(arcsz);
			arcsz = inFile->readU64();
			Array<uint8_t> commentArr;
			commentArr.resize(arcsz);
			inFile->read(commentArr.data(), arcsz);
			
			GzipDecompressionStream gzc;
			gzc.decompress(commentArr.data(), commentArr.size(), arc, 0);
			comment = string((char*)arc.data(), arc.size());
		}
		comments[f] = comment;
		samComment.push_back(SAMComment(comments[f]));
	}
}

void FileDecompressor::readBlock (Array<uint8_t> &in) 
{
	size_t sz = inFile->readU64();
	in.resize(sz);
	if (sz) inFile->read(in.data(), sz);
}

void FileDecompressor::printRecord(const string &rname, int flag, 
	const string &chr, const EditOperation &eo, int mqual, 
	const string &qual, const string &optional, const PairedEndInfo &pe, 
	int file, int thread)
{
}

size_t FileDecompressor::getBlock (int f, const string &chromosome, 
	size_t start, size_t end, int filterFlag) 
{
	static string chr;
	if (finishedRange) 
		return 0;
	if (chromosome != "")
		chr = chromosome;

	char chflag;
	if (inFile->read(&chflag, 1) != 1)
		return 0;
	if (chflag > 1) // index!
		return 0;
	if (chflag) {
		chr = "";
		chflag = inFile->readU8();
		while (chflag)
			chr += chflag, chflag = inFile->readU8();
		if (chromosome != "" && chr != chromosome)
			return 0;
	}
	while (chr != sequence[f]->getChromosome())
		sequence[f]->scanChromosome(chr, samComment[f]);

	ZAMAN_START_P(Blocks);
	Array<uint8_t> in[8];

	for (int ti = 0; ti < 8; ti++) {
		readBlock(in[ti]);
	}

	ctpl::thread_pool threadPool(optThreads);
	threadPool.push([&](int t, Array<uint8_t> &buffer) {
		optField[f]->importRecords(buffer.data(), buffer.size());
		ZAMAN_THREAD_JOIN();
		// LOG("opt done");
	}, ref(in[7]));
	threadPool.push([&](int t, Array<uint8_t> &buffer) {
		sequence[f]->importRecords(buffer.data(), buffer.size());
		ZAMAN_THREAD_JOIN();
		// LOG("seq done");
	}, ref(in[0]));
	threadPool.push([&](int t, Array<uint8_t> &buffer) {
		editOp[f]->importRecords(buffer.data(), buffer.size());
		ZAMAN_THREAD_JOIN();
		// LOG("seq done");
	}, ref(in[1]));
	threadPool.push([&](int t, Array<uint8_t> &buffer) {
		readName[f]->importRecords(buffer.data(), buffer.size());
		ZAMAN_THREAD_JOIN();
		// LOG("rname done");
	}, ref(in[2]));
	threadPool.push([&](int t, Array<uint8_t> &buffer) {
		mapFlag[f]->importRecords(buffer.data(), buffer.size());
		ZAMAN_THREAD_JOIN();
		// LOG("mflag done");
	}, ref(in[3]));
	threadPool.push([&](int t, Array<uint8_t> &buffer) {
		mapQual[f]->importRecords(buffer.data(), buffer.size());
		ZAMAN_THREAD_JOIN();
		// LOG("mqual done");
	}, ref(in[4]));
	threadPool.push([&](int t, Array<uint8_t> &buffer) {
		quality[f]->importRecords(buffer.data(), buffer.size());
		ZAMAN_THREAD_JOIN();
		// LOG("qual done");
	}, ref(in[5]));
	threadPool.push([&](int t, Array<uint8_t> &buffer) {
		pairedEnd[f]->importRecords(buffer.data(), buffer.size());
		ZAMAN_THREAD_JOIN();
		// LOG("pe done");
	}, ref(in[6]));
	optField[f]->decompressThreads(threadPool);
	threadPool.stop(true);
	ZAMAN_END_P(Blocks);

	// TODO: stop early if slice/random access
	ZAMAN_START_P(CheckMate);
	for (int i = 0, j = 0; i < editOp[f]->size(); i++) {
		PairedEndInfo &pe = (*pairedEnd[f])[i];
		if (pe.bit == PairedEndInfo::Bits::LOOK_BACK) {
			int prevPos = i - readName[f]->getPaired(j++);
			(*readName[f])[i] = (*readName[f])[prevPos];
						
			EditOperation &eo = (*editOp[f])[i];
			EditOperation &peo = (*editOp[f])[prevPos];
			PairedEndInfo &ppe = (*pairedEnd[f])[prevPos];

			ppe.tlen = eo.start + (peo.end - peo.start) - peo.start;
			ppe.pos = eo.start;
			pe.pos = peo.start;
			pe.tlen = -ppe.tlen;

			if (ppe.bit == PairedEndInfo::Bits::LOOK_AHEAD_1) 
				pe.tlen++, ppe.tlen--;
		}
	}
	ZAMAN_END_P(CheckMate);

	ZAMAN_START_P(Parse);
	size_t recordCount = editOp[f]->size();
	size_t threadSz = recordCount / optThreads + 1;
	vector<thread> threads(optThreads);
	vector<vector<string>> records(optThreads);
	vector<char> finishedRangeThread(optThreads, 0);
	size_t count = 0;

	for (int ti = 0; ti < optThreads; ti++) {
		if (ti) records[ti].reserve(threadSz);
		threads[ti] = thread([&](int ti, size_t S, size_t E) {
			ZAMAN_START(Thread);
			size_t maxLen = 255;
			string record;
			for (size_t i = S; i < E; i++) {
				int flag = mapFlag[f]->getRecord(i);

				if (filterFlag) {
					if (filterFlag > 0 && (flag & filterFlag) != filterFlag)
						continue;
					if (filterFlag < 0 && (flag & -filterFlag) == -filterFlag)
						continue;
				}

				auto &eo = editOp[f]->getRecord(i);
				auto &pe = pairedEnd[f]->getRecord(i, eo.start, eo.end - eo.start, flag & 0x10);

				if (chr != "*") 
					eo.start++;
				if (pe.chr != "*") 
					pe.pos++;

				if (eo.start < start)
					continue;
				if (eo.start > end) {
					finishedRangeThread[ti] = true;
					return;
				}

				if (isAPI) {
					string of;
					optField[f]->getRecord(i, eo, of);

					//LOG("%d %d", ti, eo.start);
					printRecord(readName[f]->getRecord(i), 
						flag, chr, eo, 
						mapQual[f]->getRecord(i),
						quality[f]->getRecord(i, eo.seq.size(), flag), 
						of, pe, f, ti);
					count++;
					continue;
				}

				record.resize(0);
				record += readName[f]->getRecord(i);
				record += '\t';
				inttostr(flag, record); 
				record += '\t';
				record += chr; 
				record += '\t';
				inttostr(eo.start, record); 
				record += '\t';
				inttostr(mapQual[f]->getRecord(i), record); 
				record += '\t';
				record += eo.op; 
				record += '\t';
				record += pe.chr; 
				record += '\t';
				inttostr(pe.pos, record); 
				record += '\t';
				inttostr(pe.tlen, record); 
				record += '\t';
				record += eo.seq; 
				record += '\t';
				record += quality[f]->getRecord(i, eo.seq.size(), flag);
				optField[f]->getRecord(i, eo, record);
				maxLen = max(record.size(), maxLen);
				record.reserve(maxLen);

				if (ti == 0) {
					printRecord(record, f);
					count++;
				} else {
					records[ti].push_back(record);
				}
			}
			ZAMAN_END(Thread);
			ZAMAN_THREAD_JOIN();
		}, ti, threadSz * ti, min(recordCount, (ti + 1) * threadSz));
	}
	for (int ti = 0; ti < optThreads; ti++) {
		threads[ti].join();
		if (finishedRangeThread[ti])
			finishedRange = true;
	}
	ZAMAN_END_P(Parse);
	
	ZAMAN_START_P(Write);
	for (auto &v: records)
		for (auto &r: v) {
			printRecord(r, f);
			count++;
		}
	LOGN("\r\t%5.2lf%% [Chr %-10s]", (100.0 * inFile->tell()) / inFileSz, chr.substr(0, 10).c_str());
	ZAMAN_END_P(Write);
	
	ZAMAN_THREAD_JOIN();
	return count;
}

void FileDecompressor::loadIndex () 
{
	ZAMAN_START_P(LoadIndex);
	fileBlockCount.clear();
	bool firstRead = 1;
	indices.resize(fileNames.size());
	while (1) {
		//WARN("Here...%x",this->magic&0xff);
		index_t idx;
		string chr;
		
		size_t sz;
		if (firstRead) 
			firstRead = 0;
		else for (int i = 0; i < 8; i++) {
			gzread(idxFile, &sz, sizeof(size_t));
			idx.fieldData[i].resize(sz); 
			if (sz) gzread(idxFile, idx.fieldData[i].data(), sz); 
		}

		int16_t f = 0;
		if ((this->magic & 0xff) >= 0x11) {
			if (gzread(idxFile, &f, sizeof(int16_t)) != sizeof(int16_t)) 
				break;
			gzread(idxFile, &idx.zpos, sizeof(size_t));
		}
		else {
			if (gzread(idxFile, &idx.zpos, sizeof(size_t)) != sizeof(size_t))
				break;
		}
		fileBlockCount.push_back(f);
		
		gzread(idxFile, &idx.currentBlockCount, sizeof(size_t));
		char c; while (gzread(idxFile, &c, 1) && c) chr += c;
		gzread(idxFile, &idx.startPos, sizeof(size_t));
		gzread(idxFile, &idx.endPos, sizeof(size_t));
		gzread(idxFile, &idx.fS, sizeof(size_t));
		gzread(idxFile, &idx.fE, sizeof(size_t));

		//if (inMemory)
		indices[f][chr][idx.startPos] = idx;
	}
	ZAMAN_END_P(LoadIndex);
}

vector<range_t> FileDecompressor::getRanges (string range)
{
	if (range.size() && range[range.size() - 1] != ';') 
		range += ';';
	vector<range_t> ranges;

	size_t p;
	while ((p = range.find(';')) != string::npos) {
		string chr;
		size_t start, end;
		char *dup = strdup(range.substr(0, p).c_str());
		char *tok = strtok(dup, ":-");
		
		if (tok) {
			chr = tok, tok = strtok(0, ":-");
			if (tok) {
				start = atol(tok), tok = strtok(0, ":-");
				if (tok) 
					end = atol(tok), tok = strtok(0, ":-");
				else 
					end = -1;
			}
			else {
				start = 1; end = -1;
			}
		}
		else throw DZException("Range string %s invalid", range.substr(0, p - 1).c_str());
		if (end < start)
			swap(start, end);
		if (start) start--; 
		if (end) end--;

		range = range.substr(p + 1);
		int f = 0;
		if ((p = chr.find(',')) != string::npos) {
			f = atoi(chr.substr(0, p).c_str());
			chr = chr.substr(p + 1);
		}

		ranges.push_back(make_pair(make_pair(f, chr), make_pair(start, end)));
	}

	return ranges;
}

void FileDecompressor::decompress (int filterFlag) 
{
	ZAMAN_START_P(Decompress);
	for (int f = 0; f < comments.size(); f++)
		printComment(f);

	size_t blockSz = 0, 
		   totalSz = 0, 
		   blockCount = 0;
	while (blockCount < fileBlockCount.size() && (blockSz = getBlock(fileBlockCount[blockCount], "", 0, -1, filterFlag)) != 0) {
		totalSz += blockSz;
		blockCount++;
	}
	LOGN("\nDecompressed %'lu records, %'lu blocks\n", totalSz, blockCount);
	ZAMAN_END_P(Decompress);
}

inline bool intersect(size_t a, size_t b, size_t x, size_t y)
{
	return min(b, y) >= max(a, x);
}

void FileDecompressor::decompress (const string &range, int filterFlag)
{
	ZAMAN_START_P(Decompress);

	auto ranges = getRanges(range);

	size_t blockSz = 0, 
			totalSz = 0, 
			blockCount = 0;

	foreach (r, ranges) {
		int f = r->first.first;
		string chr = r->first.second;
		if (f < 0 || f >= fileNames.size())
			throw DZException("Invalid sample ID %d", f);
		if (indices[f].find(chr) == indices[f].end())
			throw DZException("Invalid chromosome %s for sample ID %d", chr.c_str(), f);

		auto idx = indices[f][chr];
		auto i = idx.upper_bound(r->second.first);
		if (i == idx.begin() || idx.size() == 0)
			throw DZException("Region %s:%d-%d not found for sample ID %d", 
				chr.c_str(), r->second.first, r->second.second, f);
		i--;
		
		// prepare reference
		foreach (j, idx) { // TODO speed up
			if (j == i) break;
			if (r->second.first >= j->second.fS && r->second.first <= j->second.fE) {
				inFile->seek(j->second.zpos);
				char chflag = inFile->readU8();
				while (chflag) chflag = inFile->readU8();
				while (chr != sequence[f]->getChromosome())
					sequence[f]->scanChromosome(chr, samComment[f]);

				Array<uint8_t> in;
				readBlock(in);
				sequence[f]->importRecords(in.data(), in.size());
			}
		}
		// set up field data
		while (intersect(i->second.startPos, i->second.endPos, r->second.first, r->second.second)) {	
			inFile->seek(i->second.zpos);
			shared_ptr<Decompressor> di[] = { 
				sequence[f], editOp[f], readName[f], mapFlag[f], 
				mapQual[f], quality[f], pairedEnd[f], optField[f] 
			};

			for (int ti = 0; ti < 8; ti++) if (i->second.fieldData[ti].size()) 
				di[ti]->setIndexData(i->second.fieldData[ti].data(), i->second.fieldData[ti].size());

			size_t blockSz = getBlock(f, chr, r->second.first, r->second.second, filterFlag);
			if (!blockSz) {
				break;
			}
			totalSz += blockSz;
			blockCount++;

			if (finishedRange) {
				finishedRange = false;
				break;
			}

			i++;
		}
	}
	ZAMAN_END_P(Decompress);
	LOGN("\nDecompressed %'lu records, %'lu blocks\n", totalSz, blockCount);
}

bool FileDecompressor::decompress2 (const string &range, int filterFlag, bool cont)
{
	static range_t r;
	static map<size_t, index_t> idx;
	static map<size_t, index_t>::iterator i;
	static string chr;
	static int f;

	if (!cont) {
		auto ranges = getRanges(range);
		if (ranges.size() != 1) 
			throw DZException("API supports only single range per invocation, %d provided", ranges.size());

		r = ranges[0];
		f = r.first.first;
		chr = r.first.second;
		if (f < 0 || f >= fileNames.size())
			throw DZException("Invalid sample ID %d", f);
		if (indices[f].find(chr) == indices[f].end())
			throw DZException("Invalid chromosome %s for sample ID %d", chr.c_str(), f);

		idx = indices[f][chr];
		i = idx.upper_bound(r.second.first);
		if (i == idx.begin() || idx.size() == 0)
			throw DZException("Region %s:%d-%d not found for sample ID %d", 
				chr.c_str(), r.second.first, r.second.second, f);
		i--;
		
		// prepare reference
		foreach (j, idx) { // TODO speed up
			if (j == i) break;
			if (r.second.first >= j->second.fS && r.second.first <= j->second.fE) {
				inFile->seek(j->second.zpos);
				char chflag = inFile->readU8();
				while (chflag) chflag = inFile->readU8();
				while (chr != sequence[f]->getChromosome())
					sequence[f]->scanChromosome(chr, samComment[f]);

				Array<uint8_t> in;
				readBlock(in);
				sequence[f]->importRecords(in.data(), in.size());
			}
		}
	}
	
	// set up field data
	if (intersect(i->second.startPos, i->second.endPos, r.second.first, r.second.second)) {		
		inFile->seek(i->second.zpos);
		shared_ptr<Decompressor> di[] = { 
			sequence[f], editOp[f], readName[f], mapFlag[f], 
			mapQual[f], quality[f], pairedEnd[f], optField[f] 
		};

		for (int ti = 0; ti < 8; ti++) if (i->second.fieldData[ti].size()) 
			di[ti]->setIndexData(i->second.fieldData[ti].data(), i->second.fieldData[ti].size());

		size_t blockSz = getBlock(f, chr, r.second.first, r.second.second, filterFlag);
		
		if (!blockSz) {
			return false;
		}
		
		if (finishedRange) {
			finishedRange = false;
			//return false;
		}
		i++;
		if (i == idx.end()) {
			return false;
		} 
		return true;
	} else {
		return false;
	}
}


inline void FileDecompressor::printRecord(const string &record, int file) 
{
	fputs(record.c_str(), samFiles[file]);
	fputc('\n', samFiles[file]);
}

inline void FileDecompressor::printComment(int file) 
{
    fputs(comments[file].c_str(), samFiles[file]);
}

/*
inline void FileDecompressor::printRecord(const string &rname, int flag, const string &chr, const EditOperation &eo, int mqual,
    const string &qual, const string &optional, const PairedEndInfo &pe, int file)
{
	fputs(rname.c_str(), samFiles[file]);
	fputc('\t', samFiles[file]);
	inttostr(flag, samFiles[file]);
	fputc('\t', samFiles[file]);
	fputs(chr.c_str(), samFiles[file]);
	fputc('\t', samFiles[file]);
	inttostr(eo.start, samFiles[file]);
	fputc('\t', samFiles[file]);
	inttostr(mqual, samFiles[file]);
	fputc('\t', samFiles[file]);
	fputs(eo.op.c_str(), samFiles[file]);
	fputc('\t', samFiles[file]);
	fputs(pe.chr.c_str(), samFiles[file]);
	fputc('\t', samFiles[file]);
	inttostr(pe.pos, samFiles[file]);
	fputc('\t', samFiles[file]);
	inttostr(pe.tlen, samFiles[file]);
	fputc('\t', samFiles[file]);
	fputs(eo.seq.c_str(), samFiles[file]);
	fputc('\t', samFiles[file]);
	fputs(qual.c_str(), samFiles[file]);
	if (optional.size()) {
		fputc('\t', samFiles[file]);
		fputs(optional.c_str(), samFiles[file]);
	}
	fputc('\n', samFiles[file]);
}
*/