#include "Decompress.h"
using namespace std;

static const char *NAMES[8] = {
	"SQ","RN","MF","ML", 
	"MQ","QQ","PE","OF" 
};

FileDecompressor::FileDecompressor (const string &inFile, const string &outFile, const string &genomeFile, int bs): 
	blockSize(bs)
{
	decompressor[Sequence] = new SequenceDecompressor(genomeFile, bs);
	decompressor[ReadName] = new ReadNameDecompressor(bs);
	decompressor[MappingFlag] = new MappingFlagDecompressor(bs);
	decompressor[MappingLocation] = new MappingLocationDecompressor(bs);
	decompressor[MappingQuality] = new MappingQualityDecompressor(bs);
	decompressor[QualityScore] = new QualityScoreDecompressor(bs);
	decompressor[PairedEnd] = new PairedEndDecompressor(bs);
	decompressor[OptionalField] = new OptionalFieldDecompressor(bs);

	string name1 = inFile;
	this->inFile = fopen(name1.c_str(), "rb");
	if (this->inFile == NULL)
		throw DZException("Cannot open the file %s", name1.c_str());
	fseek(this->inFile, 0L, SEEK_END);
	inFileSz = ftell(this->inFile);
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
	for (int i = 0; i < 8; i++)
		delete decompressor[i];
	fclose(samFile);
	fclose(inFile);
}

void FileDecompressor::decompress (void) {
	getMagic();
	getComment(true);
	size_t 	blockSz = 0, 
			totalSz = 0, 
			blockCount = 0;
	while ((blockSz = getBlock("", 0, -1)) != 0) {
		totalSz += blockSz;
		blockCount++;
	}
	SCREEN("\rDecompressed %'lu records, %'lu blocks\n", totalSz, blockCount);
}

void FileDecompressor::getMagic (void) {
	uint32_t magic;
	fread(&magic, 4, 1, inFile);
	DEBUG("File format: %c%c v%d.%d", 
		(magic >> 16) & 0xff, 
		(magic >> 8) & 0xff, 
		(magic >> 4) & 0xf,
		magic & 0xf
	);
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

size_t FileDecompressor::getBlock (const string &chromosome, size_t start, size_t end) {
	static string chr;
	if (chromosome != "")
		chr = chromosome;

	char chflag;
	// EOF case
	if (fread(&chflag, 1, 1, inFile) != 1)
		return 0;
	if (chflag) {
		chr = "";
		fread(&chflag, 1, 1, inFile);
		while (chflag)
			chr += chflag, fread(&chflag, 1, 1, inFile);
		if (chromosome != "" && chr != chromosome)
			return 0;
	}

	SequenceDecompressor *seq = (SequenceDecompressor*)decompressor[Sequence];
	while (chr != seq->getChromosome())
		seq->scanChromosome(chr);

	Array<uint8_t> in;
	SCREEN("\n");
	for (int i = 0; i < 8; i++) {
		fprintf(stderr,"%s~ ", NAMES[i]);
	ZAMAN_START();
		size_t sz;
		if (fread(&sz, sizeof(size_t), 1, inFile) != 1)
			throw "aaargh";
		in.resize(sz);
		if (sz) 
			fread(in.data(), 1, sz, inFile);
		decompressor[i]->importRecords(in.data(), in.size());		
	ZAMAN_END(NAMES[i]);
	}
	SCREEN("\n");	

	size_t count = 0;
	while (seq->hasRecord()) {
		string rname = ((ReadNameDecompressor*)decompressor[ReadName])->getRecord();
		int flag = ((MappingFlagDecompressor*)decompressor[MappingFlag])->getRecord();
		uint32_t loc = ((MappingLocationDecompressor*)decompressor[MappingLocation])->getRecord();

		int mqual = ((MappingQualityDecompressor*)decompressor[MappingQuality])->getRecord();
		EditOP eo = seq->getRecord(loc);
		if (chr != "*") loc++;

		PairedEndInfo pe = ((PairedEndDecompressor*)decompressor[PairedEnd])->getRecord();
		if (chr != "*") pe.pos++;
		string qual = ((QualityScoreDecompressor*)decompressor[QualityScore])->getRecord(eo.seq.size(), flag);
		string optional = ((OptionalFieldDecompressor*)decompressor[OptionalField])->getRecord();

		if (loc < start)
			continue;
		if (loc > end)
			return count;

		fprintf(samFile, "%s\t%d\t%s\t%d\t%d\t%s\t%s\t%lu\t%d\t%s\t%s",
		 	rname.c_str(),
		 	flag,
		 	chr.c_str(),
		 	loc,
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
			SCREEN("\r   Chr %-6s %5.2lf%%", 
				chr.c_str(), (100.0 * ftell(inFile)) / inFileSz);
		count++;
	}
	//fprintf(samFile, ">>>>>>>>>>>>>>>>>>>>\n");
	//SCREEN("\n");
	return count;
}

void FileDecompressor::decompress (const string &idxFilePath, const string &range) {
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
	SCREEN("Seeking to chromosome %s, [%'lu:%'lu]...\n", chr.c_str(), start, end);
	start--; end--;

	// read index, detect
	gzFile idxFile = gzopen(idxFilePath.c_str(), "rb");
	if (idxFile == NULL)
		throw DZException("Cannot open the file %s", idxFilePath.c_str());
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
			ei[i].resize(x);
			if(x) {
				gzread(idxFile, ei[i].data(), x);
				// gzseek(idxFile, gztell(idxFile) + x, SEEK_SET);
			}
		}

		DEBUG("~~ %d",_bC_++);
		if (gzread(idxFile, &zpos, sizeof(size_t)) != sizeof(size_t)) 
			throw DZException("Requested range %s not found", range.c_str());
		gzread(idxFile, &currentBlockCount, sizeof(size_t));
		while (gzread(idxFile, c, 1) && *c++);
		gzread(idxFile, &startPos, sizeof(size_t));
		gzread(idxFile, &endPos, sizeof(size_t));

		size_t fS, fE;
		gzread(idxFile, &fS, sizeof(size_t));
		gzread(idxFile, &fE, sizeof(size_t));

		if (prevChr != string(chrx)) {
			startPos = 0;
			fS = 0;
			prevChr = string(chrx);
		}

		SCREEN("%s:%'lu-%'lu ... fixes %'lu-%'lu\n", chrx, startPos, endPos, fS, fE);

		if (string(chrx) == chr && (start >= startPos && start <= endPos)) {
			
			fseek(inFile, zpos, SEEK_SET);
			SCREEN("In...\n");
			for (int i = 0; i < 8; i++) 
				if (ei[i].size())
					decompressor[i]->setIndexData(ei[i].data(), ei[i].size());
			break;
		}
		else if (string(chrx) == chr && (start >= fS && start <= fE)) {
			fseek(inFile, zpos, SEEK_SET);

			// TODO do import fixes only ...
			char chflag;
			fread(&chflag, 1, 1, inFile);
			while (chflag) fread(&chflag, 1, 1, inFile);

			SequenceDecompressor *seq = (SequenceDecompressor*)decompressor[Sequence];
			while (chr != seq->getChromosome())
				seq->scanChromosome(chr);

			Array<uint8_t> in;
			size_t sz;
			fread(&sz, sizeof(size_t), 1, inFile);
			in.resize(sz);
			if (sz) fread(in.data(), 1, sz, inFile);
			SCREEN("Loading...\n");
			seq->importFixes(in.data(), in.size());	
		}
	}
	gzclose(idxFile);

	size_t 	blockSz = 0, 
			totalSz = 0, 
			blockCount = 0;
	while ((blockSz = getBlock(chr, start, end)) != 0) {
		totalSz += blockSz;
		blockCount++;
	}
	SCREEN("\rDecompressed %'lu records, %'lu blocks\n", totalSz, blockCount);
}
