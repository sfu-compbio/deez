**DeeZ**: Reference Based Compression by Local Assembly
===================

---

### So what is DeeZ?
DeeZ is a tool for compressing SAM/BAM files.

### How do I get DeeZ?
Just clone our repository and issue `make` command:
```
git clone https://bitbucket.org/compbio/dz.git
cd dz && make
```
> **Note**: You will need at least g++ 4.4 to compile the sources, 
as well as CURL and OpenSSL libraries.

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

- `--read-lossy, -L`

	Enables lossy compression of read names. 

	**WARNING: This option is experimental and might require a lot of memory!**

- `--quality, -q [mode]`

	If `mode` is **1** or **samcomp**, DeeZ will use [sam_comp][2] quality model to encode the qualities. Quality random access is not supported on those files. 

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

### Licence

Copyright (c) 2013, 2014, Simon Fraser University, Indiana University Bloomington. All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
- Neither the name of the Simon Fraser University, Indiana University Bloomington nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  [1]: http://scalce.sourceforge.net/
  [2]: https://sourceforge.net/projects/samcomp/
  [3]: http://www.nature.com/nmeth/journal/v11/n11/full/nmeth.3133.html
