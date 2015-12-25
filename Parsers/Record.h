
#ifndef Record_H
#define Record_H

#include <string>
#include <cstdlib>
#include "../Common.h"

class Record {
	char *line;
	size_t lineLength, lineSize;
	array<int32_t, 7> strFields;
	array<int32_t, 5> intFields;

private:
	friend class BAMParser;
	friend class SAMParser;

public:
	enum StringField {
		RN,
		CHR,
		CIGAR,
		P_CHR,
		SEQ,
		QUAL,
		OPT
	};
	enum IntField {
		MF,
		LOC,
		MQ,
		P_LOC,
		TLEN
	};

public:
	Record(): line(0), lineLength(0), lineSize(0)
	{
	}
	~Record (void)
	{
		if (line) {
			free(line);
			line = 0;
		}
	}
	Record (const Record& a)
	{
		ZAMAN_START(Record_Copy);
	   // LOG("Copying record of size %d %d", a.lineLength, a.lineSize);

		strFields = a.strFields;
		intFields = a.intFields;
		lineLength = a.lineLength;
		lineSize = a.lineSize;
		line = (char*)malloc(a.lineSize + 1);
		std::copy(a.line, a.line + a.lineSize, line);
		line[lineSize] = 0;

		ZAMAN_END(Record_Copy);
	}

	Record(Record&& a): Record()
	{
		//LOG("Move called");
		swap(*this, a);
	}

	Record& operator= (Record a) 
	{
		swap(*this, a);
		return *this;
	}

	friend void swap(Record& a, Record& b) // nothrow
	{
		using std::swap;

		swap(a.strFields, b.strFields);
		swap(a.intFields, b.intFields);
		swap(a.lineLength, b.lineLength);
		swap(a.lineSize, b.lineSize);
		swap(a.line, b.line);
	}


public:
	const char* getReadName() const { return &line[0] + strFields[RN]; }
	const int getMappingFlag() const { return intFields[MF]; }
	const char* getChromosome() const { return &line[0] + strFields[CHR]; }
	const size_t getLocation() const { return intFields[LOC]; }
	const char getMappingQuality() const { return intFields[MQ]; }
	const char* getCigar() const { return &line[0] + strFields[CIGAR]; }
	const char* getPairChromosome() const { return &line[0] + strFields[P_CHR]; }
	const size_t getPairLocation() const { return intFields[P_LOC]; }
	const int getTemplateLenght() const { return intFields[TLEN]; }
	const char* getSequence() const { return &line[0] + strFields[SEQ]; }
	const char* getQuality() const { return &line[0] + strFields[QUAL]; }
	const char* getOptional() const { return &line[0] + strFields[OPT]; }

	size_t getReadNameSize() const { return strlen(getReadName()); }
	size_t getSequenceSize() const { return strFields[QUAL] - strFields[SEQ] - 1; }
	size_t getOptionalSize() const { return lineLength - strFields[OPT]; }


	std::string getFullRecord() const {
		return S(
			"%s %d %s %zu %d %s %s %zu %d %s %s %s",
			getReadName(),
			getMappingFlag(),
			getChromosome(),
			getLocation(),
			getMappingQuality(),
			getCigar(),
			getPairChromosome(),
			getPairLocation(),
			getTemplateLenght(),
			getSequence(),
			getQuality(),
			getOptional()
		);
	}

	void testRecords() const {
		LOG(
			"%s %d %s %zu %d %s %s %zu %d %s %s %s\n",
			getReadName(),
			getMappingFlag(),
			getChromosome(),
			getLocation(),
			getMappingQuality(),
			getCigar(),
			getPairChromosome(),
			getPairLocation(),
			getTemplateLenght(),
			getSequence(),
			getQuality(),
			getOptional()
		);
	}

	size_t getLineLength() { return lineLength; };
};
template<>
size_t sizeInMemory(Record t);


#endif // RECORD_H
