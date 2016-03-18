#!/bin/bash
# 786
# One day, this should be written in python...

mkdir -p out
mkdir -p log
out=`realpath out`
log=`realpath log`
deez=`realpath deez`

find . | grep dztest | xargs rm -f 
find . | grep dztemp | xargs rm -f
rm -rf ${out}/*
rm -rf ${log}/*

function test {
	sam=$1;
	ref="";

	if [ ! -z "$2" ];
	then 
		comp="$2";
	else
		comp=""
	fi

	if [ ! -z "$3" ];
	then 
		ref="-r $3";
	else
		ref=""
	fi

	echo -ne "${sam}\t${comp} ${ref}\n\tCompressing ... " >&2 ;
	cmdc="${deez} ${comp} ${ref} ${sam} -! -o ${out}/${sam}.dz";
	#echo ${cmdc};
	`${cmdc} 2>${log}/c_${sam}`;
	
	echo -ne "\tDecompressing ... " >&2 ;
	cmdd="${deez} ${ref} ${out}/${sam}.dz -! -o ${out}/dz_${sam}.sam";
	`${cmdd} 2>${log}/d_${sam}`;

	echo -ne "\tTesting ... " >&2 ;
	if grep -q 'unsorted' <<< "${sam}" ;
	then
		suffix='.sort'
	fi
	if `cmp ${sam}${suffix} ${out}/dz_${sam}.sam 2>/dev/null` ;
	then 
		echo "OK" >&2
	else
		echo -ne "FAIL\n\t\tCC: ${cmdc}\n\t\tDC: ${cmdd}\n" >&2 
		exit
	fi;
}

# function testra {
# 	sam=$1;
# 	ref="";
# 	comp="$2";
# 	reg="$3";

# 	if [ ! -z "$4" ];
# 	then 
# 		ref="-r $4";
# 	else
# 		ref=""
# 	fi

# 	echo -ne "${sam}\t${comp} ${ref}\n\tCompressing ... " >&2 ;
# 	cmdc="${deez} ${comp} ${ref} ${sam} -! -o ${out}/${sam}.dz";
# 	#echo ${cmdc};
# 	`${cmdc} 2>${log}/c_${sam}`;
	
# 	echo -ne "\tDecompressing ... " >&2 ;
# 	cmdd="${deez} ${ref} ${out}/${sam}.dz -! -o ${out}/dz_${sam}.sam ${reg}";
# 	`${cmdd} 2>${log}/d_${sam}`;

# 	echo -ne "\tTesting ... " >&2 ;
# 	samtools view -bS ${sam} -o ${out}/dztemp_${sam}.bam ;
# 	samtools index ${out}/dztemp_${sam}.bam ;
# 	samtools view ${out}/dztemp_${sam}.bam ${reg} -o ${out}/dztemp_${sam}.RA ;

# 	cmpd="cmp ${out}/dztemp_${sam}.RA ${out}/dz_${sam}.sam";
# 	echo ${cmpd};
# 	`${cmpd}`;
# 	if [ $? -eq 0 ] ;
# 	then 
# 		echo "OK" >&2
# 	else
# 		echo -ne "FAIL\n\t\tCC: ${cmdc}\n\t\tDC: ${cmdd}\n" >&2 
# 		exit
# 	fi;
# }


# sam="small.sam"
# for reg in "chr1:2-3" "chr1:2-3" "chr2:0-1" "chrE" ; 
# do
# #	testra ${sam} ""   "${reg}"
# 	testra ${sam} ""   "${reg}" "ref"
# #	testra ${sam} "-b" "${reg}" "ref"
# done
# exit
# # sam="testChr.sam"
# # chr5
# # chr14
# # chr14:18359325-18360000
# # chr20:61742360-
# # chr3:


# TODO: test random access, noqual, filtering

echo -ne "========================================================================\n"
echo -ne "### Batch BASIC\n" >&2 ;
echo -ne "========================================================================\n"

for sam in *.sam ;
do
	test ${sam}
	test ${sam} ""       "ref"
	test ${sam} "-b"     "ref"
	test ${sam} "-q1"    "ref"
	test ${sam} "-q2"    "ref"
	test ${sam} "-b -q1" "ref"
	test ${sam} "-b -q2" "ref"

	echo;
done ;

cd staden
for r in aux c1 ce xx;
do
	echo -ne "========================================================================\n"
	echo -ne "### Batch ${r}\n" >&2 ;
	echo -ne "========================================================================\n"
	for sam in ${r}*.sam ;
	do
		test ${sam}
		test ${sam} ""       "${r}.fa"
		test ${sam} "-b -q1" "${r}.fa"

		echo
	done ;
done
