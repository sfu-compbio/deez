#include "SAMFile.h"
using namespace std;

SAMFileCompressor::SAMFileCompressor (const string &outFile, const string &samFile, const string &genomeFile, int bs): 
	parser(samFile), 
	reference(genomeFile, bs),
	readName(bs),
	mappingFlag(bs),
	mappingLocation(bs),
	mappingQuality(bs),
	queryQual(bs), 
	pairedEnd(bs),
	optionalField(bs),
	blockSize(bs) 
{
	outputFile = fopen(outFile.c_str(), "wb");
	if (outputFile == NULL)
		throw DZException("Cannot open the file %s", outFile.c_str());

	string idxFile = outFile + "i";
	indexFile = fopen(idxFile.c_str(), "wb");
	if (indexFile == NULL)
		throw DZException("Cannot open the file %s", idxFile.c_str());

#ifdef DZ_EVAL
	reference.debugStream 			= fopen((outFile + ".EditOperation").c_str(), "wb");
	readName.debugStream 			= fopen((outFile + ".ReadName").c_str(), "wb");
	mappingFlag.debugStream 		= fopen((outFile + ".MappingFlag").c_str(), "wb");
	mappingLocation.debugStream 	= fopen((outFile + ".MappingLocation").c_str(), "wb");
	mappingQuality.debugStream 		= fopen((outFile + ".MappingQuality").c_str(), "wb");
	queryQual.debugStream 			= fopen((outFile + ".QualityScore").c_str(), "wb");
	pairedEnd.debugStream 			= fopen((outFile + ".PairedEnd").c_str(), "wb");
	optionalField.debugStream 		= fopen((outFile + ".OptionalField").c_str(), "wb");
#endif
}

SAMFileCompressor::~SAMFileCompressor (void) {
	fclose(outputFile);
	fclose(indexFile);
}

void SAMFileCompressor::outputBlock (Compressor *c) {	
	static Array<uint8_t> _cmp;
	c->outputRecords(_cmp);
	uint8_t *out = _cmp.data();
	size_t out_sz = _cmp.size();

	fwrite(&out_sz, sizeof(size_t), 1, outputFile);
	#ifdef DZ_EVAL
		if(c->debugStream) fwrite(&out_sz, sizeof(size_t), 1, c->debugStream);
	#endif

	if (out_sz) {
		fwrite(out, sizeof(char), out_sz, outputFile);
		c->compressedCount += out_sz + sizeof(size_t);

		#ifdef DZ_EVAL
			if(c->debugStream) fwrite(out, sizeof(char), out_sz, c->debugStream);
		#endif
	}
}

void SAMFileCompressor::compress (void) {
	/* Structure:
	 	* magic[4]
	 	* comment[..\0]
		* chrEqual[1]
		* chrEqual=1? chr[..\0]
		* blockSz[8]
		* blockSz>0?  block[..blockSz] */

	uint32_t magic = MAGIC;
	fwrite(&magic, 4, 1, outputFile);

	// Comment!
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

	// SAM file
	size_t lastStart;
	int64_t total = 0;
	
	while (parser.hasNext()) {
		char op = 0;
		while (reference.getChromosome() != parser.head())
			reference.scanNextChromosome(), op = 1;

		// write chromosome id
		fwrite(&op, sizeof(char), 1, outputFile);
		if (op) 
			fwrite(parser.head().c_str(), parser.head().size() + 1, 1, outputFile);

		size_t startPos = -1;
		size_t endPos = -1;

		//LOG("Loading records ...");
		size_t i;
		for (i = 0; i < blockSize 
							//records.size() < blockSize
							&& parser.hasNext() 
							&& parser.head() == reference.getChromosome(); i++, total++) {
			const Record &rc = parser.next();

			size_t loc = rc.getLocation();
			if (loc == (size_t)-1) loc = 0; // fix for 0 locations in *

			reference.addRecord(
				loc, 
				rc.getSequence(), 
				rc.getCigar()
			);
			readName.addRecord(rc.getReadName());
			mappingFlag.addRecord(rc.getMappingFlag());
			mappingLocation.addRecord(loc, parser.head());
			mappingQuality.addRecord(rc.getMappingQuality());
			queryQual.addRecord(rc.getQuality(), rc.getMappingFlag());
			
			size_t p_loc = rc.getPairLocation();
			if (p_loc == (size_t)-1) p_loc = 0;
			pairedEnd.addRecord(
				rc.getPairChromosome(),
				p_loc, 
				rc.getTemplateLenght()
			);
			optionalField.addRecord(rc.getOptional());

			lastStart = loc;

			if (startPos == -1) startPos = loc;
			endPos = loc;

			parser.readNext();

			if (i % (1 << 16) == 0) 
				SCREEN("\r   Chr %-6s %5.2lf%%", parser.head().c_str(), (100.0 * parser.fpos()) / parser.fsize());
		}
		
		size_t zcnt, zes, zee;
		reference.applyFixes(!parser.hasNext() 
							|| parser.head() != reference.getChromosome() 
							|| parser.head() == "*" 
						? (size_t)-1 : lastStart - 1,
						zcnt, zes, zee);
		
		/// index
		size_t zpos = ftell(outputFile);
		fwrite(&zpos, sizeof(size_t), 1, indexFile);
		// cnt
		// ref
		fwrite(&zcnt, sizeof(size_t), 1, indexFile);
		// rest
		fwrite(&i, sizeof(size_t), 1, indexFile);
		// chr		
		fwrite(reference.getChromosome().c_str(), reference.getChromosome().size() + 1, 1, indexFile);
		// start pos		
		fwrite(&startPos, sizeof(size_t), 1, indexFile);
		// end pos
		// ref
		fwrite(&zes, sizeof(size_t), 1, indexFile);
		// fwrite(&zee, sizeof(size_t), 1, indexFile);
		// rest
		fwrite(&endPos, sizeof(size_t), 1, indexFile);

		//LOG("Writing to disk ...");
		// First ref fixes
		outputBlock(&reference);
		outputBlock(&readName);
		outputBlock(&mappingFlag);
		outputBlock(&mappingLocation);
		outputBlock(&mappingQuality);
		outputBlock(&queryQual);
		outputBlock(&pairedEnd);
		outputBlock(&optionalField);
	}
	SCREEN("\r");
	DEBUG("%s: %'15lu", "RN", readName.compressedCount);
	DEBUG("%s: %'15lu", "MF", mappingFlag.compressedCount);
	DEBUG("%s: %'15lu", "ML", mappingLocation.compressedCount);
	DEBUG("%s: %'15lu", "MQ", mappingQuality.compressedCount);
	DEBUG("%s: %'15lu", "PE", pairedEnd.compressedCount);
	DEBUG("%s: %'15lu", "SQ", reference.compressedCount);
	DEBUG("%s: %'15lu", "QQ", queryQual.compressedCount);
	DEBUG("%s: %'15lu", "OF", optionalField.compressedCount);
}


SAMFileDecompressor::SAMFileDecompressor (const string &inFile, const string &outFile, const string &genomeFile, int bs): 
	reference(genomeFile, bs),
	readName(bs),
	mappingFlag(bs),
	mappingLocation(bs),
	mappingQuality(bs),
	editOperation(bs),
	queryQual(bs), 
	pairedEnd(bs),
	optionalField(bs),
	blockSize(bs)
{
	string name1 = inFile;
	this->inFile = fopen(name1.c_str(), "rb");
	if (this->inFile == NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	name1 += "i";
	this->idxFile = fopen(name1.c_str(), "rb");
	if (this->idxFile == NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	samFile = fopen(outFile.c_str(), "wb");
	if (samFile == NULL)
		throw DZException("Cannot open the file %s", outFile.c_str());
}

SAMFileDecompressor::~SAMFileDecompressor (void) {
	fclose(samFile);
	fclose(inFile);
	fclose(idxFile);
}

bool SAMFileDecompressor::getSingleBlock (Array<uint8_t> &in) {
	size_t i;
	if (fread(&i, sizeof(size_t), 1, inFile) != 1)
		throw "aaargh";
	in.resize(i);
	if (i) fread(in.data(), sizeof(uint8_t), i, inFile);
	return true;
}

int SAMFileDecompressor::getBlock (string &chr) {
	static Array<uint8_t> in;
	char chflag;
	if (fread(&chflag, 1, 1, inFile) != 1)
		return -1;
	if (chflag) {
		chr = "";
		fread(&chflag, 1, 1, inFile);
		while (chflag) {
			chr += chflag;
			fread(&chflag, 1, 1, inFile);
		}
		while (chr != reference.getChromosome())
			reference.scanNextChromosome();
	}

	//DEBUG("chr %s\n", chr.c_str());

	getSingleBlock(in);
	reference.importRecords(in.data(), in.size());
	
	getSingleBlock(in);
	readName.importRecords(in.data(), in.size());

	getSingleBlock(in);
	mappingFlag.importRecords(in.data(), in.size());

	getSingleBlock(in);
	mappingLocation.importRecords(in.data(), in.size());

	getSingleBlock(in);
	mappingQuality.importRecords(in.data(), in.size());

	getSingleBlock(in);
	queryQual.importRecords(in.data(), in.size());

	getSingleBlock(in);
	pairedEnd.importRecords(in.data(), in.size());

	getSingleBlock(in);
	optionalField.importRecords(in.data(), in.size());

	return 0;
}

void SAMFileDecompressor::decompress (void) {
	uint32_t magic;
	fread(&magic, 4, 1, inFile);
	DEBUG("File format: %c%c v%d.%d", 
		(magic >> 16) & 0xff, 
		(magic >> 8) & 0xff, 
		(magic >> 4) & 0xf,
		magic & 0xf
	);

	// Comment!
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
		fwrite(arc.data(), 1, arc.size(), samFile);
	}

	// Rest of file
	string chr;
	while (getBlock(chr) != -1) {		
		while (1) {
			if (!reference.hasRecord() ||
				!mappingLocation.hasRecord() || 
				!readName.hasRecord() ||
				!mappingFlag.hasRecord() ||
				!mappingQuality.hasRecord() ||
				!queryQual.hasRecord() ||
				!pairedEnd.hasRecord()
			) break;

			uint32_t dtmp = mappingLocation.getRecord();
			EditOP eo = reference.getRecord(dtmp);
			if (chr != "*") dtmp++;
			PairedEndInfo mate = pairedEnd.getRecord();
			if (chr != "*") mate.pos++;

			int flag = mappingFlag.getRecord();

			fprintf(samFile, "%s\t%d\t%s\t%d\t%d\t%s\t%s\t%lu\t%d\t%s\t%s",
			 	readName.getRecord().c_str(),
			 	flag,
			 	reference.getChromosome().c_str(),
			 	dtmp,
			 	mappingQuality.getRecord(),
			 	eo.op.c_str(),
			 	mate.chr.c_str(),
			 	mate.pos,
			 	mate.tlen,
			 	eo.seq.c_str(),
			 	queryQual.getRecord(eo.seq.size(), flag).c_str()
			);

			string optional = optionalField.getRecord();
			if (optional.size())
				fprintf(samFile, "\t%s", optional.c_str());
			fprintf(samFile, "\n");
		}
	}
}


/*
void SAMFileDecompressor::decompress () {
	uint32_t magic;
	fread(&magic, 4, 1, inFile);
	DEBUG("File format: %c%c v%d.%d", 
		(magic >> 16) & 0xff, 
		(magic >> 8) & 0xff, 
		(magic >> 4) & 0xf,
		magic & 0xf
	);

	// Comment!
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
		fwrite(arc.data(), 1, arc.size(), samFile);
	}

	// Rest of file
	string chr;
	while (getBlock(chr) != -1) {		
		while (1) {
			if (!reference.hasRecord() ||
				!mappingLocation.hasRecord() || 
				!readName.hasRecord() ||
				!mappingFlag.hasRecord() ||
				!mappingQuality.hasRecord() ||
				!queryQual.hasRecord() ||
				!pairedEnd.hasRecord()
			) break;

			uint32_t dtmp = mappingLocation.getRecord();
			EditOP eo = reference.getRecord(dtmp);
			if (chr != "*") dtmp++;
			PairedEndInfo mate = pairedEnd.getRecord();
			if (chr != "*") mate.pos++;

			int flag = mappingFlag.getRecord();

			fprintf(samFile, "%s\t%d\t%s\t%d\t%d\t%s\t%s\t%lu\t%d\t%s\t%s",
			 	readName.getRecord().c_str(),
			 	flag,
			 	reference.getChromosome().c_str(),
			 	dtmp,
			 	mappingQuality.getRecord(),
			 	eo.op.c_str(),
			 	mate.chr.c_str(),
			 	mate.pos,
			 	mate.tlen,
			 	eo.seq.c_str(),
			 	queryQual.getRecord(eo.seq.size(), flag).c_str()
			);

			string optional = optionalField.getRecord();
			if (optional.size())
				fprintf(samFile, "\t%s", optional.c_str());
			fprintf(samFile, "\n");
		}
	}
}

*/