#include "SAMFile.h"
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

enum Fields {
	Sequence, 
	ReadName, 
	MappingFlag, 
	MappingLocation,
	MappingQuality, 
	QualityScore, 
	PairedEnd, 
	OptionalField
};

SAMFileCompressor::SAMFileCompressor (const string &outFile, const string &samFile, const string &genomeFile, int bs): 
	parser(samFile), 
	blockSize(bs) 
{
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

SAMFileCompressor::~SAMFileCompressor (void) {
	for (int i = 0; i < 8; i++)
		delete compressor[i];
	fclose(outputFile);
	gzclose(indexFile);
}

void SAMFileCompressor::outputMagic (void) {
	uint32_t magic = MAGIC;
	fwrite(&magic, 4, 1, outputFile);
}

void SAMFileCompressor::outputComment (void) {
	string comment;
	char *cm;
	while ((cm = parser.readComment()) != 0)
		comment += cm;
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

void SAMFileCompressor::outputRecords (void) {
	// SAM file
	size_t lastStart;
	int64_t total = 0;
	int64_t blockCount = 0;
	Array<uint8_t> outputBuffer;

	SequenceCompressor* seq = (SequenceCompressor*)(compressor[Sequence]);
	
	while (parser.hasNext()) {
		char op = 0;
		while (seq->getChromosome() != parser.head())
			seq->scanNextChromosome(), op = 1;

		/// index
		size_t zpos = ftell(outputFile);
		gzwrite(indexFile, &zpos, sizeof(size_t));

		// write chromosome id
		fwrite(&op, sizeof(char), 1, outputFile);
		if (op) 
			fwrite(parser.head().c_str(), parser.head().size() + 1, 1, outputFile);

		//LOG("Loading records ...");
		for (; seq->size() < blockSize
							&& parser.hasNext() 
							&& parser.head() == seq->getChromosome(); total++) {
			const Record &rc = parser.next();

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
			((QualityScoreCompressor*)compressor[QualityScore])->addRecord(rc.getQuality(), rc.getMappingFlag());
			
			size_t p_loc = rc.getPairLocation();
			if (p_loc == (size_t)-1) p_loc = 0;
			((PairedEndCompressor*)compressor[PairedEnd])->addRecord(PairedEndInfo(
				rc.getPairChromosome(),
				p_loc, 
				rc.getTemplateLenght()
			));
			((OptionalFieldCompressor*)compressor[OptionalField])->addRecord(rc.getOptional());

			lastStart = loc;
			parser.readNext();		
		}
		SCREEN("\r   Chr %-6s %5.2lf%% [%ld]", parser.head().c_str(), (100.0 * parser.fpos()) / parser.fsize(), blockCount + 1);		
		
		size_t 	currentBlockCount, 
				currentBlockFirstLoc, 
				currentBlockLastLoc, 
				currentBlockLastEndLoc;
		//ZAMAN_START();
		currentBlockCount = seq->applyFixes(!parser.hasNext() 
							|| parser.head() != seq->getChromosome() 
							|| parser.head() == "*" 
						? (size_t)-1 : lastStart - 1, 
						currentBlockFirstLoc, currentBlockLastLoc, currentBlockLastEndLoc);	
		SCREEN("\t%s:%'lu-%'lu\t%'lu\n", seq->getChromosome().c_str(), 
			currentBlockFirstLoc+1, currentBlockLastLoc+1, currentBlockCount);
		// cnt
		gzwrite(indexFile, &currentBlockCount, sizeof(size_t));
		// chr		
		gzwrite(indexFile, seq->getChromosome().c_str(), seq->getChromosome().size() + 1);
		// start pos		
		gzwrite(indexFile, &currentBlockFirstLoc, sizeof(size_t));
		// end pos
		gzwrite(indexFile, &currentBlockLastLoc, sizeof(size_t));
		//ZAMAN_END("FIX");
		for (int i = 0; i < 8; i++) {
		//ZAMAN_START();
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
		//ZAMAN_END(NAMES[i]);
		}
		blockCount++;
	}
	SCREEN("\nWritten %'lu lines\n", total);
	for (int i = 0; i < 8; i++)
		DEBUG("%s: %'15lu", NAMES[i], compressor[i]->compressedCount);
}

void SAMFileCompressor::compress (void) {
	outputMagic();
	outputComment();
	outputRecords();
}

SAMFileDecompressor::SAMFileDecompressor (const string &inFile, const string &outFile, const string &genomeFile, int bs): 
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

SAMFileDecompressor::~SAMFileDecompressor (void) {
	for (int i = 0; i < 8; i++)
		delete decompressor[i];
	fclose(samFile);
	fclose(inFile);
}

void SAMFileDecompressor::getMagic (void) {
	uint32_t magic;
	fread(&magic, 4, 1, inFile);
	DEBUG("File format: %c%c v%d.%d", 
		(magic >> 16) & 0xff, 
		(magic >> 8) & 0xff, 
		(magic >> 4) & 0xf,
		magic & 0xf
	);
}

void SAMFileDecompressor::getComment (bool output) {
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

size_t SAMFileDecompressor::getBlock (const string &chromosome, size_t start, size_t end) {
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
		seq->scanNextChromosome();

	Array<uint8_t> in;
	for (int i = 0; i < 8; i++) {
	//ZAMAN_START();
		size_t sz;
		if (fread(&sz, sizeof(size_t), 1, inFile) != 1)
			throw "aaargh";
		in.resize(sz);
		if (sz) 
			fread(in.data(), 1, sz, inFile);
		if(i!=QualityScore) decompressor[i]->importRecords(in.data(), in.size());		
	//ZAMAN_END(NAMES[i]);
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
		string qual = "";//((QualityScoreDecompressor*)decompressor[QualityScore])->getRecord(eo.seq.size(), flag);
		string optional = ((OptionalFieldDecompressor*)decompressor[OptionalField])->getRecord();

	//	if (loc < start)
	//		continue;
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
	fprintf(samFile, ">>>>>>>>>>>>>>>>>>>>\n");
	//SCREEN("\n");
	return count;
}

void SAMFileDecompressor::decompress (void) {
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

void SAMFileDecompressor::decompress (const string &idxFilePath, const string &range) {
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
	DEBUG("Seeking to chromosome %s, [%'lu:%'lu]...", chr.c_str(), start, end);
	start--; end--;

	// read index, detect
	gzFile idxFile = gzopen(idxFilePath.c_str(), "rb");
	if (idxFile == NULL)
		throw DZException("Cannot open the file %s", idxFilePath.c_str());
	Array<uint8_t> ei[8];
	bool firstRead = 1;
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

		if (gzread(idxFile, &zpos, sizeof(size_t)) != sizeof(size_t)) 
			throw DZException("Requested range %s not found", range.c_str());
		gzread(idxFile, &currentBlockCount, sizeof(size_t));
		while (gzread(idxFile, c, 1) && *c++);
		gzread(idxFile, &startPos, sizeof(size_t));
		gzread(idxFile, &endPos, sizeof(size_t));
		DEBUG("%s:%'lu-%'lu ...", chrx, startPos, endPos);
		if (string(chrx) == chr && 
			(start >= startPos && start <= endPos) ||
			(start < startPos && end >= startPos) )
		{
			fseek(inFile, zpos, SEEK_SET);
			for (int i = 0; i < 8; i++) 
				if (ei[i].size())
					decompressor[i]->setIndexData(ei[i].data(), ei[i].size());
			break;
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
