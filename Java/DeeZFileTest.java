/// 786

public class DeeZFileTest {
	public static void main (String[] args) {
		try {
			DeeZFile df = new DeeZFile("test.dz", "test.fa");
			//df.setLogLevel(1);

			int f = df.getFileCount();
			String comment = df.getComment(0);
			
			//System.out.println(f);
			//System.out.println(comment);

			DeeZFile.SAMRecord[] records = df.getRecords("1:30000-32000");
			while (records.length > 0) { 
				System.err.println("Block");
				for (DeeZFile.SAMRecord r: records) {
					System.out.println(r.rname + " at " + r.loc + ": " + r.optmap.toString());
				}
				records = df.getRecords();
			}

			df.dispose();

			// df = new DeeZFile("../run/74299.merged.sorted.nodups.realigned.recal.dz");
			// f = df.getFileCount();
			// comment = df.getComment(0);
			
			// System.out.println(f);
			// System.out.println(comment);

			// records = df.getRecords("1:100000-200000;2:200000-300000");
			// while (records.size() > 0) { 
			// 	for (DeeZFile.SAMRecord r: records) 
			// 		System.out.println(r.rname + " at " + r.loc + ": " + r.optmap.toString());
			// 	records = df.getRecords();
			// }

			// df.dispose();
		}
		catch (DeeZException e) {
			System.out.println("DeeZ exception caught: " + e.getMessage());
		}
	}
}