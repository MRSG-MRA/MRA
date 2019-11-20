/* Copyright (c) 2010-2014. MRA Team. All rights reserved. */

/* This file is part of MRSG and MRA++.

MRSG and MRA++ are free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MRSG and MRA++ are distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MRSG and MRA++.  If not, see <http://www.gnu.org/licenses/>. */

#include <mra.hpp>
#include "common_mra.hpp"
#include "mra_cv.hpp"
#include <iostream>


/**
 * User function that indicates the amount of bytes
 * that a map task will emit to a reduce task.
 *
 * @param  mid  The ID of the map task.
 * @param  rid  The ID of the reduce task.
 * @return The amount of data emitted (in bytes).
 */
int mra_map_mra_output_function (size_t mid, size_t rid)
{
	if(config_mra.mra_map_output == 0 ){
		printf("sub");
		return ((config_mra.mra_chunk_size*(config_mra.mra_perc/100))/config_mra.amount_of_tasks_mra[MRA_REDUCE]);

	}
    else
    	return config_mra.mra_map_output;
    
    
}


/**
 * User function that indicates the cost of a task.
 *
 * @param  mra_phase  The execution phase.
 * @param  tid    The ID of the task.
 * @param  mra_wid    The ID of the worker that received the task.
 * @return The task cost in FLOPs.
 */
double mra_task_mra_cost_function (enum mra_phase_e mra_phase, size_t tid, size_t mra_wid)
{
   
     switch (mra_phase)
    {
	case MRA_MAP:
	    return config_mra.map_task_cost_mra;

	case MRA_REDUCE:
	    return config_mra.reduce_task_cost_mra;
    }
    
    /*
    switch (mra_phase)
    {
	case MRA_MAP:
	    return 3e+11;

	case MRA_REDUCE:
	    return (3e+11/config_mra.Fg);
    }
    */
    
}

int main (int argc, char* argv[])
{    
    /* MRA_init must be called before setting the user functions. */
    
    if(argc != 5){
		std::cout << "Please insert the parameters <platform.xml> <deployplatform.xml> <yourconfig.conf> <volatile-file.txt>" << std::endl;
		return -1;
	}

    simgrid::s4u::Engine e(&argc, argv);

    MRA_init ();
    /* Set the task cost function. */
    MRA_set_task_mra_cost_f (mra_task_mra_cost_function);
    /* Set the map output function. */
    MRA_set_map_mra_output_f (mra_map_mra_output_function);
   
    /* Run the simulation. */
    
    MRA_main (argv[1], argv[2], argv[3], argv[4]);
    
    return 0;
}



