#include "FileIO.h"
#include <sys/stat.h>
#include <cstring>
#include <sstream>

#ifdef OPENSSL
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/md5.h>
#endif

using namespace std;

namespace Legacy {
namespace v11 {

bool IsWebFile (const string &path)
{
	return path.find("://") != string::npos;
}

bool IsS3File (const string &path)
{
	return IsWebFile(path) && (path.size() > 5 && path.substr(0, 5) == "s3://");
}

string SetS3File (string url, CURL *ch, string method = "GET") 
{
	if (IsS3File(url)) {
		url = url.substr(5);
		auto pos = url.find('/');
		if (pos != string::npos) {
			string bucket = url.substr(0, pos);
			string location = url.substr(pos + 1);

			url = S("https://%s.s3.amazonaws.com/%s", bucket.c_str(), location.c_str());
			//LOG("Using %s", url.c_str());

#ifdef OPENSSL
			char *accessKey = getenv("AWS_ACCESS_KEY_ID");
			char *secretKey = getenv("AWS_SECRET_ACCESS_KEY");
			if (accessKey && secretKey && string(accessKey) != "" && string(secretKey) != "") {
				curl_slist *list = 0;
				//LOG("Using AWS credentials found in AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY");
				
				list = curl_slist_append(list, "Accept:");
				list = curl_slist_append(list, S("Host: %s.s3.amazonaws.com", bucket.c_str()).c_str());

				// Proper AWS date
				time_t rawtime;
				time(&rawtime);
				tm *tms = localtime(&rawtime);
				char date[150];
				strftime(date, 150, "%a, %d %b %Y %T %z", tms);
				list = curl_slist_append(list, S("Date: %s", date).c_str());

				// HMAC-SHA1 hash
				string data = S("%s\n\n\n%s\n/%s/%s", method.c_str(), date, bucket.c_str(), location.c_str());
				unsigned char* digest = HMAC(
					EVP_sha1(), 
					secretKey, strlen(secretKey), 
					(unsigned char*)data.c_str(), data.size(), 
					NULL, NULL
				);    

				// Base64 
				BUF_MEM *bufferPtr;
				BIO *b64 = BIO_new(BIO_f_base64());
				BIO *bio = BIO_new(BIO_s_mem());
				bio = BIO_push(b64, bio);
				BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
				BIO_write(bio, digest, 20);
				BIO_flush(bio);
				BIO_get_mem_ptr(bio, &bufferPtr);
				BIO_set_close(bio, BIO_NOCLOSE);
				BIO_free_all(bio);

				char *encodedSecret = (*bufferPtr).data;

				//auto q =S("curl -X GET -H 'Host: 1000genomes.s3.amazonaws.com' -H 'Date: %s' -H 'Authorization: AWS %s:%s' %s",
				//	date, accessKey, encodedSecret, url.c_str());
				//LOG("%s",q.c_str());
				//system(q.c_str());

				list = curl_slist_append(list, S("Authorization: AWS %s:%s", accessKey, encodedSecret).c_str());
				curl_easy_setopt(ch, CURLOPT_HTTPHEADER, list);
			}
#endif
			//curl_easy_setopt(ch, CURLOPT_VERBOSE, 1);
		}
	}
	return url;
}


File *OpenFile (const string &path, const char *mode) 
{
	if (IsWebFile(path)) 
		return new WebFile(path, mode);
	else
		return new File(path, mode);
}

bool FileExists (const string &path)
{
	bool result = false;
	if (IsWebFile(path)) {
		CURL *ch = curl_easy_init();
		curl_easy_setopt(ch, CURLOPT_NOBODY, 1);
		curl_easy_setopt(ch, CURLOPT_FAILONERROR, 1);

		string url = path;
		if (IsS3File(url))
			url = SetS3File(url, ch, "HEAD");
		curl_easy_setopt(ch, CURLOPT_URL, url.c_str());

		result = (curl_easy_perform(ch) == CURLE_OK);
		curl_easy_cleanup(ch);
	}
	else {
		FILE *f = fopen(path.c_str(), "r");
		result = (f != 0);
		if (f) fclose(f);
	}
	return result;
}

string fullPath (const string &s) {
	if (IsWebFile(s))
		return s;
	char *pp = realpath(s.c_str(), 0);
	string p = pp;
	free(pp);
	return p;
}

File::File () 
{
	fh = 0;
}

File::File (FILE *handle) 
{
	fh = handle;
}

File::File (const string &path, const char *mode) 
{ 
	open(path, mode); 
}

File::~File () 
{
	close();
}

void File::open (const string &path, const char *mode) 
{ 
	fh = fopen(path.c_str(), mode); 
	if (!fh) throw DZException("Cannot open file %s", path.c_str());
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

char File::getc ()
{
	char c;
	if (read(&c, 1) == 0)
		c = EOF;
	return c;
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

void *File::handle ()
{
	return fh;
}

WebFile::WebFile (const string &path, const char *mode) 
{ 
	open(path, mode); 
}

WebFile::~WebFile () 
{
	close();
}

void WebFile::open (const string &path, const char *mode) 
{ 
	ch = curl_easy_init();
	if (!ch) throw DZException("Cannot open file %s", path.c_str());

	string url = path;
	if (IsS3File(url))
		url = SetS3File(url, ch);

	curl_easy_setopt(ch, CURLOPT_URL, url.c_str());
	curl_easy_setopt(ch, CURLOPT_VERBOSE, optLogLevel >= 2);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, WebFile::CURLCallback); 
	curl_easy_setopt(ch, CURLOPT_FAILONERROR, 1);
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

File *WebFile::Download (const string &path, bool detectGZFiles)
{
	LOGN("Downloading %s ...     ", path.c_str());
	CURL *ch = curl_easy_init();
	string url = path;
	if (IsS3File(url))
		url = SetS3File(url, ch);
	curl_easy_setopt(ch, CURLOPT_URL, url.c_str());
	curl_easy_setopt(ch, CURLOPT_VERBOSE, optLogLevel >= 2);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, WebFile::CURLDownloadCallback); 
	FILE *f = tmpfile(); //fopen("HAMO.txt","a+b");// tmpfile();
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, f);
	curl_easy_setopt(ch, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(ch, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(ch, CURLOPT_PROGRESSFUNCTION, WebFile::CURLDownloadProgressCallback);
	auto res = curl_easy_perform(ch);
	if (res != CURLE_OK) 
		throw DZException("Cannot download file %s", path.c_str());
	curl_easy_cleanup(ch);
	fflush(f);
	fseek(f, 0, SEEK_SET);
	unsigned char magic[2];
	fread(magic, 1, 2, f);
	fseek(f, 0, SEEK_SET);
	fflush(f);

	if (detectGZFiles && magic[0] == 0x1f && magic[1] == 0x8b) {
		LOG("\b\b\b\b opened as GZ file"); 
		return new GzFile(f);
	} 
	else {
		LOG("");
		return new File(f);
	}
}

int WebFile::CURLDownloadProgressCallback (void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded)
{
	if (int(TotalToDownload))
		LOGN("\b\b\b\b%3.0lf%%", 100.0 * NowDownloaded / TotalToDownload);
	return 0;
}

size_t WebFile::CURLDownloadCallback (void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    return fwrite(ptr, size, nmemb, stream);
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


void *WebFile::handle ()
{
	return ch;
}

GzFile::GzFile (const string &path, const char *mode) 
{ 
	open(path, mode); 
}

GzFile::GzFile (FILE *handle) 
{
	fh = gzdopen(fileno(handle), "rb");
	if (!fh) throw DZException("Cannot open GZ file via handle");

	//char r[50];
	//gzread(fh, r, 50); r[49] = 0;
	//LOG("%s",r);
}

GzFile::~GzFile () 
{
	close();
}

void GzFile::open (const string &path, const char *mode) 
{ 
	fh = gzopen(path.c_str(), mode); 
	if (!fh) throw DZException("Cannot open file %s", path.c_str());
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
	else {
		return gzread(fh, buffer, size);
	}
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

void *GzFile::handle ()
{
	return fh;
}


// Load order:
// 	-r
//		if FASTA file, provided file
// 		if directory, <chr>.fa
//	SQ, if exists
//  No file

Reference::Reference (const string &fn):
	input(0)
{
	chromosomes["*"] = {"*", "", "", 0, 0};

	if (fn == "") // Load filename later via SQ header, if applicable
		return;
	else if (!IsWebFile(fn)) { // If directory, load later
		struct stat st;
		lstat(fn.c_str(), &st);
		if (fn == "" || S_ISDIR(st.st_mode)) {
			directory = fn;
			LOG("Using directory %s for searching chromosome FASTA files", directory.c_str());
			return;
		}
	}
	else if (fn.size() > 0 && fn[fn.size() - 1] == '/') { // Web directory
		directory = fn;
		LOG("Using web directory %s for searching chromosome FASTA files", directory.c_str());
		return;
	}
	loadFromFASTA(fn);
}

Reference::~Reference (void) 
{
	if (input) delete input;
}

void Reference::loadFromFASTA (const string &fn)
{
	input = OpenFile(fn, "rb");
	LOG("Loaded reference file %s", fn.c_str());

	try { // Try loading .fai file
		string faidx = fn + ".fai";
		File *fastaidx;
		if (IsWebFile(faidx)) 
			fastaidx = WebFile::Download(faidx);
		else
			fastaidx = new File(faidx, "rb");
		if (!fastaidx)
			throw DZException("Cannot open FASTA index %s", faidx.c_str());
		char chr[50];
		size_t len, loc;
		while (fscanf((FILE*) fastaidx->handle(), "%s %lu %lu %*lu %*lu", chr, &len, &loc) != EOF) 
			chromosomes[chr] = {chr, "", fn, len, loc};
		delete fastaidx;
		LOG("Loaded reference index %s", faidx.c_str());
	}
	catch (DZException &e) {
		FILE *fastaidx = 0;
		if (IsWebFile(fn)) {
			delete input, input = 0;
			input = WebFile::Download(fn, true);
			currentWebFile = fn;
		}
		else {
			LOG("FASTA index for %s not found, creating one ...", fn.c_str());
			fastaidx = fopen(string(fn + ".fai").c_str(), "wb");
			if (!fastaidx)
				WARN("Cannot create reference index for %s", fn.c_str());
		}

		string chr = "";
		size_t cnt = 0, cntfull = 0, pos;
		while ((c = input->getc()) != EOF) {
			if (c == '>') {
				if (chr != "") {
					if (fastaidx) fprintf(fastaidx, "%s\t%lu\t%lu\t%lu\t%lu\t\n", chr.c_str(), cnt, pos, cnt, cntfull);
					chromosomes[chr].len = cnt;
				}

				chr = "";
				cnt = cntfull = 0;
				
				c = input->getc();
				while (!isspace(c) && c != EOF) 
					chr += c, c = input->getc();
				while (c != '\n' && c != EOF) {
					c = input->getc();
				}
				
				pos = input->tell();
				chromosomes[chr] = {chr, "", filename, 0, pos};
				continue;
			}
			if (!isspace(c)) 
				cnt++;
			cntfull++;
		}
		if (chr != "") {
			if (fastaidx) fprintf(fastaidx, "%s\t%lu\t%lu\t%lu\t%lu\n", chr.c_str(), cnt, pos, cnt, cntfull);
			chromosomes[chr].len = cnt;
		}
		if (fastaidx) fclose(fastaidx);
		input->seek(0);
	}

	filename = fullPath(fn);
}

string Reference::getChromosomeName (void) const 
{
	return currentChr;
}

size_t Reference::getChromosomeLength(const std::string &s) const 
{
	auto it = chromosomes.find(s);
	if (it != chromosomes.end())
		return it->second.len;
	else
		return 0;
}

void Reference::scanSAMComment (const string &comment) 
{	
	size_t pos;
	istringstream ss(comment);
	string l, lo;
	while (getline(ss, l)) {
		if (l.size() <= 4 || l.substr(0, 4) != "@SQ\t") continue;
		lo = l, l = l.substr(4);
		
		unordered_map<string, string> val;
		while (l.size() > 3 && (pos = l.find('\t')) != string::npos) {
			val[l.substr(0, 2)] = l.substr(3, pos - 3);
			l = l.substr(pos + 1);
		}
		if (l.size() > 3 && isalpha(l[0]))
			val[l.substr(0, 2)] = l.substr(3, pos - 3);
		//val["fullLine"] = lo;
		if (val.find("SN") != val.end()) 
			samCommentData[val["SN"]] = val;
	}

	for (auto &c: samCommentData) {
		DEBUGN("%s: ", c.first.c_str());
		for (auto &e: c.second)
			DEBUGN("%s->%s ", e.first.c_str(), e.second.c_str());
		DEBUG("");
	}
}

std::string Reference::scanChromosome (const string &s) 
{
	if (s == "*")
		return currentChr = s;

	try {
		if (directory == "") {
			auto it = chromosomes.find(s);
			if (it == chromosomes.end()) 
				throw DZException("Chromosome %s not found in the reference", s.c_str());
			input->seek(it->second.loc);
		}
		else {
			string fn = directory + "/" + s + ".fa";
			if (directory.size() && directory[directory.size() - 1] == '/')
				fn = directory + s + ".fa";
			delete input, input = 0;
			if (IsWebFile(fn)) {
				input = WebFile::Download(fn, true);
				currentWebFile = fn;
			}
			else
				input = OpenFile(fn, "rb");
			
			LOG("Loaded reference file %s for chromosome %s", fn.c_str(), s.c_str());
			/*char c;
			while ((c = input->getc()) != EOF) {
				if (c == '>') {
					string chr;
					size_t pos = input->tell();
					c = input->getc();
					while (!isspace(c) && c != EOF) 
						chr += c, c = input->getc();
					chromosomes[chr] = {chr, "", fn, 0, pos};
				}
			}
			input->seek(0);
			input->getc();*/

			filename = fullPath(fn);
		}
	}
	catch (DZException &e) {
		// Try Doing Sth Else
		try {
			auto it = samCommentData[s].find("UR");
			if (it != samCommentData[s].end() && IsWebFile(it->second) && currentWebFile != it->second) {
				LOG("Loaded reference file %s for chromosome %s via @SQ:UR field", it->second.c_str(), s.c_str());
				delete input, input = 0;
				input = WebFile::Download(currentWebFile = it->second, true);
				filename = fullPath(it->second);
			}
			else throw DZException("Cannot find reference in @SQ:UR");
		}
		catch (DZException &e) {
			WARN("Could not find reference file for chromosome %s, using no reference instead", s.c_str());
			filename = "";
			chromosomes[s] = {s, "", "<implicit>", 0, 0};
			return currentChr = s; // fill with Ns later
		}
	}
	
	currentChr = s;
	c = input->getc();
	if (c == '>') {
		currentChr = "";
		while (!isspace(c) && c != EOF) 
			currentChr += c, c = input->getc();
		currentChr = currentChr.substr(1);
		// skip fasta comment
		while (c != '\n') 
			c = input->getc();
		// park at first nucleotide
		c = input->getc();
	}
	chromosomes[currentChr].chr = currentChr;
	chromosomes[currentChr].filename = filename;
	chromosomes[currentChr].loc = input->tell();
	if (chromosomes[currentChr].loc) chromosomes[currentChr].loc--;

#ifdef OPENSSL // calculate MD5 of chromosome
	MD5_CTX ctx;
    MD5_Init(&ctx);
    size_t pos = input->tell(), cnt = 0;
    string buf = "";
    while (c != '>' && c != EOF) {
    //	LOGN("%c",c);
    	if (isalpha(c)) buf += c;
    	c = input->getc();
    	if (buf.size() == 1024 * 1024)
    		MD5_Update(&ctx, buf.c_str(), buf.size()), cnt += buf.size(), buf = "";
    }
	if (buf.size())
		MD5_Update(&ctx, buf.c_str(), buf.size()), cnt += buf.size(), buf = "";
	uint8_t md5[MD5_DIGEST_LENGTH];
	MD5_Final(md5, &ctx);
	string md5sum = "";
	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
		md5sum += S("%02x", md5[i]);
	chromosomes[currentChr].len = cnt;
	chromosomes[currentChr].md5 = md5sum;
	DEBUG("%s %s %d...",currentChr.c_str(),md5sum.c_str(),cnt);
	input->seek(pos);
#endif

	currentPos = 0;
	if (c == '>' || c == EOF) 
		return currentChr = s; // Empty chromosome
	return currentChr;
}

void Reference::load (char *arr, size_t s, size_t e) 
{
	auto it = chromosomes.find(currentChr);
	if (it == chromosomes.end() || it->second.len == 0) {
		for (size_t i = 0; i < e - s; i++)
			arr[i] = 'N';
		return;
	}

	size_t i = 0;
	while (currentPos < e) {
		if (currentPos >= s) 
			arr[i++] = c;

		c = input->getc();
		while (isspace(c) && c != EOF)
			c = input->getc();
		if (c == '>' || c == EOF) 
			break;
		currentPos++;
	}
	while (i < e - s)
		arr[i++] = 'N';
	assert(i == e - s);
}

};
};
