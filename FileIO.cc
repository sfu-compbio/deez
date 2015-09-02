#include "FileIO.h"

bool IsWebFile (const string &path)
{
	return path.find("://") != string::npos;
}

File *OpenFile (const char *path, const char *mode) 
{
	if (IsWebFile(string(path))) 
		return new WebFile(path, mode);
	else
		return new File(path, mode);
}

bool FileExists (const char *path)
{
	bool result = false;
	if (IsWebFile(string(path))) {
		CURL *ch = curl_easy_init();
		curl_easy_setopt(ch, CURLOPT_URL, path);
		curl_easy_setopt(ch, CURLOPT_NOBODY, 1);
		curl_easy_setopt(ch, CURLOPT_FAILONERROR, 1);
		result = (curl_easy_perform(ch) == CURLE_OK);
		curl_easy_cleanup(ch);
	}
	else {
		FILE *f = fopen(path, "r");
		result = (f != 0);
		if (f) fclose(f);
	}
	return result;
}

File::File () 
{
	fh = 0;
}

File::File (const char *path, const char *mode) 
{ 
	open(path, mode); 
}

File::~File () 
{
	close();
}

void File::open (const char *path, const char *mode) 
{ 
	fh = fopen(path, mode); 
	if (!fh) throw DZException("Cannot open file %s", path);
	get_size();
}

void File::close () 
{ 
	if (fh) fclose(fh);
	fh = 0; 
}

ssize_t File::read (void *buffer, size_t size) 
{ 
	return fread(buffer, 1, size, fh); 
}

ssize_t File::read (void *buffer, size_t size, size_t offset) 
{
	fseek(fh, offset, SEEK_SET);
	return read(buffer, size);
}

uint8_t File::readU8 () 
{
	uint8_t var;
	if (read(&var, sizeof(uint8_t)) != sizeof(uint8_t))
		throw DZException("uint8_t read failed");
	return var;
}

uint16_t File::readU16 () 
{
	uint16_t var;
	if (read(&var, sizeof(uint16_t)) != sizeof(uint16_t))
		throw DZException("uint16_t read failed");
	return var;
}

uint32_t File::readU32 () 
{
	uint32_t var;
	if (read(&var, sizeof(uint32_t)) != sizeof(uint32_t))
		throw DZException("uint32_t read failed");
	return var;
}

uint64_t File::readU64 () 
{
	uint64_t var;
	if (read(&var, sizeof(uint64_t)) != sizeof(uint64_t))
		throw DZException("uint64_t read failed");
	return var;
}

ssize_t File::write (void *buffer, size_t size) 
{ 
	return fwrite(buffer, 1, size, fh); 
}

ssize_t File::tell ()
{
	return ftell(fh);
}

ssize_t File::seek (size_t pos)
{
	return fseek(fh, pos, SEEK_SET);
}

size_t File::size () 
{
	return fsize;
}

bool File::eof () 
{ 
	return feof(fh); 
}

void File::get_size () 
{
	fseek(fh, 0, SEEK_END);
	fsize = ftell(fh);
	fseek(fh, 0, SEEK_SET);
}

WebFile::WebFile (const char *path, const char *mode) 
{ 
	open(path, mode); 
}

WebFile::~WebFile () 
{
	close();
}

void WebFile::open (const char *path, const char *mode) 
{ 
	ch = curl_easy_init();
	if (!ch) throw DZException("Cannot open file %s", path);
	curl_easy_setopt(ch, CURLOPT_URL, path);
	curl_easy_setopt(ch, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, WebFile::CURLCallback); 
	get_size();
	foffset = 0;
}

void WebFile::close () 
{ 
	if (ch) curl_easy_cleanup(ch);
	ch = 0; 
}

ssize_t WebFile::read (void *buffer, size_t size) 
{ 
	return read(buffer, size, foffset);
}

ssize_t WebFile::read(void *buffer, size_t size, size_t offset) 
{
	CURLBuffer bfr;
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void*)&bfr); 
	curl_easy_setopt(ch, CURLOPT_RANGE, S("%zu-%zu", offset, offset + size - 1).c_str());
	auto result = curl_easy_perform(ch);
	if (result != CURLE_OK)
		throw DZException("CURL read failed");
	memcpy(buffer, bfr.data, bfr.size);
	foffset = offset + bfr.size;
	return bfr.size;
}

ssize_t WebFile::write (void *buffer, size_t size) 
{ 
	throw DZException("CURL write is not supported"); 
}

ssize_t WebFile::tell ()
{
	return foffset;
}

ssize_t WebFile::seek (size_t pos)
{
	return (foffset = pos);
}

size_t WebFile::size () 
{
	return fsize;
}

bool WebFile::eof () 
{ 
	return foffset >= fsize;  
}

void WebFile::get_size () 
{
	curl_easy_setopt(ch, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(ch, CURLOPT_HEADER, 0L);
	curl_easy_perform(ch);
 
 	double sz;
	auto res = curl_easy_getinfo(ch, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &sz);
	if (res != CURLE_OK) 
		throw DZException("Web server does not support file size query");
	fsize = (size_t)sz;
	curl_easy_setopt(ch, CURLOPT_NOBODY, 0);
}

size_t WebFile::CURLCallback (void *ptr, size_t size, size_t nmemb, void *data) 
{
	size_t realsize = size * nmemb;
	CURLBuffer *buffer = (CURLBuffer*)data;
	buffer->data = (char*)realloc(buffer->data, buffer->size + realsize);
	if (!buffer->data) 
		throw DZException("CURL reallocation failed");
	memcpy(buffer->data + buffer->size, ptr, realsize);
	buffer->size += realsize;		
	return realsize;
}

GzFile::GzFile (const char *path, const char *mode) 
{ 
	open(path, mode); 
}

GzFile::~GzFile () 
{
	close();
}

void GzFile::open (const char *path, const char *mode) 
{ 
	fh = gzopen(path, mode); 
	if (!fh) throw DZException("Cannot open file %s", path);
}

void GzFile::close () 
{ 
	if (fh) gzclose(fh);
	fh = 0; 
}

ssize_t GzFile::read (void *buffer, size_t size) 
{ 
	const size_t offset = 1 * (size_t)GB;
	if (size > offset) 
		return gzread(fh, buffer, offset) + read((char*)buffer + offset, size - offset); 
	else 
		return gzread(fh, buffer, size);
}

ssize_t GzFile::read(void *buffer, size_t size, size_t offset) 
{
	throw DZException("GZ random access is not yet supported");
}

ssize_t GzFile::write (void *buffer, size_t size) 
{ 
	return gzwrite(fh, buffer, size); 
}

ssize_t GzFile::tell ()
{
	return gztell(fh);
}

ssize_t GzFile::seek (size_t pos)
{
	return gzseek(fh, pos, SEEK_SET);
}

size_t GzFile::size () 
{
	throw DZException("GZ file size is not supported");
}

bool GzFile::eof () 
{ 
	return gzeof(fh); 
}

void GzFile::get_size () 
{
	throw DZException("GZ file size is not supported");
}
