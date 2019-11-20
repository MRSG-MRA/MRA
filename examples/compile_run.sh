#!/bin/bash

#making 
cd ..
make clean all
cd examples/
make clean all
./hello_mra.bin $1 $2 $3 $4 

#clearing files


if (($5  == 0));then 
	rm -f *.log *.csv
fi

rm -f *.bin *.plist  
cd ..
rm -f  libmrsg.a *.o *.a
