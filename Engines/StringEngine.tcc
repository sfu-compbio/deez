using namespace std;

template<typename TStream>
StringCompressor<TStream>::StringCompressor (int blockSize):
	GenericCompressor<char, TStream>(blockSize * 100) 
{
	this->records.set_extend(100 * 100);
}

template<typename TStream>
StringCompressor<TStream>::~StringCompressor (void) {
}

template<typename TStream>
void StringCompressor<TStream>::addRecord (const string &rec) {
	// this->records.resize(this->records.size() + rec.size() + 1);
	this->records.add(rec.c_str(), rec.size() + 1);
	// memcpy(this->records.data() + st, rec.c_str(), rec.size() + 1);
}


template<typename TStream>
StringDecompressor<TStream>::StringDecompressor (int blockSize):
	GenericDecompressor<char, TStream>(blockSize) 
{
}

template<typename TStream>
StringDecompressor<TStream>::~StringDecompressor (void) {
}

template<typename TStream>
const char *StringDecompressor<TStream>::getRecord (void) {
	assert(this->hasRecord());
	char *c = this->records.data() + this->recordCount;
	
	while (*(this->records.data() + this->recordCount)) 
		this->recordCount++;
	this->recordCount++;

	return c;
}
