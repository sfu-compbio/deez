#!/bin/bash

mkdir -p out
mkdir -p log

find '*dztest*' -delete
rm -rf out/*
rm -rf log/*

echo "/// Batch BASIC" >&2 ;
for sam in ${ref}*.sam ;
do
	echo -ne "=== ${sam} ... compressing ... " >&2 ;
	echo "./deez -r ref/ ${sam} -! -o out/${sam}.dz" >&2;
	./deez -I -r ref ${sam} -! -o out/${sam}.dz 2>log/c_${sam};
	echo -ne "decompressing ... " >&2 ;
	./deez -I -r ref out/${sam}.dz -! -o out/dz_${sam}.sam 2>log/d_${sam};
	echo -ne "testing equality ... " >&2 ;

	if `cmp ${sam}${suffix} out/dz_${sam}.sam` ;
	then 
		echo "OK ===" >&2
	else
		echo "FAIL ===" >&2 
	fi;
done ;

for sam in ${ref}*.sam ;
do
	echo -ne "=== ${sam} ... compressing ... " >&2 ;
	./deez -I -! -o out/${sam}.dz 2>log/c_${sam};
	echo -ne "decompressing ... " >&2 ;
	./deez -I -! -o out/dz_${sam}.sam 2>log/d_${sam};
	echo -ne "testing equality ... " >&2 ;

	if `cmp ${sam}${suffix} out/dz_${sam}.sam` ;
	then 
		echo "OK ===" >&2
	else
		echo "FAIL ===" >&2 
	fi;
done ;


cd staden
for ref in aux c1 ce xx;
do
	echo "/// Batch ${ref}" >&2 ;
	for sam in ${ref}*.sam ;
	do
		echo -ne "=== ${sam} ... compressing ... " >&2 ;
		../deez -r ${ref}.fa ${sam} -! -o ../out/${sam}.dz 2>../log/c_${sam};
		echo -ne "decompressing ... " >&2 ;
		../deez -r ${ref}.fa ../out/${sam}.dz -! -o ../out/dz_${sam}.sam 2> ../log/d_${sam};
		echo -ne "testing equality ... " >&2 ;

		if grep -q 'unsorted' <<< "${sam}" ;
		then
			suffix='.sort'
		fi

		if `cmp ${sam}${suffix} ../out/dz_${sam}.sam` ;
		then 
			echo "OK ===" >&2
		else
			echo "FAIL ===" >&2 
		fi;
	done ;
done

