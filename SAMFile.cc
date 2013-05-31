#include "SAMFile.h"
using namespace std;

static const char *LOG_PREFIX = "<SF>";

SAMFileCompressor::SAMFileCompressor (const string &outFile, const string &samFile, const string &genomeFile, int bs): 
	parser(samFile), 
	reference(outFile, genomeFile, bs),
	readName(outFile, bs),
	mappingFlag(outFile, bs),
	mappingOffset(outFile, bs),
	mappingQuality(outFile, bs),
	queryQual(outFile, bs), 
	pairedEnd(outFile, bs),
	blockSize(bs) 
{
	string name1(outFile + ".meta.dz");
	metaFile = fopen(name1.c_str(), "wb");
	if (metaFile == NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	mapinfo.reserve(blockSize + 1000);
}

SAMFileCompressor::~SAMFileCompressor (void) {
	fclose(metaFile);
}

void SAMFileCompressor::compress (void) {
	int lastFixedLocation = 0;
	int lastStart;
	int64_t total = 0;

	while (parser.hasNext()) {
		if (parser.head() != reference.getName()) { 
			while (reference.getName() != parser.head())
				reference.getNext();
			mappingOffset.addMetadata(reference.getName());
		}

		LOG("Loading records ...");
		int i;
		for (i = 0; i < blockSize && parser.hasNext() && parser.head() == reference.getName(); i++) {
			total++;
			const Record &rc = parser.next();
			
            EditOP e;
			e.start = rc.getMappingLocation();
			e.seq   = rc.getQuerySeq();
			e.op 	= rc.getMappingOperation();
			
            /*e.end   = e.start + reference.updateGenome(e.start - 1, e.seq, e.op);
			mapinfo.push_back(e);*/

            reference.addRecord(e);
			readName.addRecord(rc.getQueryName());
			mappingFlag.addRecord(rc.getMappingFlag());
			mappingOffset.addRecord(rc.getMappingLocation(), rc.getMappingReference());
			mappingQuality.addRecord(rc.getMappingQuality());
			queryQual.addRecord((rc.getMappingFlag() & 16) ? rc.getQueryQualRev() : rc.getQueryQual());
			pairedEnd.addRecord(reference.getChromosome(rc.getMateMappingReference()), rc.getMateMappingLocation(), rc.getTemplateLenght());

			lastStart = rc.getMappingLocation();
            parser.readNext();
		}
		LOG("%d records are loaded.", i);

		if (!parser.hasNext() || parser.head() != reference.getName()) {
			reference.fixGenome(lastFixedLocation, reference.getLength());
			lastFixedLocation = reference.getLength() - 1;
		}
		else if (parser.head() != "*") {
			reference.fixGenome(lastFixedLocation, lastStart - 2);
			lastFixedLocation = lastStart - 2;
		}
		else 
			lastFixedLocation = 1; 

		LOG("Writing to disk ...");
        int k = reference.outputRecords(lastFixedLocation);
		readName.outputRecords();
		mappingFlag.outputRecords();
		mappingOffset.outputRecords();
		mappingQuality.outputRecords();
		queryQual.outputRecords();
		pairedEnd.outputRecords();

        LOG("%d records are processed", k);
	}
	parser.getOFMeta();

	fwrite(&total, 1, sizeof(int64_t), metaFile);
}

SAMFileDecompressor::SAMFileDecompressor (const string &inFile, const string &outFile, const string &genomeFile, int bs): 
	reference(inFile, genomeFile, bs),
	readName(inFile, bs),
	mappingFlag(inFile, bs),
	mappingOffset(inFile, bs),
	mappingQuality(inFile, bs),
	editOperation(inFile, bs),
	queryQual(inFile, bs), 
	pairedEnd(inFile, bs),
	blockSize(bs)
{
	string name1(inFile + ".meta.dz");
	metaFile = fopen(name1.c_str(), "rb");
	if (metaFile == NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	samFile = fopen(outFile.c_str(), "wb");
	if (samFile == NULL)
		throw DZException("Cannot open the file %s", outFile.c_str());
}

SAMFileDecompressor::~SAMFileDecompressor (void) {
	fclose(metaFile);
	fclose(samFile);
}

void SAMFileDecompressor::decompress (void) {
	int64_t total;
	fread(&total, 1, sizeof(int64_t), metaFile);
	for (int64_t z = 0; z < total; z++) {
        Locs dtmp = mappingOffset.getRecord();
		while (dtmp.ref != reference.getName())
			reference.getNext();
		string  dname = readName.getRecord();
		short   dflag = mappingFlag.getRecord();
		uint8_t dmq   = mappingQuality.getRecord();
		EditOP  eo    = reference.getRecord(dtmp.loc - 1);
		string  dqual = queryQual.getRecord();
		
        string mateRef = reference.getChromosome(pairedEnd.getRecord());
        int mateLoc = pairedEnd.getRecord();
        int tl = pairedEnd.getRecord();
		/*if (tl & (1 << 31)) {
			mateRef = reference.getChromosome(tl & ~(1 << 31));
			mateLoc = pairedEnd.getRecord();
			tl = 0;
		}
		else { // not realy!
			if (tl & (1 << 30)) 
				tl = -(tl & ~(1 << 30));
			mateLoc = (int)dtmp.loc + tl - eo.end + eo.start - 2;
			mateRef = "=";
		}*/
        if (mateRef == dtmp.ref && dtmp.ref != "*") 
			mateRef = "=";

		fprintf(samFile, "%s\t%d\t%s\t%d\t%d\t%s\t%s\t%d\t%d\t%s\t%s\n",
				dname.c_str(),
				dflag,
				dtmp.ref.c_str(),
				dtmp.loc,
				dmq,
				eo.op.c_str(),
				mateRef.c_str(), 
				mateLoc,
				tl,
				eo.seq.c_str(),
				dqual.c_str()
		);
	}
}

