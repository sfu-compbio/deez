#ifndef FileIO_H
#define FileIO_H

#include "Common.h"
#include <curl/curl.h>
#include <zlib.h>

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
	virtual size_t advance(size_t size);

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

public:
	static shared_ptr<File> Open(const string &path, const char *mode);
	static bool Exists (const string &path);
	static string FullPath (const string &s);
	static string RemoveExtension (const string &s);
	static bool IsWeb (const string &path);
	static bool IsS3 (const string &path);
	static string GetURLforS3 (string url, CURL *ch, string method = "GET");
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

public:
	static shared_ptr<File> Download (const string &path, bool detectGZFiles = false);

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

#endif // FileIO_H
