#include "Decompress.h"
#include <thread>
using namespace std;

static const char *NAMES[8] = {
	"SQ","RN","MF","ML", 
	"MQ","QQ","PE","OF" 
};

void FileDecompressor::printStats (const string &path, int filterFlag) {
	FILE *inFile = fopen(path.c_str(), "rb");
	if (inFile == NULL)
		throw DZException("Cannot open the file %s", path.c_str());

	fseek(inFile, 0L, SEEK_END);
	size_t inFileSz = ftell(inFile);

	// seek to index
	fseek(inFile, inFileSz - sizeof(size_t), SEEK_SET);
	size_t statPos;
	fread(&statPos, sizeof(size_t), 1, inFile);
	
	char magic[10] = {0};
	fseek(inFile, statPos, SEEK_SET);
	fread(magic, 1, 7, inFile);
	if (strcmp(magic, "DZSTATS"))
		throw DZException("Stats are corrupted ...%s", magic);

	size_t sz;
	fread(&sz, 1, 8, inFile);
	Array<uint8_t> in(sz);
	in.resize(sz);
	fread(in.data(), 1, sz, inFile);
	Stats *stats = new Stats(in);

	LOG("Index size %'lu bytes", inFileSz - statPos);
	LOG("%'16lu reads", stats->getReadCount());
	LOG("%'16lu mapped reads", stats->getStats(-4));
	LOG("%'16lu unmapped reads", stats->getStats(4));
	LOG("%'16lu chromosomes in reference file", stats->getChromosomeCount());
	if (filterFlag) {
		size_t p = stats->getStats(filterFlag);
		if (filterFlag > 0)
			LOG("%'16lu records with flag %d(0x%x)", p, filterFlag, filterFlag);
		else
			LOG("%'16lu records without flag %d(0x%x)", p, -filterFlag, -filterFlag);
	}
	else {
		for (int i = 0; i < Stats::FLAGCOUNT; i++) {
			size_t p = stats->getFlagCount(i);
			if (p) LOG("%4d 0x%04x: %'16lu", i, i, p);
		}
	}

	fclose(inFile);
}

FileDecompressor::FileDecompressor (const string &inFile, const string &outFile, const string &genomeFile, int bs): 
	blockSize(bs)
{
	sequence = new SequenceDecompressor(genomeFile, bs);
	editOp = new EditOperationDecompressor(bs);
	readName = new ReadNameDecompressor(bs);
	mapFlag = new MappingFlagDecompressor(bs);
	mapQual = new MappingQualityDecompressor(bs);
	pairedEnd = new PairedEndDecompressor(bs);
	optField = new OptionalFieldDecompressor(bs);

	string name1 = inFile;
	this->inFile = fopen(name1.c_str(), "rb");
	if (this->inFile == NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	fseek(this->inFile, 0L, SEEK_END);
	inFileSz = ftell(this->inFile);

	// seek to index

	if (fseek(this->inFile, inFileSz - sizeof(size_t), SEEK_SET))
		throw DZException("fseek failed ... %d", ferror(this->inFile));
	size_t statPos;
	if (fread(&statPos, sizeof(size_t), 1, this->inFile) != sizeof(size_t))
		throw DZException("fread failed ... %lu", statPos);
	
	char magic[10] = {0};
	if (fseek(this->inFile, statPos, SEEK_SET))
		throw DZException("fseek failed ... %d", ferror(this->inFile));

	if (fread(magic, 1, 7, this->inFile) != 7)
		throw DZException("MAGIC fread failed");	
	if (strcmp(magic, "DZSTATS"))
		throw DZException("Stats are corrupted ...%c%c%c%c%c%c%c", magic[0], magic[1], magic[2], magic[3], magic[4], magic[5], magic[6]);

	size_t sz;
	fread(&sz, 1, 8, this->inFile);
	Array<uint8_t> in(sz);
	in.resize(sz);
	fread(in.data(), 1, sz, this->inFile);
	stats = new Stats(in);

	magic[5] = 0;
	fread(magic, 1, 5, this->inFile);
	if (strcmp(magic, "DZIDX"))
		throw DZException("Index is corrupted ...%s", magic);

	size_t idxToRead = inFileSz - ftell(this->inFile) - sizeof(size_t);
	FILE *tmp = tmpfile();
	char *buffer = (char*)malloc(MB);
	while (idxToRead && (sz = fread(buffer, 1, min((size_t)MB, idxToRead), this->inFile))) {
		fwrite(buffer, 1, sz, tmp);
		idxToRead -= sz;
	}
	free(buffer);
	
	int idx = dup(fileno(tmp)); //  dup(fileno(this->inFile));
	lseek(idx, 0/*sz + 5*/, SEEK_SET); // needed for gzdopen
	idxFile = gzdopen(idx, "rb");
	if (idxFile == Z_NULL)
		throw DZException("Cannot open the index");

	//gzread(idxFile, magic, 5);
	//LOG("%s", magic);
	
	fseek(this->inFile, 0L, SEEK_SET);

	if (optStdout)
		samFile = stdout;
	else {
		samFile = fopen(outFile.c_str(), "wb");
		if (samFile == NULL)
			throw DZException("Cannot open the file %s", outFile.c_str());
	}
}

FileDecompressor::~FileDecompressor (void) {
	delete sequence;
	delete editOp;
	delete readName;
	delete mapFlag;
	delete mapQual;
	delete quality;
	delete pairedEnd;
	delete optField;
	if (idxFile) gzclose(idxFile);
	if (samFile) fclose(samFile);
	if (inFile) fclose(inFile);
}

void FileDecompressor::decompress (int filterFlag) {
	getMagic();
	getComment(true);
	size_t blockSz = 0, 
		   totalSz = 0, 
		   blockCount = 0;
	while ((blockSz = getBlock("", 0, -1, filterFlag)) != 0) {
		totalSz += blockSz;
		blockCount++;
	}
	LOGN("\nDecompressed %'lu records, %'lu blocks\n", totalSz, blockCount);
}

void FileDecompressor::getMagic (void) {
	uint32_t magic;
	fread(&magic, 4, 1, inFile);
	LOG("File format: %c%c v%d.%d", 
		(magic >> 16) & 0xff, 
		(magic >> 8) & 0xff, 
		(magic >> 4) & 0xf,
		magic & 0xf
	);
	fread(&optQuality, 1, 1, inFile);
	quality = new QualityScoreDecompressor(blockSize);
}

void FileDecompressor::getComment (bool output) {
	size_t arcsz;
	fread(&arcsz, sizeof(size_t), 1, inFile);
	if (arcsz) {
		Array<uint8_t> arc;
		arc.resize(arcsz);
		fread(&arcsz, sizeof(size_t), 1, inFile);
		Array<uint8_t> comment;
		comment.resize(arcsz);
		fread(comment.data(), 1, arcsz, inFile);

		GzipDecompressionStream gzc;
		gzc.decompress(comment.data(), comment.size(), arc, 0);

		if (output)
			fwrite(arc.data(), 1, arc.size(), samFile);
	}
}

void FileDecompressor::readBlock (Decompressor *d, Array<uint8_t> &in) {
	size_t sz;
	if (fread(&sz, sizeof(size_t), 1, inFile) != 1)
		throw "File reading failed";
	in.resize(sz);
	if (sz) 
		fread(in.data(), 1, sz, inFile);
	d->importRecords(in.data(), in.size());
}

void readBlockThread (Decompressor *d, Array<uint8_t> &in) {
	d->importRecords(in.data(), in.size());
}

size_t FileDecompressor::getBlock (const string &chromosome, 
	size_t start, size_t end, int filterFlag) 
{
	static string chr;
	static bool done(false);
	if (done)
		return 0;
	if (chromosome != "")
		chr = chromosome;

	char chflag;
	//LOG("> %lu",ftell(inFile));
	// EOF case
	if (fread(&chflag, 1, 1, inFile) != 1)
		return 0;
	if (chflag > 1) // index!
		return 0;
	if (chflag) {
		chr = "";
		fread(&chflag, 1, 1, inFile);
		while (chflag)
			chr += chflag, fread(&chflag, 1, 1, inFile);
		if (chromosome != "" && chr != chromosome)
			return 0;
	}

	while (chr != sequence->getChromosome())
		sequence->scanChromosome(chr);

//	__DC=0; 
	Array<uint8_t> in[8];
	readBlock(sequence, in[7]);
	sequence->setFixed(*editOp);

	Decompressor *di[] = { editOp, readName, mapFlag, mapQual, quality, pairedEnd, optField };
	thread t[7];
	for (int ti = 0; ti < 7; ti++) {
		size_t sz;
		if (fread(&sz, sizeof(size_t), 1, inFile) != 1)
			throw "File reading failed";
		in[ti].resize(sz);
		if (sz) 
			fread(in[ti].data(), 1, sz, inFile);
		t[ti] = thread(readBlockThread, di[ti], ref(in[ti]));
	}
	for (int ti = 0; ti < 7; ti++)
		t[ti].join();

	// readBlock(editOp, in);  //05
	// readBlock(readName, in); //67
	// readBlock(mapFlag, in);  //8
	// readBlock(mapQual, in);  //9
	// readBlock(quality, in);  //10
	// readBlock(pairedEnd, in); // 11
	// readBlock(optField, in); // 12

	size_t count = 0;
	while (editOp->hasRecord()) {
		string rname = readName->getRecord();
		int flag = mapFlag->getRecord();
		EditOperation eo = editOp->getRecord();
		int mqual = mapQual->getRecord();
		string qual = quality->getRecord(eo.seq.size(), flag);
		string optional = optField->getRecord();
		PairedEndInfo pe = pairedEnd->getRecord(chr, eo.start);

		if (filterFlag) {
			if (filterFlag > 0 && (flag & filterFlag) != filterFlag)
				continue;
			if (filterFlag < 0 && (flag & -filterFlag) == -filterFlag)
				continue;
		}
		if (eo.start < start)
			continue;
		if (eo.start > end) {
			done = true;
			return count;
		}

		if (chr != "*") eo.start++;
		if (pe.chr != "*") pe.pos++;

		fprintf(samFile, "%s\t%d\t%s\t%d\t%d\t%s\t%s\t%lu\t%d\t%s\t%s",
		 	rname.c_str(),
		 	flag,
		 	chr.c_str(),
		 	eo.start,
		 	mqual,
		 	eo.op.c_str(),
		 	pe.chr.c_str(),
		 	pe.pos,
		 	pe.tlen,
		 	eo.seq.c_str(),
		 	qual.c_str()
		);
		if (optional.size())
			fprintf(samFile, "\t%s", optional.c_str());
		fprintf(samFile, "\n");

		if (count % (1 << 16) == 0) 
			LOGN("\r   Chr %-6s %5.2lf%%", chr.c_str(), (100.0 * ftell(inFile)) / inFileSz);
		count++;
	}
	return count;
}

void FileDecompressor::decompress (const string &idxFilePath, 
	const string &range, int filterFlag) 
{
	getMagic();
	getComment(false);

	string chr;
	size_t start, end;
	char *dup = strdup(range.c_str());
	char *tok = strtok(dup, ":-");
	
	if (tok) 
		chr = tok, tok = strtok(0, ":-");
	else 
		throw DZException("Range string %s invalid", range.c_str());
	if (tok) 
		start = atol(tok), tok = strtok(0, ":-");
	else 
		throw DZException("Range string %s invalid", range.c_str());
	if (tok) 
		end = atol(tok), tok = strtok(0, ":-");
	else 
		throw DZException("Range string %s invalid", range.c_str());
	if (end < start)
		swap(start, end);
	LOG("Seeking to chromosome %s, [%'lu:%'lu]...", chr.c_str(), start, end);
	start--; end--;

	// read index, detect
	Array<uint8_t> ei[8];
	bool firstRead = 1;
	string prevChr;
	int _bC_=1;
	while (1) {
		size_t zpos, currentBlockCount, startPos, endPos;
		char chrx[100];
		char *c = chrx;

		if (firstRead) firstRead = 0;
		else for (int i = 0; i < 8; i++) {
			size_t x;
			gzread(idxFile, &x, sizeof(size_t));
			ei[i].resize(x+2);
			//LOGN("%'lu ", x);
			if (x) gzread(idxFile, ei[i].data(), x); //LOGN("<%'lu>", gzread(idxFile, ei[i].data(), x));
			//LOGN("%d | ",gzeof(idxFile));
		}

		if (gzread(idxFile, &zpos, sizeof(size_t)) != sizeof(size_t)) 
			throw DZException("Requested range %s not found", range.c_str());
		gzread(idxFile, &currentBlockCount, sizeof(size_t));
		//LOGN("ZP %'lu ", zpos);
		//LOGN("BC %'lu ", currentBlockCount);
		while (gzread(idxFile, c, 1) && *c++);
		gzread(idxFile, &startPos, sizeof(size_t));
		//LOGN("SP %'lu ", startPos);
		gzread(idxFile, &endPos, sizeof(size_t));
		//LOGN("EP %'lu ", endPos);

		size_t fS, fE;
		gzread(idxFile, &fS, sizeof(size_t));
		gzread(idxFile, &fE, sizeof(size_t));

		if (prevChr != string(chrx)) {
			startPos = 0;
			fS = 0;
			prevChr = string(chrx);
		}

		//LOG(" ... %s:%'lu-%'lu ... fixes %'lu-%'lu", chrx, startPos, endPos, fS, fE);

		if (string(chrx) == chr && (start >= startPos && start <= endPos)) {		
			fseek(inFile, zpos, SEEK_SET);
			Decompressor *di[] = { sequence, editOp, readName, mapFlag, mapQual, quality, pairedEnd, optField };
			for (int ti = 0; ti < 8; ti++)
				if (ei[ti].size()) di[ti]->setIndexData(ei[ti].data(), ei[ti].size());
			break;
		}
		else if (string(chrx) == chr && (start >= fS && start <= fE)) {
			fseek(inFile, zpos, SEEK_SET);

			// TODO do import fixes only ...
			char chflag;
			fread(&chflag, 1, 1, inFile);
			while (chflag) fread(&chflag, 1, 1, inFile);

			while (chr != sequence->getChromosome())
				sequence->scanChromosome(chr);

			Array<uint8_t> in;
			readBlock(sequence, in);
		}
	}

	size_t 	blockSz = 0, 
			totalSz = 0, 
			blockCount = 0;
	while ((blockSz = getBlock(chr, start, end, filterFlag)) != 0) {
		totalSz += blockSz;
		blockCount++;
	}
	LOGN("\nDecompressed %'lu records, %'lu blocks\n", totalSz, blockCount);
}
