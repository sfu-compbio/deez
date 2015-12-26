#include "Decompress.h"
#include <thread>
using namespace std;

void FileDecompressor::printStats (const string &path, int filterFlag) 
{
	auto inFile = File::Open(path.c_str(), "rb");

	if (inFile == NULL)
		throw DZException("Cannot open the file %s", path.c_str());

	size_t inFileSz = inFile->size();
	uint32_t magic = inFile->readU32();
	uint8_t optQuality = inFile->readU8();
	uint16_t numFiles = 1;
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

FileDecompressor::FileDecompressor (const string &inFilePath, const string &outFile, const string &genomeFile, int bs): 
	blockSize(bs), genomeFile(genomeFile), outFile(outFile), finishedRange(false)
{
	string name1 = inFilePath;
	this->inFile = File::Open(name1.c_str(), "rb");
	if (this->inFile == NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	inFileSz = inFile->size();
	magic = inFile->readU32();

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
		delete sequence[f];
		delete editOp[f];
		delete readName[f];
		delete mapFlag[f];
		delete mapQual[f];
		delete quality[f];
		delete pairedEnd[f];
		delete optField[f];
		if (samFiles[f]) fclose(samFiles[f]);
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

	uint16_t numFiles = 1;
	if ((magic & 0xff) >= 0x11) // DeeZ v1.1
		numFiles = inFile->readU16();

	for (int f = 0; f < numFiles; f++) {
		sequence.push_back(new SequenceDecompressor(genomeFile, blockSize));
		editOp.push_back(new EditOperationDecompressor(blockSize, (*sequence.back())));
		readName.push_back(new ReadNameDecompressor(blockSize));
		mapFlag.push_back(new MappingFlagDecompressor(blockSize));
		mapQual.push_back(new MappingQualityDecompressor(blockSize));
		pairedEnd.push_back(new PairedEndDecompressor(blockSize));
		optField.push_back(new OptionalFieldDecompressor(blockSize));
		quality.push_back(new QualityScoreDecompressor(blockSize));
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

void FileDecompressor::readBlock (Decompressor *d, Array<uint8_t> &in) 
{
	size_t sz = inFile->readU64();
	in.resize(sz);
	if (sz) inFile->read(in.data(), sz);
	d->importRecords(in.data(), in.size());
}

void readBlockThread (Decompressor *d, Array<uint8_t> &in) 
{
	d->importRecords(in.data(), in.size());
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
	ZAMAN_START_P(Seek);
	while (chr != sequence[f]->getChromosome())
		sequence[f]->scanChromosome(chr, samComment[f]);
	ZAMAN_END_P(Seek);

	ZAMAN_START_P(Blocks);
	Array<uint8_t> in[8];
	readBlock(sequence[f], in[7]);
	//sequence[f]->setFixed(*(editOp[f]));

	Decompressor *di[] = { editOp[f], readName[f], mapFlag[f], mapQual[f], quality[f], pairedEnd[f], optField[f] };
	thread t[7];
	for (int ti = 0; ti < 7; ti++) {
		size_t sz = inFile->readU64();
		in[ti].resize(sz);
		if (sz) inFile->read(in[ti].data(), sz); 
		t[ti] = thread(readBlockThread, di[ti], ref(in[ti]));
	}
	for (int ti = 0; ti < 7; ti++) 
		t[ti].join();
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
		//	LOG("%d %d / %d == %d %d %d %d %d %d %d", i, prevPos, readName[f]->getPaired(j-1), peo.start,peo.end,ppe.tlen,ppe.pos,eo.start,pe.tlen,pe.pos);
		}
	}
	ZAMAN_END_P(CheckMate);

	size_t count = 0;
		
	ZAMAN_START_P(Parse);
	while (editOp[f]->hasRecord()) {	
		int flag = mapFlag[f]->getRecord();
		if (filterFlag) {
			if (filterFlag > 0 && (flag & filterFlag) != filterFlag)
				continue;
			if (filterFlag < 0 && (flag & -filterFlag) == -filterFlag)
				continue;
		}
		EditOperation eo = editOp[f]->getRecord();		
		auto pe = pairedEnd[f]->getRecord(eo.start, eo.end - eo.start, flag & 0x10);

		if (chr != "*") eo.start++;
		if (pe.chr != "*") pe.pos++;

		if (eo.start < start)
			continue;
		if (eo.start > end) {
			finishedRange = true;
			ZAMAN_END_P(Parse);
			return count;
		}

		//LOG("%d %d %d", pe.pos, pe.tlen, pe.diff);

		string rname = readName[f]->getRecord();
		int mqual = mapQual[f]->getRecord();
		string qual = quality[f]->getRecord(eo.seq.size(), flag);
		string optional = optField[f]->getRecord(eo).data;
		
		printRecord(rname, flag, chr, eo, mqual, qual, optional, pe, f);
		if (count % (1 << 16) == 0) 
			LOGN("\r   Chr %-6s %5.2lf%%", chr.c_str(), (100.0 * inFile->tell()) / inFileSz);
		count++;
	}
	ZAMAN_END_P(Parse);
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

void FileDecompressor::decompress (const string &range, int filterFlag)
{
	auto ranges = getRanges(range);

	size_t 	blockSz = 0, 
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
				readBlock(sequence[f], in);
			}
		}
		// set up field data
		while (r->second.first >= i->second.startPos && r->second.first <= i->second.endPos) {		
			inFile->seek(i->second.zpos);
			Decompressor *di[] = { 
				sequence[f], editOp[f], readName[f], mapFlag[f], 
				mapQual[f], quality[f], pairedEnd[f], optField[f] 
			};

			for (int ti = 0; ti < 8; ti++) if (i->second.fieldData[ti].size()) 
				di[ti]->setIndexData(i->second.fieldData[ti].data(), i->second.fieldData[ti].size());

			size_t blockSz = getBlock(f, chr, r->second.first, r->second.second, filterFlag);
			if (!blockSz) break;
			totalSz += blockSz;
			blockCount++;

			if (finishedRange) {
				finishedRange = false;
				break;
			}

			i++;
		}
	}

	LOGN("\nDecompressed %'lu records, %'lu blocks\n", totalSz, blockCount);
}
