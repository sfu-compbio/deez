#include "SAMComment.h"
using namespace std;

SAMComment::SAMComment(const string &comment)
{
	lines = split(comment, '\n');
	for (auto &l: lines) {
		auto f = split(l, '\t');
		if (!f.size()) continue;

		unordered_map<string, string> kv;
		for (auto &ff: f) {
			if (ff.size() > 3) {
				kv[ff.substr(0, 2)] = ff.substr(3);
			} else {
				kv[""] = ff;
			}
		}

		string id = f[0];
		if (id == "@SQ" && kv.find("SN") != kv.end()) {
			SQ[kv["SN"]] = kv;
		} else if (id == "@RG" && kv.find("ID") != kv.end()) {
			int cnt = RG.size();
			RG[kv["ID"]] = cnt;
		} else if (id == "@PG" && kv.find("ID") != kv.end()) {
			int cnt = PG.size();
			PG[kv["ID"]] = cnt;
		}
	}

	// for (auto &c: SQ) {
	// 	LOGN("%s: ", c.first.c_str());
	// 	for (auto &e: c.second)
	// 		LOGN("%s->%s ", e.first.c_str(), e.second.c_str());
	// 	LOG("");
	// }
	// for (auto &c: RG) {
	// 	LOG("%s->%d ", c.first.c_str(), c.second);
	// }
	// for (auto &c: PG) {
	// 	LOG("%s->%d ", c.first.c_str(), c.second);
	// }
}
