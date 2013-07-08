#include "Record.h"
using namespace std;

static const char* LOG_PREFIX = "<Record>";

Record::Record() {
    mappingSpanSize = 0;
    qlen = 0;
}

/*void Record::setQueryName(const const char* & t) {
    qname = t;
}*/

const char* Record::getQueryName(void) const {
    return qname;
}
/*
void Record::setQuerySeq(const const char* & t) {
    qseq = t;
}
*/
const char* Record::getQuerySeq(void) const {
    return qseq;
}

/*
void Record::setQueryQual(const const char* & t) {
    qqual = t;
}
*/
const char* Record::getQueryQual(void) const {
    return qqual;
}
/*
void Record::setMappingReference(const const char* & t) {
    mref = t;
}
*/
const char* Record::getMappingReference(void) const {
    return mref;
}

/*
void Record::setMappingLocation(int l) {
    mloc = l;
}
*/
int Record::getMappingLocation(void) const {
    return mloc;
}

/*
void Record::setMappingQuality(int q) {
    mqual = q;
}
*/
int Record::getMappingQuality(void) const {
    return mqual;
}
/*
void Record::setMappingFlag(int f) {
    mflag = f;
}
*/
int Record::getMappingFlag(void) const {
    return mflag;
}
/*
void Record::setMappingOperation(const const char* & s) {
    operation = s;
}
*/
const char* Record::getMappingOperation(void) const {
    return operation;
}
/*
void Record::setMateMappingReference(const const char* & s) {
    mmref = s;
}
*/
const char* Record::getMateMappingReference(void) const {
    return mmref;
}
/*
void Record::setMateMappingLocation(int l) {
    mmloc = l;
}*/

int Record::getMateMappingLocation(void) const {
    return mmloc;
}
/*
void Record::setTemplateLength(int l) {
    tlen = l;
}
*/
int Record::getTemplateLenght(void) const {
    return tlen;
}
/*
void Record::setOptional(const const char* & s) {
    optional=s;
}
*/
const char* Record::getOptional(void) const {
    return optional;
}

int Record::getQueryLength(void) const {
    return qlen;
}

int Record::getMappingSpanSize(void) const {
    return mappingSpanSize+qlen;
}

int Record::getMappingLocationEnd(void) const {
    return mappingSpanSize+mloc+qlen;
}

