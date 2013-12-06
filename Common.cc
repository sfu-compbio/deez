#include <vector>
#include <string>
using namespace std;

string int2str (int k) {
	string s = "";
	while (k) {
		s = char(k % 10 + '0') + s;
		k /= 10;
	}
	return s;
}

string inttostr (int k) {
    static vector<std::string> mem;
    if (mem.size() == 0) {
    	mem.resize(10001);
		mem[0] = "0";
		for (int i = 1; i < 10000; i++)
			mem[i] = int2str(i);
    }

    if (k < 10000)
    	return mem[k];
    else
    	return int2str(k);
}

char getDNAValue (char ch) {
	static char c[128] = {
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 0 15
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 16 31
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 5, // 32 47
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 48 63
		5, 1, 5, 2, 5, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5, // 64 79
		5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 80 95
		5, 1, 5, 2, 5, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5, // 96 111
		5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 112 127
	};	
	return c[ch];
}
