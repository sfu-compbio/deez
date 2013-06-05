using namespace std;

template<typename TStream>
StringCompressor<TStream>::StringCompressor (int blockSize):
	GenericCompressor<string, TStream>(blockSize) 
{
}

template<typename TStream>
void StringCompressor<TStream>::outputRecords (vector<char> &output) {
	if (this->records.size()) {
		string buffer = "";
		for (size_t i = 0; i < this->records.size(); i++) {
			buffer += this->records[i];
			buffer += '\0';
		}
		output.clear();
		this->stream->compress((void*)buffer.c_str(), buffer.size(), output);
		this->records.erase(this->records.begin(), this->records.begin() + this->records.size());
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
	this->stream->decompress((void*)&input[0], input.size(), c);

	this->records.erase(this->records.begin(), this->records.begin() + this->recordCount);

	string tmp = "";
	size_t pos = 0;
	this->records.clear();
	this->recordCount = 0;
	while (pos < c.size()) {
		if (c[pos] != 0)
			tmp += c[pos];
		else {
			this->records.push_back(tmp);
			tmp = "";
		}
		pos++;
	}
	assert(c.back() == 0);

	LOG("%d strings are loaded", this->records.size());
}

