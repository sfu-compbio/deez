using namespace std;

template<typename T, typename TStream>
GenericCompressor<T, TStream>::GenericCompressor (int blockSize) {
	stream = new TStream();
	records.reserve(blockSize);
}

template<typename T, typename TStream>
GenericCompressor<T, TStream>::~GenericCompressor (void) {
	delete stream;
}

template<typename T, typename TStream>
void GenericCompressor<T, TStream>::addRecord (const T &rec) {
	records.push_back(rec);
}

template<typename T, typename TStream>
void GenericCompressor<T, TStream>::outputRecords (vector<char> &output) {
	if (records.size()) {
		output.clear();
		stream->compress(&records[0], records.size() * sizeof(records[0]), output);
		DEBUG("%d generics %s of size %d are flushed, compressed size %d", records.size(), typeid(T).name(), sizeof(T), output.size());
		records.erase(records.begin(), records.end());
	}
}

template<typename T, typename TStream>
GenericDecompressor<T, TStream>::GenericDecompressor (int blockSize): 
	recordCount (0) 
{
	stream = new TStream();
	records.reserve(blockSize);
}

template<typename T, typename TStream>
GenericDecompressor<T, TStream>::~GenericDecompressor (void) {
	delete stream;
}

template<typename T, typename TStream>
bool GenericDecompressor<T, TStream>::hasRecord (void) {
	return recordCount < records.size();
}

template<typename T, typename TStream>
const T &GenericDecompressor<T, TStream>::getRecord (void) {
	assert(hasRecord());
	return records[recordCount++];
}

template<typename T, typename TStream>
void GenericDecompressor<T, TStream>::importRecords (const vector<char> &input) {
	if (input.size() == 0) return;

	vector<char> c;
	stream->decompress((void*)&input[0], input.size(), c);

	assert(c.size() % sizeof(T) == 0);
	vector<T> tmp(c.size() / sizeof(T));
	memcpy(&tmp[0], &c[0], c.size());

	records.erase(records.begin(), records.begin() + recordCount);
	records.insert(records.end(), tmp.begin(), tmp.end());

	DEBUG("%d generics of size %d are loaded, %d erased, total %d", tmp.size(), sizeof(T), recordCount, records.size());
	recordCount = 0;
}

