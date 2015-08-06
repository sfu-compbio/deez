#ifndef AC2Stream_H
#define AC2Stream_H

#include <vector>
#include "../Common.h"
#include "Order0Stream.h"	

template<int AS>
class AC2CompressionStream: public CompressionStream, public DecompressionStream {
	static const int MOD_SZ = AS * AS;
	AC0CompressionStream<AS> mod[MOD_SZ];

	uint8_t loq, hiq;
	uint8_t q1;
	uint32_t context;

public:
	AC2CompressionStream (void) {
		q1 = loq = hiq = 0;
		context = 0;
	}

private:
	void encode (uint8_t q, AC *ac) {
		assert(q < AS);
		assert(context < MOD_SZ);
		mod[context].encode(q, ac);
		
		// squeeze index
		loq = min(loq, q);
		hiq = max(hiq, q);

		context  = q1 * AS;
		context += q;
		q1 = q;
	}

	uint8_t decode (AC *ac) {
		assert(context < MOD_SZ);
		uint8_t q = mod[context].decode(ac);

		context  = q1 * AS;
		context += q;
		q1 = q;

		loq = min(loq, q);
		hiq = max(hiq, q);
		
		return q;
	}

public:
	size_t compress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) 
	{	
		// ac keeps appending to the array.
		// thus, just resize dest
		dest.resize(dest_offset + sizeof(size_t));
		memcpy(dest.data() + dest_offset, &source_sz, sizeof(size_t));
		ACType *ac = new ACType();
		ac->initEncode(&dest);
		for (size_t i = 0; i < source_sz; i++)
			encode(source[i], ac);
		ac->flush();
		this->compressedCount += dest.size() - dest_offset;
		delete ac;
		return dest.size() - dest_offset;
		//return 
	}

	//string phteven;
	size_t decompress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) 
	{	
		// ac keeps appending to the array.
		// thus, just resize dest

		/*if (phteven=="") phteven="_frabug0";
		FILE *fi=fopen(string(phteven + "_AC").c_str(), "w");
		fprintf(fi, "STA %d %d %d CTX %d\n", loq, hiq, q1, context);
		for (int i = 0; i < AS; i++)
			for (int j = 0; j < AS; j++) {
				fprintf(fi, "%d,%d | SUM %lu ", i, j, mod[i * AS + j].sum);
				for (int k = 0; k < AS; k++)
					fprintf(fi, "(%d %d) ", mod[i * AS + j].stats[k].sym, mod[i * AS + j].stats[k].freq);
				fprintf(fi, "\n");
			}
		fclose(fi);
		fi=fopen(string(phteven + "_QD").c_str(), "w");
		fprintf(fi, "DEC %lu %lu\n", source_sz, dest_offset);
		for (int i = 0; i < source_sz; i++)
			fprintf(fi, "%x", source[i] & 0xff);
		fprintf(fi, "\n");
		fclose(fi);
		phteven[phteven.size()-1]++;*/

		size_t num = *((size_t*)source);
		ACType *ac = new ACType();
		ac->initDecode(source + sizeof(size_t), source_sz);
		dest.resize(dest_offset + num);
		for (size_t i = 0; i < num; i++) {
			*(dest.data() + dest_offset + i) = decode(ac);
		}
		return num;
	}

	void getCurrentState (Array<uint8_t> &ou) {
		ou.add(loq);
		ou.add(hiq);
		for (int i = loq; i <= hiq; i++) 
			for (int j = loq; j <= hiq; j++)
				mod[i * AS + j].getCurrentState(ou);
		ou.add(q1);
		ou.add((uint8_t*)&context, sizeof(uint32_t));
	//	printf("<<%d,%d,%d,%u    %d>>\n",q1,context, loq,hiq,ou.size());
	}

	void setCurrentState (uint8_t *in, size_t sz) {
	//	LOG("Setting state");
		loq = *in++;
		hiq = *in++;

		//size_t eha=0;
		for (int i = loq; i <= hiq; i++) 
			for (int j = loq; j <= hiq; j++) {
				// first dictates
				// uint8_t t = *in;
				size_t szx = AS * (1 + sizeof(uint16_t));
				mod[i * AS + j].setCurrentState(in, szx);
				in += szx;
				//eha += szx;
			}
		q1 = *in++;
		context = *(uint32_t*)in;
	//	LOG("---> %d %d %d %d %d %d\n", sz, eha + 7, loq, hiq, q1, context);
	}
};

#define AC2DecompressionStream AC2CompressionStream

#endif // AC2Stream_H
