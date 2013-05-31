#ifndef Record_H
#define Record_H

#include <string>
#include "../Common.h"

class Record {
    std::string	qname;           // query name
    std::string	qqual;           // query quality
    std::string	qseq;            // query sequence
    std::string	mref;            // mmapping reference
    std::string 	operation;       // mapping operation
    std::string 	mmref;           // mate mapping reference
    int 		mflag;              // mapping flag
    int 		mloc;               // mapping location begin
    int 		mqual;              // mmaping quality
    int 		mmloc;              // mate mapping location
    int 		tlen;               // template length
    int 		mappingSpanSize;    // size of the span on the genome
    int 		qlen;               // query length
    std::string 	optionalKeys;    // optional fields
    std::string 	optionalValues;
    bool 	isSafeToProcess;
    bool 	isProcessed;

public:
    Record();

public:
    void setQueryName(const std::string &);
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
    void setOptionalFieldKey(const std::string &);
    void setOptionalFieldValue(const std::string &);
    void setIsSafeToProcess(bool);
    void setIsProcessed(bool);

    std::string 	getQueryName() const;
    std::string 	getQuerySeq() const;
    std::string 	getQueryQual() const;
    std::string 	getQueryQualRev() const;
    std::string 	getMappingReference() const;
    int 		getMappingLocation() const;
    int 		getMappingFlag() const;
    int 		getMappingQuality() const;
    std::string 	getMappingOperation() const;
    std::string 	getMateMappingReference() const;
    int 		getMateMappingLocation() const;
    int 		getTemplateLenght() const;
    std::string 	getOptionalFieldKeys() const;
    std::string 	getOptionalFieldValues() const;
    int 		getQueryLength() const;
    int 		getMappingSpanSize() const;
    bool 	getIsSafeToProcess() const;
    bool 	getIsProcessed() const;
    int 		getMappingLocationEnd() const;

    void print();
};

#endif // RECORD_H
