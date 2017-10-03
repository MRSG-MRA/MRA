#!/bin/bash

export LD_LIBRARY_PATH=$HOME/simgrid-3.11/lib
 
for volatil_perc in 0 5 10 15 20 25 30
do    
    for grain_factor in 1 2 4 6 8
    do
        for chunk_input in 256 512 768
        do
            for chunk_size in 16 32 64
            do
            rm -f mra128.conf
            echo "mra_reduces 256
mra_chunk_size $chunk_size
mra_input_chunks $chunk_input
mra_dfs_replicas 3
mra_map_slots 2
mra_reduce_slots 2
grain_factor $grain_factor
mra_intermed_perc 100
perc_num_volatile_node $volatil_perc
failure_timeout 4" >> mra128.conf
            echo "size $chunk_size, input $chunk_input, grain $grain_factor, volatile perc $volatil_perc, 128  machines"
            ./hello_mra.bin 2>&1 | $HOME/simgrid-3.11/bin/simgrid-colorizer > saida.txt
            rm -f saida.txt
            done
        done
    done
done 
 
mv -f hello_mra256.c hello_mra.c
make clean all

for volatil_perc in 0 5 10 15 20 25 30
do    
   
    for grain_factor in 1 2 4 6 8
    do
        for chunk_input in 512 1024 1536
        do
            for chunk_size in 16 32 64
            do
            rm -f mra256.conf
            echo "mra_reduces 512
mra_chunk_size $chunk_size
mra_input_chunks $chunk_input
mra_dfs_replicas 3
mra_map_slots 2
mra_reduce_slots 2
grain_factor $grain_factor
mra_intermed_perc 100
perc_num_volatile_node $volatil_perc
failure_timeout 4" >> mra256.conf
	    echo "size $chunk_size, input $chunk_input, grain $grain_factor, volatile perc $volatil_perc, 256  machines"
            ./hello_mra.bin 2>&1 | $HOME/simgrid-3.11/bin/simgrid-colorizer > saida.txt
            rm -f saida.txt
            done
        done
    done
done
