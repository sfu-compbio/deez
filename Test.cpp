#include <cstdio>
#include <vector>
#include <clocale>
//#include "Streams/ArithmeticStream.h"
#include "Streams/GzipStream.h"
#include "Streams/ContextModels/Order3Model.h"
#include "Streams/ContextModels/Order3Model.cc"

#include "Engines/StringEngine.h"

using namespace std;

const int bfsz = 100000000;
char bf[bfsz];

int main (int argc, char **argv) {
	setlocale(LC_ALL, "");
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
	if (argv[1][0] == 'd') {
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
		} catch(const char* c) {LOG("%s", c);}
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
	}

	return 0;
}
