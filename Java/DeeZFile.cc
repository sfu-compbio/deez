/// 786

#include <DeeZAPI.h>
#include "DeeZFile.h"
#include <stdio.h>

jfieldID getHandleField (JNIEnv *env, jobject obj)
{
	jclass c = env->GetObjectClass(obj);
	return env->GetFieldID(c, "nativeHandle", "J");
}

template <typename T>
T *getHandle (JNIEnv *env, jobject obj)
{
	jlong handle = env->GetLongField(obj, getHandleField(env, obj));
	return reinterpret_cast<T*>(handle);
}

template <typename T>
void setHandle(JNIEnv *env, jobject obj, T *t)
{
	jlong handle = reinterpret_cast<jlong>(t);
	env->SetLongField(obj, getHandleField(env, obj), handle);
}

void throwJavaException(JNIEnv *env, const char *msg)
{
	jclass c = env->FindClass("DeeZException");
	if (c == NULL) 
		c = env->FindClass("java/lang/NullPointerException");
	env->ThrowNew(c, msg);
}

/*******************************************************************************************************/

JNIEXPORT void JNICALL Java_DeeZFile_init (JNIEnv *env, jobject obj, jstring path, jstring genome)
{
	try {
		auto f = new DeeZFile(
			env->GetStringUTFChars(path, 0),
			env->GetStringUTFChars(genome, 0)
		);
		setHandle(env, obj, f);
	}
	catch (DZException &e) {
		throwJavaException(env, e.what());
	}
}

JNIEXPORT void JNICALL Java_DeeZFile_setLogLevel (JNIEnv *env, jobject obj, jint level)
{
	try {
		auto f = getHandle<DeeZFile>(env, obj);
		f->setLogLevel(level);
	}
	catch (DZException &e) {
		throwJavaException(env, e.what());
	}
}

JNIEXPORT jint JNICALL Java_DeeZFile_getFileCount (JNIEnv *env, jobject obj)
{
	try {
		auto f = getHandle<DeeZFile>(env, obj);
		return (jint)f->getFileCount();
	}
	catch (DZException &e) {
		throwJavaException(env, e.what());
	}
}

JNIEXPORT jstring JNICALL Java_DeeZFile_getComment (JNIEnv *env, jobject obj, jint file)
{
	try {
		auto f = getHandle<DeeZFile>(env, obj);
		return env->NewStringUTF(f->getComment(file).c_str());
	}
	catch (DZException &e) {
		throwJavaException(env, e.what());
	}
}

JNIEXPORT jobjectArray JNICALL Java_DeeZFile_getRecords (JNIEnv *env, jobject obj, jstring range, jint flags)
{
	auto cls = env->FindClass("DeeZFile$SAMRecord");
	auto ctor = env->GetMethodID(cls, "<init>", "(LDeeZFile;Ljava/lang/String;ILjava/lang/String;JILjava/lang/String;Ljava/lang/String;JILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

	try {
		auto f = getHandle<DeeZFile>(env, obj);
		auto records = f->getRecords(env->GetStringUTFChars(range, 0), flags);

		auto arr = env->NewObjectArray(records.size(), cls, NULL);
		for (int i = 0; i < records.size(); i++) {
			auto &rec = records[i];
			
			auto rname = env->NewStringUTF(rec.rname.c_str());
			jint flag = rec.flag;
			auto chr = env->NewStringUTF(rec.chr.c_str());
			jlong loc = rec.loc;
			jint mapqual = rec.mapqual;
			auto cigar = env->NewStringUTF(rec.cigar.c_str());
			auto pchr = env->NewStringUTF(rec.pchr.c_str());
			jlong ploc = rec.ploc;
			jint tlen = rec.tlen;
			auto seq = env->NewStringUTF(rec.seq.c_str());
			auto qual = env->NewStringUTF(rec.qual.c_str());
			auto opt = env->NewStringUTF(rec.opt.c_str());

	    	auto item = env->NewObject(cls, ctor, obj, rname, flag, chr, loc, mapqual, cigar, pchr, ploc, tlen, seq, qual, opt);
			env->SetObjectArrayElement(arr, i, item);
		}
		return arr;
	}
	catch (DZException &e) {
		throwJavaException(env, e.what());
		return NULL;
	}
}

JNIEXPORT void JNICALL Java_DeeZFile_dispose (JNIEnv *env, jobject obj)
{
	try {
		auto f = getHandle<DeeZFile>(env, obj);
	    setHandle<DeeZFile>(env, obj, 0);
	    delete f;
  	}
	catch (DZException &e) {
		throwJavaException(env, e.what());
	}
}
