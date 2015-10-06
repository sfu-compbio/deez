#!/bin/bash

mkdir -p out
mkdir -p log
out=`realpath out`
log=`realpath log`
deez=`realpath deez`

find '*dztest*' -delete
rm -rf ${out}/*
rm -rf ${log}/*

function test {
	sam=$1;
	ref="";
	if [ ! -z "$2" ];
	then 
		ref="-r $2";
	else
		ref="";
	fi

	echo -ne "${sam}\n\tCompressing ... " >&2 ;
	cmdc="${deez} ${ref} ${sam} -! -o ${out}/${sam}.dz";
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
	fi;
}

echo -ne "========================================================================\n"
echo -ne "### Batch BASIC\n" >&2 ;
echo -ne "========================================================================\n"

for sam in *.sam ;
do
	test ${sam} "ref";
done ;

echo -ne "========================================================================\n"
echo -ne "### Batch BASIC NoRef\n" >&2 ;
echo -ne "========================================================================\n"
for sam in *.sam ;
do
	test ${sam};
done ;

cd staden
for r in aux c1 ce xx;
do
	echo -ne "========================================================================\n"
	echo -ne "### Batch ${r}\n" >&2 ;
	echo -ne "========================================================================\n"
	for sam in ${r}*.sam ;
	do
		test ${sam} "${r}.fa"
	done ;
done

