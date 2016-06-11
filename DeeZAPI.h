#ifndef DeeZAPI_H
#define DeeZAPI_H

#include "Common.h"
#include "Parsers/BAMParser.h"
#include "Parsers/SAMParser.h"
#include "Parsers/Record.h"
#include "Sort.h"
#include "Common.h"
#include "Decompress.h"

class DeeZFile : public FileDecompressor {
public:
	struct SAMRecord {
		string rname;
		int flag;
		string chr;
		unsigned long loc;
		int mapqual;
		string cigar;
		string pchr;
		unsigned long ploc;
		int tlen;
		string seq;
		string qual;
		string opt;
	};

private:
	std::vector<std::vector<SAMRecord>> records;

	std::string prev_range;
	int prev_filter_flag;
	
protected:
    virtual inline void printRecord(const string &rname, int flag, const string &chr, const EditOperation &eo, int mqual,
        const string &qual, const string &optional, const PairedEndInfo &pe, int file, int thread)
    {
    	records[thread].push_back({rname, flag, chr, eo.start, mqual, eo.op, pe.chr, pe.pos, pe.tlen, eo.seq, qual, optional});
    }
    virtual inline void printComment(int file) {
    }

public:
	DeeZFile (const std::string &inFile, const std::string &genomeFile = ""):
		FileDecompressor(inFile, "", genomeFile, optBlock, true), records(4)
	{
	}
	~DeeZFile (void) {}

public:
	void setLogLevel (int level) {
		if (level < 0 || level > 2)
			throw DZException("Log level must be 0, 1 or 2");
		optLogLevel = level;
	}

	int getFileCount () {
		return comments.size();
	}

	std::string getComment (int file) {
		if (file < 0 || file >= comments.size())
			throw DZException("Invalid file index");
		return comments[file];
	}

	std::vector<SAMRecord> &getRecords (const std::string &range = "", int filterFlag = 0) {
		for (auto &r: records)
			r.clear();
		if (range != "") {
			prev_range = range;
			prev_filter_flag = filterFlag;
			FileDecompressor::decompress2(prev_range, prev_filter_flag, false);
		} else {
			FileDecompressor::decompress2(prev_range, prev_filter_flag, true);
			for (size_t i = 1; i < records.size(); i++)
				records[0].insert(records[0].end(), records[i].begin(), records[i].end());
		}
		return records[0];
	}

	std::vector<SAMRecord> &getRecordsOld (const std::string &range, int filterFlag = 0) {                                        
		for (auto &r: records)                                   
			r.clear();                                            
		FileDecompressor::decompress(range, filterFlag);         
		for (size_t i = 1; i < records.size(); i++)              
			records[0].insert(records[0].end(), records[i].begin(), records[i].end());     
		return records[0];
	}                                      
};

#endif // DeeZAPI_H