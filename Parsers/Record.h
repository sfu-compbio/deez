#ifndef Record_H
#define Record_H

#include <string>
#include "../Common.h"
//#include "SAMParser.h"

const int MAXLEN = 8 * KB;
class Record {
    char	qname[MAXLEN];           // query name
    char	qqual[MAXLEN];           // query quality
    char	qseq[MAXLEN];            // query sequence
    char	mref[MAXLEN];            // mmapping reference
    char operation[MAXLEN];       // mapping operation
    char mmref[MAXLEN];           // mate mapping reference
	 char optional[MAXLEN];
    int 		mflag;              // mapping flag
    int 		mloc;               // mapping location begin
    int 		mqual;              // mmaping quality
    int 		mmloc;              // mate mapping location
    int 		tlen;               // template length
    int 		mappingSpanSize;    // size of the span on the genome
    int 		qlen;               // query length

	 friend class SAMParser;
    
public:
    Record();

public:
/*    void setQueryName(const std::string &);
    void setQuerySeq(const std::string &);
    void setQueryQual(const std::string &);
    void setMappingReference(const std::string &);
    void setMappingLocation(int);
    void setMappingFlag(int);
    void setMappingQuality(int);
    void setMappingOperation(const std::string &);
    void setMateMappingReference(const std::string &);
    void setMateMappingLocation(int);
    void setTemplateLength(int);
    void setOptional(const std::string &);*/
    
    const char* 	getQueryName() const;
    const char* 	getQuerySeq() const;
    const char* 	getQueryQual() const;
    const char* 	getMappingReference() const;
    int 		getMappingLocation() const;
    int 		getMappingFlag() const;
    int 		getMappingQuality() const;
    const char* 	getMappingOperation() const;
    const char* 	getMateMappingReference() const;
    int 		getMateMappingLocation() const;
    int 		getTemplateLenght() const;
    const char* 	getOptional() const;
    int 		getQueryLength() const;
    int 		getMappingSpanSize() const;
    bool 	getIsSafeToProcess() const;
    bool 	getIsProcessed() const;
    int 		getMappingLocationEnd() const;

    void print();
};

#endif // RECORD_H
