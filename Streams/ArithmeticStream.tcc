#ifdef CODER_SUBBOTIN
#define  DO(n)	   for (int _=0; _<n; _++)
#define  TOP	   (1<<24)
#define  BOT	   (1<<16)
#endif

template<typename TModel>
ArithmeticCompressionStream<TModel>::ArithmeticCompressionStream (void) {
}

template<typename TModel>
ArithmeticCompressionStream<TModel>::~ArithmeticCompressionStream (void) {
}

template<typename TModel>
void ArithmeticCompressionStream<TModel>::setbit (uint32_t t) {
#ifdef CODER_SUBBOTIN
	out->push_back(t & 255);
#elif defined CODER_SHELWIEN
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
#else
	cOutput <<= 1;
	if (t) cOutput |= 1;
	if (!bufPos) {
		out->push_back(cOutput);
		bufPos = 7;
	}
	else
		bufPos--;
#endif
}

template<typename TModel>
void ArithmeticCompressionStream<TModel>::writeByte (char c) {
	if (model.active()) {
		uint32_t c_lo = model.getLow(c), c_hi = model.getHigh(c), p_sz = model.getSpan();

#ifdef CODER_SUBBOTIN
		uint32_t cumFreq = c_lo;
		uint32_t freq = c_hi - c_lo;
		uint32_t totFreq = p_sz;

		low += cumFreq * (range /= totFreq);
		range *= freq;
		while ((low ^ low + range) < TOP || range < BOT && ((range = -low & BOT - 1), 1))
		setbit(low >> 24), range <<= 8, low <<= 8;
#elif defined CODER_SHELWIEN
		uint32_t cumFreq = c_lo;
		uint32_t freq = c_hi - c_lo;
		uint32_t totFreq = p_sz;

		Range /= totFreq;
		Low += Range * cumFreq;
		Range *= freq;
		if (Range < 0x80000000) {
			Range <<= 32;
			setbit(0);
		}
#else
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
			else
				break;
			lo <<= 1;
			hi <<= 1;
			hi |= 1;
		}
#endif
	}
	else
		out->push_back(c);
	model.add(c);
}

template<typename TModel>
void ArithmeticCompressionStream<TModel>::flush (void) {
#ifdef CODER_SUBBOTIN
	DO(4) setbit(low >> 24), low <<= 8;
#elif defined CODER_SHELWIEN
	Low++;
	setbit(0);
	setbit(0);
	setbit(0);
#else
	setbit(lo & 0x40000000);
	underflow++;
	while (underflow) {
		setbit(~lo & 0x40000000);
		underflow--;
	}
	while (bufPos != 7)
		setbit(0);
#endif
}

template<typename TModel>
void ArithmeticCompressionStream<TModel>::compress (void *source, size_t sz, std::vector<char> &result) {
	out = &result;
	out->insert(out->end(), (char*)&sz, (char*)&sz + sizeof(size_t));

#ifdef CODER_SUBBOTIN
	low = 0;
	range = (uint32_t)-1;
#elif defined CODER_SHELWIEN
	rc_FFNum = rc_LowH = rc_Cache = rc_Carry = 0;
	Low = 0;
	Range = 0x7FFFFFFFFFFFFFFFll;
#else
	lo = 0;
	hi = -1;
	underflow = 0;
	bufPos = 7;
	cOutput = 0;
#endif

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
uint32_t ArithmeticDecompressionStream<TModel>::getbit (void) {
#ifdef CODER_SUBBOTIN
	return *(in++) & 255;
#elif defined CODER_SHELWIEN
	rc_Cache = *(uint32_t*)in; in += 4;
	Code = (Code << 32) + rc_Carry + (rc_Cache >> 1);
	rc_Carry = rc_Cache << 31;
#else
	char c = ((*in) >> bufPos) & 1;
	if (!bufPos) {
		in++;
		bufPos = 7;
	}
	else
		bufPos--;
	return (c ? 1 : 0);
#endif
}

template<typename TModel>
char ArithmeticDecompressionStream<TModel>::readByte (void) {
	char i;

	if (model.active()) {
		if (!initialized) {
#ifdef SUB
			DO(4) code = (code << 8) | getbit();
#elif defined CODER_SHELWIEN
			getbit();
			getbit();
			getbit();
#else
			for (int j = 0; j < 32; j++) {
				code <<= 1;
				code |= getbit();
			}
#endif

			initialized = true;
		}

		uint32_t span = model.getSpan();

#ifdef CODER_SUBBOTIN
		uint32_t count = (code - low) / (range /= span);
		uint32_t symbol = model.find(count);
		uint32_t cumFreq = model.getLow(symbol);
		uint32_t freq = model.getHigh(symbol) - model.getLow(symbol);

		low += cumFreq * range;
		range *= freq;
		while ((low ^ low + range) < TOP || range < BOT && ((range = -low & BOT - 1),1))
			code = (code << 8) | getbit(), range <<= 8, low <<= 8;
		i = symbol;
#elif defined CODER_SHELWIEN
		uint32_t count = Code / (Range / span);
		uint32_t symbol = model.find(count);
		uint32_t cumFreq = model.getLow(symbol);
		uint32_t freq = model.getHigh(symbol) - model.getLow(symbol);

		Range /= span;
		Code -= Range * cumFreq;
		Range *= freq;
		if (Range < 0x80000000) {
			Range <<= 32;
			getbit();
		}
		i = symbol;
#else
		uint64_t range = hi - lo + 1LL;
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
			else
				break;
			lo <<= 1;
			hi <<= 1;
			hi |= 1;
			code <<= 1;
			code |= getbit();
		}
		i = symbol;
#endif
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

#ifdef CODER_SUBBOTIN
	low = code = 0;
	range = (uint32_t)-1;
#elif defined CODER_SHELWIEN
	rc_FFNum = rc_LowH = rc_Cache = rc_Carry = 0;
	Range = 0x7FFFFFFFFFFFFFFFll;
	Code = 0;
#else
	lo = 0;
	hi = -1;
	bufPos = 7;
#endif

	initialized = false;
	for (size_t i = 0; i < x; i++)
		result[i] = readByte();
}
