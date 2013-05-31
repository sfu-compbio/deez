#include "Record.h"
using namespace std;

static const char* LOG_PREFIX = "<Record>";

Record::Record() {
    mappingSpanSize = 0;
    qlen = 0;
    isSafeToProcess = false;
    isProcessed = false;
}

void Record::setQueryName(const string & t) {
    qname = t;
}

string Record::getQueryName(void) const {
    return qname;
}

void Record::setQuerySeq(const string & t) {
    qseq = t;
}

string Record::getQuerySeq(void) const {
    return qseq;
}

void Record::setQueryQual(const string & t) {
    qqual = t;
}

string Record::getQueryQual(void) const {
    return qqual;
}

string Record::getQueryQualRev(void) const {
    return qqual;
    //TODO: is it worth doing this?
    string tmp;
    for (int i=qqual.length()-1;i>=0; i--)
        tmp += qqual[i];
    return tmp;
}

void Record::setMappingReference(const string & t) {
    mref = t;
}

string Record::getMappingReference(void) const {
    return mref;
}


void Record::setMappingLocation(int l) {
    mloc = l;
}

int Record::getMappingLocation(void) const {
    return mloc;
}


void Record::setMappingQuality(int q) {
    mqual = q;
}

int Record::getMappingQuality(void) const {
    return mqual;
}

void Record::setMappingFlag(int f) {
    mflag = f;
}

int Record::getMappingFlag(void) const {
    return mflag;
}

void Record::setMappingOperation(const string & s) {
    operation = s;
}

string Record::getMappingOperation(void) const {
    return operation;
}

void Record::setMateMappingReference(const string & s) {
    mmref = s;
}

string Record::getMateMappingReference(void) const {
    return mmref;
}

void Record::setMateMappingLocation(int l) {
    mmloc = l;
}

int Record::getMateMappingLocation(void) const {
    return mmloc;
}

void Record::setTemplateLength(int l) {
    tlen = l;
}

int Record::getTemplateLenght(void) const {
    return tlen;
}

void Record::setOptionalFieldKey(const string & s) {
    optionalKeys=s;
}

string Record::getOptionalFieldKeys(void) const {
    return optionalKeys+"$";
}

void Record::setOptionalFieldValue(const string & s) {
    optionalValues=s;
}

string Record::getOptionalFieldValues(void) const {
    return optionalValues+"$";
}

int Record::getQueryLength(void) const {
    return qlen;
}

int Record::getMappingSpanSize(void) const {
    return mappingSpanSize+qlen;
}

void Record::setIsSafeToProcess(bool v) {
    isSafeToProcess = v;
}

bool Record::getIsSafeToProcess(void) const {
    return isSafeToProcess;
}

void Record::setIsProcessed(bool v) {
    isProcessed = v;
}

bool Record::getIsProcessed(void) const {
    return isProcessed;
}

int Record::getMappingLocationEnd(void) const {
    return mappingSpanSize+mloc+qlen;
}

