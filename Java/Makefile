LIBRARY := libdeez-jni.so
OBJFILES := DeeZFile.o
JAVA_HOME ?= /usr/lib/jvm/java
INCLUDES := -I$(JAVA_HOME) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -I..
CXXFLAGS := $(INCLUDES) -std=c++0x -fpic -msse4.1

$(LIBRARY): $(OBJFILES)
	$(CXX) -O3 -shared -L.. -pthread -ldeez -o $@ $^ 

lib-prepare:
	make -j -B -C.. lib
	ln -f -s ../libdeez.so .

javatest: $()
	javac *.java

cpptest: 
	$(CXX) -std=c++11 DeeZFileTest.cc -o DeeZFileTest -I.. -g -pthread -L.. -ldeez

clean:
	rm -rf *.so *.o *.class DeeZFileTest
