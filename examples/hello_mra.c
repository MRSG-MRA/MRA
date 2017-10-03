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

#include "common_mra.h"
#include "mra_cv.h"
#include <mra.h>

//static int MRA_init_mra_cv (const char* cv_file_name);

static void read_mra_config_file (const char* file_name)
{
  char    property[256];
  FILE*   file;

  /* Set the default configuration. */
  config_mra.mra_chunk_size = 67108864;
  config_mra.amount_of_tasks_mra[MRA_REDUCE] = 1;
  config_mra.Fg=1;
  config_mra.mra_perc=100;

  /* Read the user configuration file. */

  file = fopen (file_name, "r");
  /* Read the user configuration file. */


  while ( fscanf (file, "%256s", property) != EOF )
  {
    if ( strcmp (property, "mra_chunk_size") == 0 )
    {
    fscanf (file, "%lg", &config_mra.mra_chunk_size);
    config_mra.mra_chunk_size *= 1024 * 1024; /* MB -> bytes */
    }
    else if ( strcmp (property, "grain_factor") == 0 )
    {
    fscanf (file, "%d", &config_mra.Fg);
    }
    else if ( strcmp (property, "mra_map_task_cost") == 0 )
		{
	    fscanf (file, "%lg", &config_mra.cpu_required_map_mra);
		}
		else if ( strcmp (property, "mra_reduce_task_cost") == 0 )
		{
	    fscanf (file, "%lg", &config_mra.cpu_required_reduce_mra);
		}
    else if ( strcmp (property, "mra_intermed_perc") == 0 )
    {
    fscanf (file, "%lg", &config_mra.mra_perc);
    }
    else if ( strcmp (property, "mra_reduces") == 0 )
    {
    fscanf (file, "%d", &config_mra.amount_of_tasks_mra[MRA_REDUCE]);
    }

  }


 
 //config_mra.cpu_required_map_mra *= config_mra.mra_chunk_size;

 //config_mra.cpu_required_reduce_mra *= ((config_mra.mra_chunk_size*config_mra.mra_perc/100)/config_mra.amount_of_tasks_mra[MRA_REDUCE]);

  fclose (file);

}



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

int mra_int_data;

  	mra_int_data = ((config_mra.mra_chunk_size*config_mra.mra_perc/100)/config_mra.amount_of_tasks_mra[MRA_REDUCE]);

  return mra_int_data;
}


/**
* User function that indicates the cost of a task.
*
* @param  mra_phase  The execution phase.
* @param  tid    The ID of the task.
* @param  mra_wid    The ID of the worker that received the task.
* @return The task cost in FLOPs.
*/
double mra_task_cost_function (enum mra_phase_e mra_phase, size_t tid, size_t mra_wid)
{
   double mra_map_required;
   double mra_reduce_required;
       
    switch (mra_phase)
    {
	case MRA_MAP:
	    config_mra.cpu_required_map_mra = config_mra.map_task_cost_mra * config_mra.mra_chunk_size/(1024 * 1024);
      mra_map_required = config_mra.cpu_required_map_mra/config_mra.mra_slots[MRA_MAP];
	    return mra_map_required;

	case MRA_REDUCE:
	    config_mra.cpu_required_reduce_mra = config_mra.reduce_task_cost_mra* ((config_mra.mra_chunk_size/(1024 * 1024) *config_mra.mra_perc/100)/config_mra.amount_of_tasks_mra[MRA_REDUCE]);
      mra_reduce_required = config_mra.cpu_required_reduce_mra/config_mra.mra_slots[MRA_REDUCE];
      if (config_mra.Fg >1) 
      {
        mra_reduce_required *= 1/config_mra.Fg;
      }
	    return mra_reduce_required;
    }

}

int main (int argc, char* argv[])
{
  /* MRA_init must be called before setting the user functions. */
  MRA_init ();
  /* Set the task cost function. */
  MRA_set_task_mra_cost_f (mra_task_cost_function);
  /* Set the map output function. */
  MRA_set_map_mra_output_f (mra_map_mra_output_function);
  /* Initialize volatility traces */
 // MRA_init_mra_cv ("parse-boinc.txt");
  /* Run the simulation. */
   // MRA_main ("mra-plat32-10M.xml", "d-mra-plat32-10M.xml", "mra32.conf","parser-boinc-180.txt");
   //MRA_main ("aula-32.xml", "d-aula-32.xml", "aula-8.conf","parser-boinc-030.txt");
   //MRA_main ("plat64.xml", "d-plat64.xml", "mra64.conf","parser-boinc-180.txt");
   //MRA_main ("mra-plat128-1G.xml", "d-mra-plat128-1G.xml", "mra128.conf","parser-boinc-180.txt");
  //  MRA_main ("mra-plat256-10M.xml", "d-mra-plat256-10M.xml", "mra256.conf","parse-boinc.txt");
  //  MRA_main ("mra-plat256-76M.xml", "d-mra-plat256-10M.xml", "mra256.conf","parse-boinc.txt"); 
  // MRA_main ("plat-2k.xml", "d-plat-2k.xml", "mra-2k.conf","parser-boinc-1500.txt"); 
    MRA_main ("plat64.xml", "d-plat64.xml", "plat64.conf","parser-boinc-padrao.txt"); 
     
  //  MRA_main ("cc-e.xml", "d-cc-e.xml", "cc-e.conf","parse-boinc.txt");     
	
//	MRA_main("mra-plat256-logn.xml","d-mra-plat256-10M.xml", "mra256.conf","parse-boinc.txt");
  //MRA_main ("mrbitdew-sophia-50.xml", "d-mrbitdew-sophia-50.xml", "mrbitdew-50.conf", "parse-boinc.txt");
     //   MRA_main ("mra-plat-45.xml", "d-mra-plat-45.xml", "mra45.conf","parse-boinc.txt");
  return 0;
}
