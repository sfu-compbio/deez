/// 786

#include <iostream>
#include <memory>

using namespace std;

#include <DeeZAPI.h>

int main (void)
{
	try {
		auto df = make_shared<DeeZFile>("../run/T1-1B.rmdup.realigned.bam.dztemp.dz", "../run/human_g1k_v37.fasta");
		df->setLogLevel(1);

		int f = df->getFileCount();
		auto comment = df->getComment(0);
		auto records = df->getRecords("1:30000-40000000");
		while (records.size()) {
			cerr << "Block!" << endl;
			for (auto &r: records)
				cout << r.rname << " " << r.loc << endl;
			records = df->getRecords();
		}
	}
	catch (DZException &e) {
		cerr << "Exception: " << e.what() << endl;
		throw;
	}

	return 0;
}
