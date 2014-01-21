#!/bin/bash

export LD_LIBRARY_PATH=$HOME/simgrid-3.6.2/lib

./mrsg $1 $2 $3 2>&1 | $HOME/simgrid-3.6.2/bin/simgrid-colorizer
