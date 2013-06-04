using namespace std;

template<typename TStream>
StringCompressor<TStream>::StringCompressor (int blockSize):
	GenericCompressor<string, TStream>(blockSize) 
{
}

template<typename TStream>
void StringCompressor<TStream>::outputRecords (vector<char> &output) {
	if (records.size()) {
		string buffer = "";
		for (size_t i = 0; i < records.size(); i++) {
			buffer += records[i];
			buffer += '\0';
		}
		output.clear();
		stream->compress((void*)buffer.c_str(), buffer.size(), output);
		records.erase(records.begin(), records.begin() + records.size());
	}
}


template<typename TStream>
StringDecompressor<TStream>::StringDecompressor (int blockSize):
	GenericDecompressor<string, TStream>(blockSize) 
{
}

template<typename TStream>
void StringDecompressor<TStream>::importRecords (const vector<char> &input) {
	if (input.size() == 0) return;

	vector<char> c;
	stream->decompress((void*)&input[0], input.size(), c);

	records.erase(records.begin(), records.begin() + recordCount);

	string tmp = "";
	size_t pos = 0;
	records.clear();
	recordCount = 0;
	while (pos < c.size()) {
		if (c[pos] != 0)
			tmp += c[pos];
		else {
			records.push_back(tmp);
			tmp = "";
		}
		pos++;
	}
	assert(c.back() == 0);

	LOG("%d strings are loaded", records.size());
}

