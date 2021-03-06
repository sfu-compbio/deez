#pragma once

#include "Common.h"
#include <curl/curl.h>
#include <zlib.h>
#include <unordered_map>

namespace Legacy {
namespace v11 {

using namespace std;

class File
{
	FILE *fh;
	size_t fsize;

protected:
	File ();

public:
	File (FILE *handle);
	File (const string &path, const char *mode);
	virtual ~File ();

	virtual void open (const string &path, const char *mode);
	virtual void close ();

	virtual ssize_t read (void *buffer, size_t size);
	virtual ssize_t read (void *buffer, size_t size, size_t offset);
	virtual ssize_t write (void *buffer, size_t size);

	virtual char getc();
	virtual uint8_t readU8();
	virtual uint16_t readU16();
	virtual uint32_t readU32();
	virtual uint64_t readU64();

	virtual ssize_t tell ();
	virtual ssize_t seek (size_t pos);

	virtual size_t size ();
	virtual bool eof ();
	virtual void *handle ();

private:
	virtual void get_size ();
};

class WebFile: public File 
{
	CURL *ch;
	size_t fsize, foffset;

public:
	WebFile (const string &path, const char *mode);
	~WebFile ();

	void open (const string &path, const char *mode);
	void close ();

	ssize_t read (void *buffer, size_t size);
	ssize_t read(void *buffer, size_t size, size_t offset);
	ssize_t write (void *buffer, size_t size);

	ssize_t tell ();
	ssize_t seek (size_t pos);

	size_t size ();
	bool eof ();
	void *handle ();

	static File *Download (const string &path, bool detectGZFiles = false);

private:
	void get_size ();

	struct CURLBuffer {
		char *data;
		size_t size;
		CURLBuffer () : data(0), size(0) {};
		~CURLBuffer () { free(data); }
	};
	static size_t CURLCallback (void *ptr, size_t size, size_t nmemb, void *data);
	static int CURLDownloadProgressCallback (void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded);
	static size_t CURLDownloadCallback (void *ptr, size_t size, size_t nmemb, FILE *stream);
};

class GzFile: public File
{
	gzFile fh;

public:
	GzFile (FILE *handle);
	GzFile (const string &path, const char *mode);
	~GzFile ();

	void open (const string &path, const char *mode);
	void close ();

	ssize_t read (void *buffer, size_t size);
	ssize_t read(void *buffer, size_t size, size_t offset);
	ssize_t write (void *buffer, size_t size);

	ssize_t tell ();
	ssize_t seek (size_t pos);

	size_t size ();
	bool eof ();
	void *handle ();

private:
	void get_size ();
};

File *OpenFile (const string &path, const char *mode);
bool FileExists (const string &path);
bool IsWebFile (const string &path);

class Reference {
	friend class Stats;

public:
	struct Chromosome {
		std::string chr;
		std::string md5;
		std::string filename;
		size_t len;
		size_t loc;
	};

private:
	File *input;
	std::string directory, filename;
	std::string currentChr;
	size_t currentPos;
	char c;

	std::unordered_map<std::string, Chromosome> 
		chromosomes;
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
		samCommentData;
	std::string currentWebFile;
	
public:
	Reference (const std::string &filename);
	~Reference (void);

public:
	std::string getChromosomeName(void) const;
	std::string scanChromosome(const std::string &s);
	void load(char *arr, size_t s, size_t e);
	size_t getChromosomeLength(const std::string &s) const;
	void scanSAMComment (const std::string &comment);

	Chromosome getChromosomeInfo (const std::string &chr) {
		return chromosomes[chr];
	}

private:
	void loadFromFASTA (const std::string &fn);
};

std::string fullPath (const std::string &s);

};
};
