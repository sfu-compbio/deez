using namespace std;

template<typename T, typename TStream>
GenericCompressor<T, TStream>::GenericCompressor (int blockSize):
	records(blockSize) 
{
	stream = new TStream();
}

template<typename T, typename TStream>
GenericCompressor<T, TStream>::~GenericCompressor (void) {
	delete stream;
}

template<typename T, typename TStream>
void GenericCompressor<T, TStream>::addRecord (const T &rec) {
	records.add(rec);
}

template<typename T, typename TStream>
// Resets out
// set out size to compressed size block
void GenericCompressor<T, TStream>::outputRecords (Array<uint8_t> &out) {
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	size_t s = stream->compress((uint8_t*)records.data(), records.size() * sizeof(T), 
		out, 0);
	out.resize(s);
	records.resize(0);
}

template<typename T, typename TStream>
// set out size to compressed size block + out size
void GenericCompressor<T, TStream>::appendRecords (Array<uint8_t> &out) {
	if (!records.size())
		return;

	size_t p = out.size();
	size_t s = stream->compress((uint8_t*)records.data(), records.size() * sizeof(T), 
		out, p);
	out.resize(p + s);
	records.resize(0);
}

template<typename T, typename TStream>
GenericDecompressor<T, TStream>::GenericDecompressor (int blockSize): 
	records(blockSize), 
	recordCount (0) 
{
	stream = new TStream();
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
	return records.data()[recordCount++];
}

template<typename T, typename TStream>
void GenericDecompressor<T, TStream>::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) 
		return;

	// move to the front ....   erase first recordCount
	if (recordCount < records.size())
		memmove(records.data(), records.data() + recordCount, (records.size() - recordCount) * sizeof(T));
	records.resize(records.size() - recordCount);

	// decompress
	Array<uint8_t> au;
	au.resize( 100000000 );
	size_t s = stream->decompress(in, in_size, au, 0);
	assert(s % sizeof(T) == 0);
	records.add((T*)au.data(), s / sizeof(T));
	
//	DEBUG("%lu generics of size %lu are loaded, %lu erased, total %lu", 
//		tmp.size(), sizeof(T), recordCount, records.size());
	recordCount = 0;
}

