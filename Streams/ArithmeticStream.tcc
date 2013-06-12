template<typename TModel>
void ArithmeticCompressionStream<TModel>::setbit (int t) {
	cOutput <<= 1;
	if (t) cOutput |= 1;
	if (!bufPos) {
		out->push_back(cOutput);
		bufPos = 7;
	// cOutput = 0;
	}
	else bufPos--;
}

template<typename TModel>
void ArithmeticCompressionStream<TModel>::writeByte (char c) {
	if (model.active()) {
		uint32_t c_lo = model.getLow(c),
				 c_hi = model.getHigh(c),
				 p_sz = model.getSpan();
		uint64_t range = hi - lo + 1LL;
		hi = lo + (range * c_hi) / p_sz - 1;
		lo = lo + (range * c_lo) / p_sz;
		while (1) {
			if ((hi & 0x80000000) == (lo & 0x80000000)) {
				setbit(hi & 0x80000000);
				while (underflow) {
					setbit(~hi & 0x80000000);
					underflow--;
				}
			}
			else if (!(hi & 0x40000000) && (lo & 0x40000000)) { // underflow ante portas
				underflow++;
				lo &= 0x3fffffff; // set 2nd highest digit to 0 -- 1st will be removed after
				hi |= 0x40000000; // set 2nd highest digit to 1
			}
			else break;
			lo <<= 1;
			hi <<= 1; hi |= 1;
		}
	}
	else
		out->push_back(c);
	model.add(c);
}

template<typename TModel>
void ArithmeticCompressionStream<TModel>::flush (void) {
	setbit(lo & 0x40000000);
	underflow++;
	while (underflow) {
		setbit(~lo & 0x40000000);
		underflow--;
	}
	while (bufPos != 7)
		setbit(0);
}

template<typename TModel>
ArithmeticCompressionStream<TModel>::ArithmeticCompressionStream (void) {
}

template<typename TModel>
ArithmeticCompressionStream<TModel>::~ArithmeticCompressionStream (void) {
}

template<typename TModel>
void ArithmeticCompressionStream<TModel>::compress (void *source, size_t sz, std::vector<char> &result) {
	out = &result;
	out->insert(out->end(), (char*)&sz, (char*)&sz + sizeof(size_t));

	lo = 0;
	hi = -1;
	underflow = 0;
	bufPos = 7;
	cOutput = 0;
	for (size_t i = 0; i < sz; i++)
		writeByte(((char*)source)[i]);
	flush();
	DEBUG("in %'llu out %'llu", sz, out->size());
}


template<typename TModel>
ArithmeticDecompressionStream<TModel>::ArithmeticDecompressionStream (void) {
}

template<typename TModel>
ArithmeticDecompressionStream<TModel>::~ArithmeticDecompressionStream (void) {
}

template<typename TModel>
char ArithmeticDecompressionStream<TModel>::getbit (void) {
	char c = ((*in) >> bufPos) & 1;
	if (!bufPos) {
		in++;
		bufPos = 7;
	}
	else
		bufPos--;
	return (c ? 1 : 0);
}

template<typename TModel>
char ArithmeticDecompressionStream<TModel>::readByte (void) {
	char i;

	if (model.active()) {
		if (!initialized) {
			//code = *(uint32_t*)in; in += sizeof(uint32_t);
			for (int j = 0; j < 32; j++) {
				code <<= 1;
				code |= getbit();
			}
			initialized = true;
		}

		uint64_t range = hi - lo + 1LL;
		uint32_t span = model.getSpan();
		uint32_t count = ((code - lo + 1LL) * span - 1LL) / range;
		uint32_t symbol = model.find(count);

		hi = lo + (range * model.getHigh(symbol)) / span - 1;
		lo = lo + (range * model.getLow(symbol)) / span;
		while (1) {
			if ((hi & 0x80000000) == (lo & 0x80000000)) 
				;
			else if (!(hi & 0x40000000) && (lo & 0x40000000)) { // underflow ante portas
				code ^= 0x40000000;
				lo &= 0x3fffffff;
				hi |= 0x40000000;
			}
			else break;
			lo <<= 1;
			hi <<= 1; hi |= 1;
			code <<= 1; code |= getbit();
		}
		i = symbol;
	}
	else
		i = *(in++);

	model.add(i);
	return i;
}

template<typename TModel>
void ArithmeticDecompressionStream<TModel>::decompress (void *source, size_t sz, std::vector<char> &result) {
	in = (char*)source + sizeof(size_t);

	size_t x = *(size_t*)source;
	result.resize(x);
	DEBUG("in %'llu out %'llu", sz, x);

	lo = 0;
	hi = -1;
	bufPos = 7;
	initialized = false;
	for (size_t i = 0; i < x; i++)
		result[i] = readByte();
}
