LIBRARY := libdeez-jni.so
OBJFILES := DeeZFile.o
JAVA_HOME ?= /usr/lib/jvm/java
INCLUDES := -I$(JAVA_HOME) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -I..
CXXFLAGS := $(INCLUDES) -O3 -std=c++11 -fpic -msse4.1

$(LIBRARY): $(OBJFILES)
	$(CXX) -O3 -shared -pthread -o $@ $^ ../libdeez.a

lib-prepare:
	make -j -B -C.. lib

javatest: $()
	javac *.java

cpptest: 
	$(CXX) -std=c++11 -msse4.1 DeeZFileTest.cc -o DeeZFileTest -I.. -g -pthread -L.. -ldeez

clean:
	rm -rf *.so *.o *.class DeeZFileTest
