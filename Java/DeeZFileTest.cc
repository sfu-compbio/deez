/// 786

#include <iostream>
#include <memory>

using namespace std;

#include <DeeZAPI.h>

int main (void)
{
	try {
		auto df = make_shared<DeeZFile>("../run/exp.sam.dztemp.dz", "../run/hs37d5.fa");
		//df->setLogLevel(1);

		int f = df->getFileCount();
		auto comment = df->getComment(0);
		auto records = df->getRecords("1:30000-40000");
		for (auto &r: records)
			cout << r.rname << " " << r.loc << endl;


		// df = make_shared<DeeZFile>("../run/74299.merged.sorted.nodups.realigned.recal.dz");
		// //df->setLogLevel(1);

		// f = df->getFileCount();
		// comment = df->getComment(0);
		// records = df->getRecords("1:100000-200000;2:200000-300000");
		// for (auto &r: records)
		// 	cout << r.rname << " " << r.loc << endl;

	}
	catch (DZException &e) {
		cerr << "Exception: " << e.what() << endl;
		throw;
	}

	return 0;
}
