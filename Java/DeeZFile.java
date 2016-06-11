/// 786

public class DeeZFile {
	public class SAMRecord {
		/// Read name
		public String rname;
		/// Mapping flag
		public int flag;
		/// Reference ID
		public String chr;
		/// Locatiwon within refrence
		public long loc;
		/// Mapping quality
		public int mapqual;
		/// CIGAR nmapping string
		public String cigar;
		/// Paired mate reference
		public String pchr;
		/// Paired mate locatin within its refrence
		public long ploc;
		/// Template length
		public int tlen;
		/// Nucleotide sequence
		public String seq;
		/// Quality score for sequence
		public String qual;
		/// Array of optional fields
		public String[] opt;
		/// Hashmap of optional fields
		public java.util.HashMap<String, String> optmap;

		public SAMRecord(String _rname, int _flag, String _chr, long _loc, int _mapqual, String _cigar, String _pchr, long _ploc, int _tlen, String _seq, String _qual, String _opt) {
			rname = _rname;
			flag = _flag;
			chr = _chr;
			loc = _loc;
			mapqual = _mapqual;
			cigar = _cigar;
			pchr = _pchr;
			ploc = _ploc;
			tlen = _tlen;
			seq = _seq;
			qual = _qual;
			opt = _opt.split("\t");

			optmap = new java.util.HashMap<String, String>();
			for (String o: opt) {
				if (o != null && o.length() > 4 && o.charAt(2) == ':') 
					optmap.put(o.substring(0,2), o.substring(3));
			}
		}
	}

    static {
        System.loadLibrary("deez-jni");
    }        

	private long nativeHandle;
    private native void init(String path, String genome);

    /// Sets DeeZ logging level. All logs will be written to stderr.
    /// Equivalent to -v [0,1,2] command line option.
    /// Values: 0, 1 and 2
	public native void setLogLevel(int level) throws DeeZException;

	/// Gets the number of compressed files within DeeZ bundle.
    /// Usually 1, but 0 and values >1 are possible.
	public native int getFileCount() throws DeeZException;
	/// Gets the comment of the file <file> within bundle.
	/// <file> is 0-indexed file index: 0 <= <file> < getFileCount().
	public native String getComment(int file) throws DeeZException;
	/// Gets the list of records in the range <range>
	/// whose mapping flag conforms to <filterFlag> (similar to samtools -f option).
	/// Range is of format: 
	/// 	(<file>),<chr>:<start>-<end>, 
	/// where <file> is optional file index (if not specified, defaults to 0). 
	/// Unlike DeeZ executable, API does not support multiple ranges separated by ";".
	/// Example: 
	///     dz.getRange("1:60-100", 4)
	/// gets all mapped records (flag 0x04) from chromosome 1 in range 60-100
	public native SAMRecord[] getRecords (String range, int filterFlag) throws DeeZException;
	/// Cleans up DeeZ native instance. 
	/// Call it after you are done with DeeZ object.
	public native void dispose() throws DeeZException;

	/// Initializes DeeZ file from <path>.
	/// Use only if DeeZ file was not compressed
	/// with specific external reference
	/// (e.g. reference is implied within @SQ:UR or
	///   no reference is used).
	public DeeZFile (String path) throws DeeZException {
		this.init(path, "");
	}
	/// Initializes DeeZ file from <path>
	/// with <genome> as a path of the reference
	public DeeZFile (String path, String genome) throws DeeZException {
		this.init(path, genome);
	}
	/// Gets all the records from the <range>
	/// without any filtering.
	public SAMRecord[] getRecords (String range) throws DeeZException {
		return getRecords(range, 0);
	}
	/// Gets all the records from the <range>
	/// without any filtering.
	public SAMRecord[] getRecords () throws DeeZException {
		return getRecords("", 0);
	}
}
