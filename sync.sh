#!/bin/bash
for i in `find . -not -path './run/*' -name '*.cc' -o -not -path './run/*' -name '*.h'`;
do
	echo "scp $i oak:/cs/compbio/ibrahim/dz/$i" ;
	scp $i oak:/cs/compbio/ibrahim/dz/$i ;
done
