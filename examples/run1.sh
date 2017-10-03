#!/bin/bash

export LD_LIBRARY_PATH=$HOME/simgrid-3.11.1/lib

./hello_mra.bin --cfg=surf/nthreads:-1 2>&1 | $HOME/simgrid-3.11.1/bin/simgrid-colorizer > exec.txt
