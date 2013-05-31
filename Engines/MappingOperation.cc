#include "MappingOperation.h"
using namespace std;

static const char *LOG_PREFIX = "<MO>";

MappingOperationCompressor::MappingOperationCompressor (const string &filename, int blockSize):
    lastLoc(0) {
	string name1(filename + ".mol.dz");
	lociFile = gzopen(name1.c_str(), "wb6");
	if (lociFile == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	name1 = filename + ".mom.dz";
	metadataFile = gzopen(name1.c_str(), "wb6");
	if (metadataFile == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	name1 = filename + ".mos.dz";
	stitchFile = gzopen(name1.c_str(), "wb6");
	if (stitchFile == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	records.reserve(blockSize);
	corrections.reserve(blockSize);
}

MappingOperationCompressor::~MappingOperationCompressor (void) {
	gzclose(lociFile);
	gzclose(metadataFile);
	gzclose(stitchFile);
}

void MappingOperationCompressor::addMetadata (const string &ref) {
	LOG("Adding Meta Data: %s", ref.c_str());
	references.push_back(ref);
	gzwrite(metadataFile, ref.c_str(), ref.size() + 1);
}

void MappingOperationCompressor::addRecord (uint32_t tmpLoc, const string &tmpRef) {
	if (tmpRef != lastRef) {
		Tuple t;
		t.first = lastLoc + 1;
		t.second = tmpLoc;
		t.ref = references.size() - 1;

		corrections.push_back(t);
		records.push_back(1);

		lastLoc = tmpLoc;
		lastRef = tmpRef;
	}
	else {
		if (tmpLoc - lastLoc > 250) {
			Tuple t;
			t.first = lastLoc + 1;
			t.second = tmpLoc;
			t.ref = references.size() - 1;

			corrections.push_back(t);
			records.push_back(1);

			lastLoc++;
		}
		else {
			records.push_back(tmpLoc - lastLoc);
		}
		lastLoc = tmpLoc;
	}
}

void MappingOperationCompressor::outputRecords (void) {
	if (records.size())
		gzwrite(lociFile, &records[0], records.size() * sizeof(records[0]));
	records.clear();

	if (corrections.size())
		gzwrite(stitchFile, &corrections[0], corrections.size() * sizeof(corrections[0]));
	corrections.clear();
}

MappingOperationDecompressor::MappingOperationDecompressor (const string &filename, int bs) 
    : blockSize (bs), recordCount (0), lastLoc(0) {
	string name1(filename + ".mol.dz");
	lociFile = gzopen(name1.c_str(), "rb");
	if (lociFile == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	name1 = filename + ".mom.dz";
	metadataFile = gzopen(name1.c_str(), "rb");
	if (metadataFile == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	name1 = filename + ".mos.dz";
	stitchFile = gzopen(name1.c_str(), "rb");
	if (stitchFile == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

    getMetaData();
	getNextCorrection();

	records.reserve(blockSize);
}

MappingOperationDecompressor::~MappingOperationDecompressor (void) {
	gzclose(lociFile);
	gzclose(metadataFile);
	gzclose(stitchFile);
}

void MappingOperationDecompressor::getNextCorrection (void) {
	size_t size = gzread(stitchFile, &lastCorrection, sizeof(Tuple));
	if (!size) {
		lastCorrection.first = 0;
		lastCorrection.ref = references.size() - 1;
		lastCorrection.second = 0;
	}
}

void MappingOperationDecompressor::getMetaData (void) {
	char *buffer = new char[blockSize];
	int size;
    while (size = gzread(metadataFile, buffer, blockSize)) {
		string tmp = "";
		int pos = 0;
		while (pos < size) {
			if (!buffer[pos]) {
				references.push_back(tmp);
				tmp = "";
			}
			else
				tmp += buffer[pos];
			pos++;
		}

		if (pos == size && buffer[pos - 1]) {
			char t = ' ';
			while (t) {
				gzread(metadataFile, &t, 1);
				tmp += t;
			}
			references.push_back(tmp);
		}
	}

	for (int i = 0; i < references.size(); i++)
		LOG("Read chromosome %s", references[i].c_str());

	delete[] buffer;
}

Locs MappingOperationDecompressor::getRecord (void) {
	if (drecords.size() == recordCount)	{
		drecords.clear();
		recordCount = 0;
		importRecords();
	}
	return drecords[recordCount++];
}

void MappingOperationDecompressor::importRecords (void) {
	records.resize(blockSize);
    size_t size = gzread(lociFile, &records[0], blockSize);
	if (size < blockSize)
		records.resize(size);

	for (int i = 0; i < records.size(); i++) {
		Locs n;
		if (lastCorrection.first == lastLoc + records[i]) {
			n.loc = lastCorrection.second;
			n.ref = references[lastCorrection.ref];
			lastLoc = lastCorrection.second;
			lastRef = references[lastCorrection.ref];
			getNextCorrection();
		}
		else {
			lastLoc += records[i];
			n.loc = lastLoc;
			n.ref = lastRef;
		}
		drecords.push_back(n);
	}

	records.clear();
	LOG("%d locations are loaded", drecords.size());
}
