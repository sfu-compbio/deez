**DeeZ**: Reference Based Compression by Local Assembly
===================

**Warning**: Current version is 1.9 beta 2. It will eventually become 2.0.
It is huge improvement over 1.x series, but it is still in developement, and it
might contain some rough edges. If you need absolute stability, please use 1.1 
until 2.0 is released.

### So what is DeeZ?
DeeZ is a tool for compressing SAM/BAM files.

### How do I get DeeZ?
Just clone our repository and issue `make` command:
```
git clone https://github.com/sfu-compbio/deez.git
cd deez && make -j 
```
> **Note**: You will need at least g++ 4.8 with C++11 support to compile the sources, as well as BZ2, CURL, and OpenSSL libraries.

### How do I run DeeZ?
DeeZ is invoked as following:

- **Compression**

	`deez -r [reference] [input.sam] -o [output]`

	This will compress `input.sam` to `input.sam.dz`.

- **Decompression**

	`deez -r [reference] [input.dz] -o [output] ([region])`

	This will decompress `input.dz` to `input.dz.sam`. `[region]` is optional.

-  **Random Access**

	You can also specify the region of interest while decompressing (i.e. randomly access the region). For example, to extract some reads from chr16 to standard output, you should run:
	`deez -r [reference] input.dz  -c chr16:15000000-16000000`

DeeZ supports URLs as input files. Supported protocols are file://, ftp://, http(s):// and s3://.
For Amazon S3 access, DeeZ accepts `AWS_ACCESS_KEY_ID` and `AWS_SECRET_ACCESS_KEY` environment 
variables if you need to access private repositories.

---

### Parameter explanation

- `--threads, -t [number]`

	Set up the number of threads DeeZ may use for compression and decompression.

	Default value: **4**

- `--header, -h`

	Outputs the SAM header.

- `--reference, -r [file|directory]`

	Specify the FASTA reference file.
	If no reference file is provided, DeeZ will either try to download
	reference specified in SAM @SQ:UR field. If such field is not
	available or if reference specified therein is not accessible, 
	DeeZ will use no reference. 

	If no reference is being used, compression rate will not be optimal.

	> **Note**: Chromosome names in the SAM and FASTA files must match. Also, instead of one big FASTA file, DeeZ supports reference lookup in the given directory for chr\*.fa files, where chr\* is the chromosome ID from the SAM file.

- `--force, -!`
	
	Force overwrite of exiting files.

- `--verbosity, -v [level]`
	
	Set the logging level. 
	Level 0 shows only errors and warnings.
	Level 1 shows many other useful information (e.g. download progress,
		compression progress, per-field data usage etc.)
	Level 2 shows debug information, including detailed CURL output.

	Default valie: **0**

- `--stdout, -c`

	Compress/decompress to the stdout.

- `--output, -o [file]`
	
	Compress/decompress to the `file`.

- `--lossy, -l`

	Set lossy parameter for quality lossy encoding (for more information, please check [SCALCE][1]).

- `--quality, -q [mode]`

	If `mode` is **1** or **samcomp**, DeeZ will use [sam_comp][2] quality model to encode the qualities. Quality random access is not supported on those files. 
	If `mode` is **2**, DeeZ will use rANS order-2 model. This will result in slightly better compression, but decompression  will be slower.

- `--withflag, -f [flag]`

	Decompress only mappings which have `flag` bits set.

- `--withoutflag, -F [flag]`

	Decompress only mappings which do not have `flag` bits set.

- `--stats, -S`

	Display mapping statistics (needs DeeZ file as input).

- `--sort, -s`

	Sort the input SAM/BAM file by mapping location.

- `--sortmem, -M [size]`

	Maximum memory used for sorting. 
	
	Default value: **1G**

- `--noqual, -Q`
	
	Disable quality score output during decompression.

---

### Contact & Support

Feel free to drop any inquiry to [inumanag at sfu dot ca](mailto:).

### Publication

DeeZ was publised in [Nature Methods in November 2014][3].

### Authors

- Dr. Faraz Hach
- Ibrahim Numanagić
- Dr. S. Cenk Şahinalp

### Many thanks to...

- **James Bonfield**, author of [Scramble](http://sourceforge.net/projects/staden/files/io_lib/) and [sam_comp](http://sourceforge.net/projects/samcomp/) for many useful ideas and suggestions. 
- **Fabian "ryg" Giessen** for his [rANS implementation](https://github.com/rygorous/ryg_rans)
- **Eugene D. Shelwien** for his [arithmetic coder implementation](http://compression.ru/sh/aridemo6.rar)
- **Vitaliy Vitsentiy** for his [ctpl threading library](https://github.com/vit-vit/CTPL)
- ... and all people who are using and testing DeeZ


  [1]: http://scalce.sourceforge.net/
  [2]: https://sourceforge.net/projects/samcomp/
  [3]: http://www.nature.com/nmeth/journal/v11/n11/full/nmeth.3133.html
