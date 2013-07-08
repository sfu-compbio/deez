#include <cstdio>
#include <vector>
#include <clocale>
#include <cctype>
#include "Engines/GenericEngine.h"
#include "Streams/ArithmeticStream.h"
#include "Streams/GzipStream.h"
#include "Streams/ContextModels/Order3Model.h"
#include "Streams/ContextModels/Order3Model.cc"
#include "Streams/ContextModels/OrderNModel.h"
#include "Engines/StringEngine.h"
#include "FieldsTest/OptionalField.h"
#include "FieldsTest/OptionalField.cc"
using namespace std;

inline bool isdna(char c) { return c=='A'||c=='G'||c=='C'||c=='T'||c=='N'; }
inline bool islowerdna(char c) { return islower(c) && isdna(toupper(c)); }

inline char getdnacode(char c) {
	if (c=='A') return 0;
	if (c=='C') return 1;
	if (c=='G') return 2;
	if (c=='T') return 3;
	if (c=='N') return 4;
	throw "dnacode";
}
inline char getopcode(char c) {
	if (c=='=') return 0;
	if (c=='D') return 1;
	if (c=='K') return 2;
	if (c=='!') return 3;
	if (c=='I') return 4;
	if (c=='S') return 5;
	if (c=='*') return 6;
	throw "opcode";
}
inline char getdnacode_r(char c) {
	if (c==0) return 'A';
	if (c==1) return 'C';
	if (c==2) return 'G';
	if (c==3) return 'T';
	if (c==4) return 'N';
	throw "dnacode";
}
inline char getopcode_r(char c) {
	if (c==0) return '=';
	if (c==1) return 'D';
	if (c==2) return 'K';
	if (c==3) return '!';
	if (c==4) return 'I';
	if (c==5) return 'S';
	if (c==6) return '*';
	throw "opcode";
}

class EditOperationCompressor {
	CompressionStream *stream;
	CompressionStream *editKeyStream;
	CompressionStream *editValStream;
	CompressionStream *sequenceStream;

	std::vector<char> lengths;
	std::vector<char> keys;
	std::vector<int> values;
	std::vector<char> sequences;

public:
	typedef OrderNModel<unsigned char, 256, 2> LenModel;
	typedef OrderNModel<char, 7, 8> KeyModel;
	typedef OrderNModel<char, 5, 12> SeqModel;

	EditOperationCompressor () {
		stream = new ArithmeticCompressionStream<LenModel>();
		editKeyStream = new ArithmeticCompressionStream<KeyModel>();
		editValStream = new GzipCompressionStream<6>();
		sequenceStream = new ArithmeticCompressionStream<SeqModel>();

		cs=csc=0;
	//	sequences.reserve(4ll*1024*1024*1024); // 4g
	//	lengths.reserve(40*1024*1024);
	//	keys.reserve(40*1024*1024);
	}
	~EditOperationCompressor (void) {
		delete editKeyStream;
		delete editValStream;
		delete sequenceStream;
		delete stream;
	}

public:
	char cs;
	char csc;

	void addRecord (const string &s) {
		vector<char> v;
		int i = 0, l = 0, n = 0;
		while (i < s.size()) {
			if (isdigit(s[i])) {
				while (isdigit(s[i])) n = n * 10 + s[i] - '0', i++;
				if(s[i] != 'D' && s[i] != 'K' && s[i] != '=') throw "DK=";
				values.push_back(n), n = 0;
				keys.push_back(getopcode(s[i++])), l++;
				continue;
			}
			if (islowerdna(s[i])) {
				int n = 0;
				while (islowerdna(s[i]))  {
				//	cs = (cs << 2) | getdnacode(toupper(s[i])), csc++, i++, n++;
				//	if (csc == 4) sequences.push_back(cs), csc = 0;
						v.push_back(getdnacode(toupper(s[i]))), n++, i++;
				}
				values.push_back(n);
				keys.push_back(getopcode('!')), l++;
				continue;
			}
			if (isdna(s[i])) {
				int n = 0;
				while (isdna(s[i])) {
				//	cs = (cs << 2) | getdnacode(toupper(s[i])), csc++, i++, n++;
				//	if (csc == 4) sequences.push_back(cs), csc = 0;
					v.push_back(getdnacode(toupper(s[i]))), n++, i++;
				}
				values.push_back(n);
				keys.push_back(getopcode(s[i++])), l++;
				continue;
			}
			LOG("%c", s[i]);
			throw "unknown";
		}
		sequences.insert(sequences.end(), v.begin(), v.end());
		lengths.push_back(l);
	}

	void flush (void) {
		// ~~~~~~~~~~~~~

		LOG("read %'lld\n", lengths.size());

		vector<char> output;
		size_t sz, t_sz(0);

		output.clear(), stream->compress(&lengths[0], lengths.size() * sizeof(lengths[0]), output), t_sz += sz = output.size();
		fwrite(&sz, sizeof(size_t), 1, stdout), fwrite(&output[0], 1, sz, stdout);
		LOG("Lengths\t%'15lld", sz);

		output.clear(), editKeyStream->compress(&keys[0], keys.size() * sizeof(keys[0]), output), t_sz += sz = output.size();
		fwrite(&sz, sizeof(size_t), 1, stdout), fwrite(&output[0], 1, sz, stdout);
		LOG("Keys\t%'15lld", sz);

		output.clear(), editValStream->compress(&values[0], values.size() * sizeof(values[0]), output), t_sz += sz = output.size();
		fwrite(&sz, sizeof(size_t), 1, stdout), fwrite(&output[0], 1, sz, stdout);
		LOG("Values\t%'15lld", sz);

		sequences.push_back(cs);
		output.clear(), sequenceStream->compress(&sequences[0], sequences.size() * sizeof(sequences[0]), output), t_sz += sz = output.size();
		fwrite(&sz, sizeof(size_t), 1, stdout), fwrite(&output[0], 1, sz, stdout);
		LOG("Sequence\t%'15lld", sz);

		LOG("wrote %'lld\n", t_sz);
	}
};

class EditOperationDecompressor {
	DecompressionStream *stream;
	DecompressionStream *editKeyStream;
	DecompressionStream *editValStream;
	DecompressionStream *sequenceStream;

	std::vector<char> lengths;
	std::vector<char> keys;
	std::vector<int> values;
	std::vector<char> sequences;

public:
	typedef OrderNModel<char, 256, 2> LenModel;
	typedef OrderNModel<char, 7, 8> KeyModel;
	typedef OrderNModel<char, 256, 2> SeqModel;

	EditOperationDecompressor () {
		stream = new ArithmeticDecompressionStream<LenModel>();
		editKeyStream = new ArithmeticDecompressionStream<KeyModel>();
		editValStream = new GzipDecompressionStream();
		sequenceStream = new ArithmeticDecompressionStream<SeqModel>();
	}
	~EditOperationDecompressor (void) {
		delete editKeyStream;
		delete editValStream;
		delete sequenceStream;
		delete stream;
	}

public:
	void addRecord () {
		int vi = 0, si = 0, ki = 0;
		for (int l = 0; l < lengths.size(); l++) {
			string s = "";
			for (int i = 0; i < lengths[l]; i++) {
				char c = getopcode_r(keys[ki]);
				if(c == 'D' || c == 'K' || c == '=') {
					char x[50];
					sprintf(x, "%d", values[vi]);
					s += string(x);
				}
				else {
					for (int j = 0; j < values[vi]; j++) {
						char x = getdnacode_r(sequences[si++]);
						if (c == 'I' || c == 'S') x = tolower(x);
						s += x;
					}
				}
				s += c;
				ki++, vi++;
			}
			fwrite(&s[0], 1, s.size()+1, stdout);
		}
	}

	void get (void) {
		vector<char> in, output;
		size_t sz, t_sz(0);

		fread(&sz, sizeof(size_t), 1, stdin);  in.resize(sz); fread(&in[0], 1, sz, stdin);
		stream->decompress(&in[0], sz, lengths);

		fread(&sz, sizeof(size_t), 1, stdin);  in.resize(sz); fread(&in[0], 1, sz, stdin);
		editKeyStream->decompress(&in[0], sz, keys);

		fread(&sz, sizeof(size_t), 1, stdin);  in.resize(sz); fread(&in[0], 1, sz, stdin);
		editValStream->decompress(&in[0], sz, output);
		values.resize(output.size() / sizeof(int));
		memcpy(&values[0], &output[0], output.size());

		fread(&sz, sizeof(size_t), 1, stdin);  in.resize(sz); fread(&in[0], 1, sz, stdin);
		sequenceStream->decompress(&in[0], sz, sequences);
	}
};

int compress_decompress_editops_test(int blocksz) {
	EditOperationCompressor ec;
	string s = "";
	char c; int i = 0;
	while ((c = fgetc(stdin)) != EOF) {
		if (!c) {
			ec.addRecord(s);
			s = "";
			i++;
		}
		else s += c;
	}
	ec.flush();
}

int compress_decompress_qualities_test(int blocksz) {
	int bsc = 0, bc = 0, tc = 0, rb = 0, ob = 0;
	int flag; char qual[500];
	auto *c = new StringCompressor<ArithmeticCompressionStream<Order3Model>>(blocksz);
	while (scanf("%d %s", &flag, qual) != EOF) {
		/// do qual magic
		int l = strlen(qual), i; 
		if (flag & 0x10) for (i = 0; i < l/2; i++) swap(qual[i], qual[l-i-1]);
		
		for (i = l-1; i >= 0 && qual[i] == '#'; i--);
		if (i < l-1) qual[1+i] = '\t', qual[2+i] = 0;
		/// add stuff
		c->addRecord(qual), bsc++, tc++; rb += l;
		if (blocksz == bsc) {
			vector<char> out; c->outputRecords(out), bsc = 0, bc++;
			size_t i = out.size(); fwrite(&i, sizeof(size_t), 1, stdout), ob += i; 
			if (out.size()) fwrite(&out[0], sizeof(char), out.size(), stdout);
		}
	}
	vector<char> out; c->outputRecords(out), bsc = 0, bc++;
	size_t i = out.size(); fwrite(&i, sizeof(size_t), 1, stdout);
	if (out.size()) fwrite(&out[0], sizeof(char), out.size(), stdout);

	LOG("read %d values, %d blocks, %d bytes, output %d bytes, redundant %d", 
		tc, bc, rb, ob, bc * sizeof(size_t));
}


int compress_decompress_optionals_test(int blocksz) {
	size_t bsc = 0, bc = 0, tc = 0, rb = 0, ob = 0;
	int flag; char qual[2000];
	auto *c = new OptionalFieldCompressor(blocksz);
	while (fgets(qual, 2000, stdin)) {
		/// do qual magic
		int l = strlen(qual)-1, i; 
		if (!l) continue;
		qual[l]=0;
		/// add stuff
		c->addRecord(qual), bsc++, tc++; rb += l;
	
		if (blocksz == bsc) {
			vector<char> out; c->outputRecords(out), bsc = 0, bc++;
			size_t i = out.size(); fwrite(&i, sizeof(size_t), 1, stdout), ob += i; 
			if (out.size()) fwrite(&out[0], sizeof(char), out.size(), stdout);
			fflush(stdout);
			LOG("Flush %d", bc);
		}
	}
	vector<char> out; c->outputRecords(out), bsc = 0, bc++;
	size_t i = out.size(); fwrite(&i, sizeof(size_t), 1, stdout);
	if (out.size()) fwrite(&out[0], sizeof(char), out.size(), stdout);

	LOG("read %llu values, %llu blocks, %llu bytes, output %llu bytes, redundant %llu", 
		tc, bc, 
		rb, ob, bc * sizeof(size_t));
	delete c;
}

int compress_decompress_readnames_test(int blocksz) {
	size_t bsc = 0, bc = 0, tc = 0, rb = 0, ob = 0;
	int flag; char qual[2000];
	auto *c = new GenericCompressor<uint32_t, GzipCompressionStream<6> >(blocksz);
	while (fgets(qual, 2000, stdin)) {
		/// do qual magic
		int l = strlen(qual)-1, i; 
		if (!l) continue;
		qual[l]=0;
		/// add stuff
		int e;
		for (e=0;qual[e]!='.';e++);
		//fprintf(stderr,"%lld\n", atoll(qual+e+1));
		assert(atoll(qual+e+1)<uint32_t(-1));
		c->addRecord(atoll(qual+e+1)), bsc++, tc++; rb += l;
	
		if (blocksz == bsc) {
			vector<char> out; c->outputRecords(out), bsc = 0, bc++;
			size_t i = out.size(); fwrite(&i, sizeof(size_t), 1, stdout), ob += i; 
			if (out.size()) fwrite(&out[0], sizeof(char), out.size(), stdout);
			fflush(stdout);
			LOG("Flush %d", bc);
		}
	}
	vector<char> out; c->outputRecords(out), bsc = 0, bc++;
	size_t i = out.size(); fwrite(&i, sizeof(size_t), 1, stdout);
	if (out.size()) fwrite(&out[0], sizeof(char), out.size(), stdout);

	LOG("read %llu values, %llu blocks, %llu bytes, output %llu bytes, redundant %llu", 
		tc, bc, 
		rb, ob, bc * sizeof(size_t));
	delete c;
}

int main (int argc, char **argv) {
	setlocale(LC_ALL, "");
	int blocksz = 500000;
	compress_decompress_editops_test(blocksz);

/*
	//try {
	
	//}
	/*catch (const char *c) {
		DEBUG("%s", c);
		abort();
	}*/

	/*	if (argv[1][0] == 'c') {
	 auto *ac = new ArithmeticCompressionStream<Order3Model>();

	 int sz;
	 int cnt = 0;
	 while (sz = fread(bf, 1, bfsz, stdin)) {
	 vector<char> result;
	 ac->compress(bf, sz, result);

	 size_t x = result.size();
	 fwrite(&x, 1, sizeof(size_t), stdout);
	 fwrite(&result[0], 1, result.size(), stdout);
	 fflush(stdout);
	 cnt++;
	 }

	 DEBUG("wasted %'llu", cnt*2*sizeof(size_t));
	 delete ac;
	 }*/
	/*if (argv[1][0] == 'd') {
	 try{
	 auto *ac = new StringDecompressor<GzipDecompressionStream>(1000000);// ArithmeticDecompressionStream<Order3Model>();

	 size_t sz;
	 while (fread(&sz, 1, sizeof(size_t), stdin)) {
	 vector<char> in(sz), result;
	 fread(&in[0], 1, sz, stdin);

	 ac->importRecords(in);
	 //				ac->getRecords((void*)&in[0], sz, result);

	 int n=0;
	 while (ac->hasRecord()) {
	 string s = ac->getRecord();
	 fwrite(&s[0], 1, s.size()+1, stdout);
	 n++;
	 }
	 fprintf(stderr,"%d\n",n);
	 fflush(stdout);
	 }

	 delete ac;
	 } catch(const char* c) {LOG("%s", c);}*/
	/*	auto *ac = new ArithmeticDecompressionStream<Order3Model>();

	 size_t sz;
	 while (fread(&sz, 1, sizeof(size_t), stdin)) {
	 vector<char> in(sz), result;
	 fread(&in[0], 1, sz, stdin);

	 ac->decompress((void*)&in[0], sz, result);

	 fwrite(&result[0], 1, result.size(), stdout);
	 fflush(stdout);
	 }

	 delete ac;
	 } catch(const char* c) {LOG("%s", c);}*/
	//}

	return 0;
}
