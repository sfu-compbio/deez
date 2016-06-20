#ifndef DeeZAPI_H
#define DeeZAPI_H

#include "Common.h"
#include "Parsers/BAMParser.h"
#include "Parsers/SAMParser.h"
#include "Parsers/Record.h"
#include "Sort.h"
#include "Common.h"
#include "Decompress.h"
#include "Legacy/v1.1/Decompress.h"

class DeeZFile {
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
	class FDv11 : public Legacy::v11::FileDecompressor {
		std::vector<std::vector<SAMRecord>> &records;
		virtual inline void printRecord(const string &rname, int flag, const string &chr, const Legacy::v11::EditOperation &eo, int mqual, const string &qual, const string &optional, const Legacy::v11::PairedEndInfo &pe, int file, int thread) {
	    	records[thread].push_back({rname, flag, chr, eo.start, mqual, eo.op, pe.chr, pe.pos, pe.tlen, eo.seq, qual, optional});
	    }
	    virtual inline void printComment(int file) {
	    }

	public:
		FDv11 (std::vector<std::vector<SAMRecord>> &rec, const std::string &inFile, const std::string &genomeFile = ""):
			Legacy::v11::FileDecompressor(inFile, "", genomeFile, optBlock), records(rec)
		{
		}
	};

	class FDv20 : public FileDecompressor {
		std::vector<std::vector<SAMRecord>> &records;
		virtual inline void printRecord(const string &rname, int flag, const string &chr, const EditOperation &eo, int mqual, const string &qual, const string &optional, const PairedEndInfo &pe, int file, int thread) {
	    	records[thread].push_back({rname, flag, chr, eo.start, mqual, eo.op, pe.chr, pe.pos, pe.tlen, eo.seq, qual, optional});
	    }
	    virtual inline void printComment(int file) {
	    }

	public:
		FDv20 (std::vector<std::vector<SAMRecord>> &rec, const std::string &inFile, const std::string &genomeFile = ""):
			FileDecompressor(inFile, "", genomeFile, optBlock, true), records(rec)
		{
		}
	};


	std::vector<std::vector<SAMRecord>> records;
	std::string prev_range;
	int prev_filter_flag;
	
	shared_ptr<IFileDecompressor> dec;


public:
	DeeZFile (const std::string &inFile, const std::string &genomeFile = ""):
		records(4)
	{
		auto file = File::Open(inFile.c_str(), "rb");
		if (file == NULL) {
			throw DZException("Cannot open the file %s", inFile.c_str());
		}

		uint32_t magic = file->readU32();
		if ((magic & 0xff) < 0x20) {
			LOG("Using old DeeZ v1.1 engine");
			dec = make_shared<FDv11>(records, inFile, genomeFile);
		} else {
			dec = make_shared<FDv20>(records, inFile, genomeFile);
		}

	}
	~DeeZFile (void) {}

public:
	void setLogLevel (int level) {
		if (level < 0 || level > 2) {
			throw DZException("Log level must be 0, 1 or 2");
		}
		optLogLevel = level;
	}

	int getFileCount () {
		return dec->comments.size();
	}

	std::string getComment (int file) {
		if (file < 0 || file >= dec->comments.size()) {
			throw DZException("Invalid file index");
		}
		return dec->comments[file];
	}

	std::vector<SAMRecord> &getRecords (const std::string &range = "", int filterFlag = 0) {
		for (auto &r: records)
			r.clear();
		if (range != "") {
			prev_range = range;
			prev_filter_flag = filterFlag;
			dec->decompress2(prev_range, prev_filter_flag, false);
		} else {
			dec->decompress2(prev_range, prev_filter_flag, true);
			for (size_t i = 1; i < records.size(); i++)
				records[0].insert(records[0].end(), records[i].begin(), records[i].end());
		}
		return records[0];
	}
};

#endif // DeeZAPI_H