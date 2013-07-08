template<typename TModel>
ArithmeticCompressionStream<TModel>::ArithmeticCompressionStream (void) {
}

template<typename TModel>
ArithmeticCompressionStream<TModel>::~ArithmeticCompressionStream (void) {
}

template<typename TModel>
void ArithmeticCompressionStream<TModel>::setbit (uint32_t t) {
	rc_Carry = (Low >> 63) & 1;
	Low &= 0x7FFFFFFFFFFFFFFFll;
	rc_LowH = Low >> 31;
	if (rc_Carry == 1 || rc_LowH < 0xFFFFFFFF) {
		uint32_t o = rc_Cache + rc_Carry;
		out->insert(out->end(), (char*)&o, (char*)&o + sizeof(uint32_t));
		for (; rc_FFNum > 0; rc_FFNum--) {
			o = 0xFFFFFFFF + rc_Carry;
			out->insert(out->end(), (char*)&o, (char*)&o + sizeof(uint32_t));
		}
		rc_Cache = rc_LowH;
	}
	else rc_FFNum++;
	Low = (Low & 0x7FFFFFFF) << 32;
}

template<typename TModel>
void ArithmeticCompressionStream<TModel>::writeByte (char c) {
	if (model.active()) {
		uint32_t cumFreq = model.getLow(c);
		uint32_t freq = model.getFreq(c);
		uint32_t totFreq = model.getSpan();

		Range /= totFreq;
		Low += Range * cumFreq;
		Range *= freq;
		if (Range < 0x80000000) {
			Range <<= 32;
			setbit(0);
		}
	}
	else out->push_back(c);
	model.add(c);
}

template<typename TModel>
void ArithmeticCompressionStream<TModel>::flush (void) {
	Low++;
	setbit(0);
	setbit(0);
	setbit(0);
}

template<typename TModel>
void ArithmeticCompressionStream<TModel>::compress (void *source, size_t sz, std::vector<char> &result) {
	out = &result;
	out->insert(out->end(), (char*)&sz, (char*)&sz + sizeof(size_t));

	rc_FFNum = rc_LowH = rc_Cache = rc_Carry = 0;
	Low = 0;
	Range = 0x7FFFFFFFFFFFFFFFll;
	for (size_t i = 0; i < sz; i++)
		writeByte(((char*)source)[i]);
	flush();
}


template<typename TModel>
ArithmeticDecompressionStream<TModel>::ArithmeticDecompressionStream (void) {
}

template<typename TModel>
ArithmeticDecompressionStream<TModel>::~ArithmeticDecompressionStream (void) {
}

template<typename TModel>
void ArithmeticDecompressionStream<TModel>::getbit (void) {
	rc_Cache = *(uint32_t*)in; in += sizeof(uint32_t);
	Code = (Code << 32) + rc_Carry + (rc_Cache >> 1);
	rc_Carry = rc_Cache << 31;
}

template<typename TModel>
char ArithmeticDecompressionStream<TModel>::readByte (void) {
	char i;

	if (model.active()) {
		if (!initialized) {
			getbit(), getbit(), getbit();
			initialized = true;
		}

		uint32_t count = Code / (Range /= model.getSpan());
		uint32_t symbol = model.find(count);
		uint32_t cumFreq = model.getLow(symbol);
		uint32_t freq = model.getFreq(symbol);

		Code -= Range * cumFreq;
		Range *= freq;
		if (Range < 0x80000000) {
			Range <<= 32;
			getbit();
		}
		i = symbol;
	}
	else i = *(in++);

	model.add(i);
	return i;
}

template<typename TModel>
void ArithmeticDecompressionStream<TModel>::decompress (void *source, size_t sz, std::vector<char> &result) {
	in = (char*)source + sizeof(size_t);

	size_t x = *(size_t*)source;
	result.resize(x);

	rc_FFNum = rc_LowH = rc_Cache = rc_Carry = 0;
	Range = 0x7FFFFFFFFFFFFFFFll;
	Code = 0;
	initialized = false;
	for (size_t i = 0; i < x; i++)
		result[i] = readByte();
}
