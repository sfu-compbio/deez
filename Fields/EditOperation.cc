#include "EditOperation.h"
using namespace std;

// N how many before
/*template<typename T, int AS, int N>
class OrderNModel: public CodingModel {
	uint32_t *cumul;
	uint32_t *freq;

	size_t seenSoFar;

	uint32_t sz;
	uint32_t pos;

	char history[N];
	char historyPos;

public:
	OrderNModel (void):
		seenSoFar(0),
		historyPos(0),
		pos(0),
		sz(1)
	{
		for (int i = 0; i < N; i++)
			sz *= AS;
		cumul = new uint64_t[sz];
		freq  = new uint64_t[sz * AS];

		for (int i = 0; i < sz; i++)
			freq[i] = 1;
	}

	~OrderNModel (void) {
		delete[] cumul;
		delete[] freq;
	}

public:
	bool active (void) const {
		return seenSoFar >= N;
	}

	uint32_t getLow (T c) const {
		uint32_t lo = 0;
		for (int i = 0; i < T; i++)
			lo += freq[pos * AS + i];
		return lo;
	}

	uint32_t getFreq (T c) const {
		return freq[pos * AS + c];
	}

	uint32_t getSpan (void) const {
		return cumul[pos];
	}

	void add (T c) {
		freq[pos * AS + c]++;
		cumul[pos]++;

		if (cumul[pos] == -1) {
			throw "not implemented";
		}

		pos -= history[historyPos] * sz;
		pos += c;

		history[historyPos] = c;
		historyPos = (historyPos + 1) % N;

		seenSoFar++;
	}

	T find (uint32_t cnt) {
		uint32_t hi = 0;
		for (size_t i = 0; i < AS; i++) {
			hi += freq[pos * AS + i];
			if (hi > cnt)
				return i;
		}
		throw "AC find failed";
	}
};

typedef OrderNModel<short, 255, 6> LenModel;
typedef OrderNModel<char, 10,	6> KeyModel;
typedef OrderNModel<char, 5, 10> SeqModel;

EditOperationCompressor::EditOperationCompressor (int blockSize) {
	stream = new ArithmeticCompressionStream<short, LenModel>();
	editKeyStream = new ArithmeticCompressionStream<char, KeyModel>();
	editValStream = new GzipCompressionStream<6>();
	sequenceStream = new ArithmeticCompressionStream<char, SeqModel>();

	lengths.reserve(blockSize);
	keys.reserve(2 * blockSize);
	values.reserve(2 * blockSize);
}

EditOperationCompressor::~EditOperationCompressor (void) {
	delete editKeyStream;
	delete editValStream;
	delete sequenceStream;
	delete stream;
}

void EditOperationCompressor::addRecord (const CigarOp &cop) {
	if (cop.keys[0] == '*') {
		lengths.push_back(cop.keys.size() - 1);
		sequences += cop.keys.substr(0, cop.keys.size() - 1);
	}

	lengths.push_back(cop.keys.size());
	for (size_t i = 0; i < cop.keys.size(); i++) {
		keys.push_back(cop.keys[i]);
		values.push_back(cop.keys[i]);
	}
}

void EditOperationCompressor::outputRecords (std::vector<char> &output) {
	if (lengths.size()) {
		output.clear();

		//DEBUG("%d corrections, %d records flushed", corrections.size(), records.size());

		output.push_back(0);

		size_t sz[3];

		stream->compress(&lengths[0], lengths.size() * sizeof(lengths[0]), output);
		sz[0] = output.size();

		editKeyStream->compress(&keys[0], keys.size() * sizeof(keys[0]), output);
		sz[1] = output.size() - sz[0];

		editValStream->compress(&values[0], values.size() * sizeof(values[0]), output);
		sz[2] = output.size() - sz[0] - sz[1];

		output.insert(output.begin(), (char*)sz, (char*)(sz + sizeof(sz)));

		lengths.erase(lengths.begin(), lengths.end());
		keys.erase(keys.begin(), keys.end());
		values.erase(values.begin(), values.end());
	}
	else if (sequences.size()) {
		output.push_back(1);

		size_t sz[2];

		stream->compress(&lengths[0], lengths.size() * sizeof(lengths[0]), output);
		sz[0] = output.size();

		sequenceStream->compress(&sequences[0], sequences.size() * sizeof(sequences[0]), output);
		sz[1] = output.size() - sz[0];

		output.insert(output.begin(), (char*)sz, (char*)(sz + sizeof(sz)));

		sequences = "";
		lengths.erase(lengths.begin(), lengths.end());
	}
}
*/
