/* Copyright (c) 2010-2014. MRA Team. All rights res_mraerved. */

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


#include <simgrid/msg.h>
#include <xbt/sysdep.h>
#include <xbt/log.h>
#include <xbt/asserts.h>
#include <stdio.h>
#include <stdlib.h>
#include "common_mra.h"
#include "worker_mra.h"
#include "dfs_mra.h"
#include "mra.h"
#include "mra_cv.h"


XBT_LOG_NEW_DEFAULT_CATEGORY (msg_test, "MRA");
//XBT_LOG_EXTERNAL_DEFAULT_CATEGORY (msg_test);

#define MAX_LINE_SIZE 512

int master_mra (int argc, char *argv[]);
int worker_mra (int argc, char *argv[]);

static void check_config_mra (void);
static msg_error_t run_mra_simulation (const char* platform_file, const char* deploy_file, const char* mra_config_file, const char* vc_file_name);
static void init_mr_mra_config (const char* mra_config_file);
//Initialize VC
static void init_mra_vc (const char* vc_file_name);
static int mra_vc_prep_traces (const char* vc_file_name);
static int read_mra_vc_config_file (const char* vc_file_name, int n_line);
//End VC
static void read_mra_config_file (const char* file_name);
static void init_mra_config (void);
static void init_job_mra (void);
static void init_mra_stats (void);
static void free_mra_global_mem (void);




int MRA_main (const char* plat, const char* depl, const char* conf, const char* trace_vc)
{
    int argc = 8;
    char* argv[] = {
	"mra",
	"--cfg=tracing:1",
	"--cfg=tracing/buffer:1",
	"--cfg=tracing/filename:tracefile.trace",
	"--cfg=tracing/categorized:1",
	"--cfg=tracing/uncategorized:1",
	"--cfg=viva/categorized:cat.plist",
	"--cfg=viva/uncategorized:uncat.plist"
    };

    msg_error_t  res_mra = MSG_OK;

    config_mra.initialized = 0;

    check_config_mra ();

    MSG_init (&argc, argv);

    res_mra = run_mra_simulation (plat, depl, conf, trace_vc);

    if (res_mra == MSG_OK)
	return 0;
    else
	return 1;
}




/**
 * @brief Check if the user configuration is sound.
 */
static void check_config_mra (void)
{
    xbt_assert (user_mra.task_mra_cost_f != NULL, "Task cost function not specified.");
    xbt_assert (user_mra.map_mra_output_f != NULL, "Map output function not specified.");
}

/**
 * @param  platform_file   The path/name of the platform file.
 * @param  deploy_file     The path/name of the deploy file.
 * @param  mra_config_file  The path/name of the configuration file.
 * @param  vc_file_name   The volunteer computing configuration file.
 */
static msg_error_t run_mra_simulation (const char* platform_file, const char* deploy_file, const char* mra_config_file, const char* vc_file_name)
{
    msg_error_t  res_mra = MSG_OK;

    read_mra_config_file (mra_config_file);

    MSG_create_environment (platform_file);

    read_bandwidth (platform_file);

    init_mra_vc (vc_file_name);

    // for tracing purposes..
    TRACE_category_with_color ("MRA_MAP", "1 0 0");
    TRACE_category_with_color ("MRA_REDUCE", "0 0 1");

    MSG_function_register ("master_mra", master_mra);
    MSG_function_register ("worker_mra", worker_mra);
    MSG_launch_application (deploy_file);

    init_mr_mra_config (mra_config_file);

    res_mra = MSG_main ();

    free_mra_global_mem ();

    return res_mra;
}

/**
 * @brief  Initialize the MapReduce configuration.
 * @param  mra_config_file  The path/name of the configuration file.
 */
static void init_mr_mra_config (const char* mra_config_file)
{
    srand (12345);
    init_mra_config ();
    init_mra_stats ();
    init_job_mra ();
    distribute_data_mra ();
}

/**
 * @brief Initialize the Volunteer Computing Traces.
 * @param vc_file_name
 *
 */

static void init_mra_vc (const char* vc_file_name){

    int n_line;
    n_line = mra_vc_prep_traces (vc_file_name);
    read_mra_vc_config_file (vc_file_name, n_line);
}

/*
* @brief Reads config file and creates a vector to each node with the availability type, and start and end times.
*/
static int read_mra_vc_config_file (const char* vc_file_name, int n_line)
{
    int   i;
    /* Allocate memory for the volunteer computing matrix. */
   	vc_node = xbt_new(int*, (n_line * sizeof (int)));
   	vc_type = xbt_new(int*, (n_line * sizeof (int)));
   	vc_start = xbt_new(long double*, (n_line * sizeof (long double)));
   	vc_end = xbt_new(long double*, (n_line * sizeof (long double)));
   	/* Allocate memory for the elements of volunteer computing matrix. */
    for (i = 0; i < (n_line + 1); i++)
    {
     		vc_node[i] = xbt_new(int, (sizeof (int)));
     		vc_type[i] = xbt_new(int, (sizeof (int)));
     		vc_start[i] = xbt_new(long double, (sizeof (long double)));
     		vc_end[i] = xbt_new(long double, (sizeof (long double)));
    }

    VC_TRACE *ptr_one;
    FILE* vc_file;

    ptr_one = (VC_TRACE *) malloc(n_line * sizeof(VC_TRACE));
    vc_file = fopen (vc_file_name, "r");

    i=0;
    while(!feof(vc_file))
    {
        fscanf(vc_file, "%d,%d,%Lf,%Lf", &ptr_one->mra_vc_node_id, &ptr_one->mra_vc_status_type,
                                  &ptr_one->mra_vc_start, &ptr_one->mra_vc_end);

        vc_node[i][0] = ptr_one->mra_vc_node_id;
        vc_type[i][0] = ptr_one->mra_vc_status_type;
        vc_start[i][0] = ptr_one->mra_vc_start;
        vc_end[i][0]= ptr_one->mra_vc_end;
    		i++;
    }

    return 0;

    fclose(vc_file);
}

/* @brief Return the number of lines on vc_file
*  @param vc_file_name
*  @return n_line
*/
static int mra_vc_prep_traces (const char* vc_file_name)
{

    int n_line = 0;
    char c;
    FILE* file;
    /* Read the user configuration file. */
    file = fopen (vc_file_name, "r");

    // Extract characters from file and store in character n_line
    for (c = getc(file); c != EOF; c = getc(file)){
        if (c == '\n')
            n_line++;
  			}
        fclose(file);
   /*Array element with number lines of vc_fine_name. The result is saved on element array config_mra_vc_file_line[0] */
    config_mra_vc_file_line[0] = n_line;
    //XBT_INFO ("Line number %d, @ in %s \n", n_line, vc_file_name );
    return n_line;
}


/**
 * @brief  Read the MapReduce configuration file.
 * @param  file_name  The path/name of the configuration file.
 */
static void read_mra_config_file (const char* file_name)
{
    char    property[256];
    FILE*   file;

    /* Set the default configuration. */
    config_mra.mra_chunk_size = 67108864;
    config_mra.mra_chunk_count = 0;
    config_mra.mra_chunk_replicas = 3;
    config_mra.mra_slots[MRA_MAP] = 2;
    config_mra.amount_of_tasks_mra[MRA_REDUCE] = 1;
    config_mra.mra_slots[MRA_REDUCE] = 2;
    config_mra.Fg=1;
    config_mra.mra_perc=100;

    /* Read the user configuration file. */

    file = fopen (file_name, "r");

    xbt_assert (file != NULL, "Error reading cofiguration file: %s", file_name);

    while ( fscanf (file, "%256s", property) != EOF )
    {
	if ( strcmp (property, "mra_chunk_size") == 0 )
	{
	    fscanf (file, "%lg", &config_mra.mra_chunk_size);
	    config_mra.mra_chunk_size *= 1024 * 1024; /* MB -> bytes */
	}
	else if ( strcmp (property, "mra_input_chunks") == 0 )
	{
	    fscanf (file, "%d", &config_mra.mra_chunk_count);
	}
	else if ( strcmp (property, "mra_dfs_replicas") == 0 )
	{
	    fscanf (file, "%d", &config_mra.mra_chunk_replicas);
	}
	else if ( strcmp (property, "mra_map_slots") == 0 )
	{
	    fscanf (file, "%d", &config_mra.mra_slots[MRA_MAP]);
	}
	else if ( strcmp (property, "grain_factor") == 0 )
	{
	    fscanf (file, "%d", &config_mra.Fg);
	}
	else if ( strcmp (property, "mra_map_task_cost") == 0 )
	{
	    fscanf (file, "%lg", &config_mra.map_task_cost_mra);
	}
	else if ( strcmp (property, "mra_reduce_task_cost") == 0 )
	{
	    fscanf (file, "%lg", &config_mra.reduce_task_cost_mra);
	}
	else if ( strcmp (property, "mra_intermed_perc") == 0 )
	{
	    fscanf (file, "%lg", &config_mra.mra_perc);
	}
		else if ( strcmp (property, "mra_reduces") == 0 )
	{
	    fscanf (file, "%d", &config_mra.amount_of_tasks_mra[MRA_REDUCE]);
	}
		else if ( strcmp (property, "perc_num_volatile_node") == 0 )
	{
	    fscanf (file, "%lg", &config_mra.perc_vc_node);
	}
		else if ( strcmp (property, "failure_timeout") == 0 )
	{
	    fscanf (file, "%lg", &config_mra.failure_timeout_conf);
	}
	else if ( strcmp (property, "mra_reduce_slots") == 0 )
	{
	    fscanf (file, "%d", &config_mra.mra_slots[MRA_REDUCE]);
	}
	else
	{
	    printf ("Error: Property %s is not valid. (in %s)", property, file_name);
	    exit (1);
	}
    }

    fclose (file);

    /* Assert the configuration values. */

    xbt_assert (config_mra.mra_chunk_size > 0, "MRA_Chunk size must be greater than zero");
    xbt_assert (config_mra.mra_chunk_count > 0, "The amount of MRA_input chunks must be greater than zero");
    xbt_assert (config_mra.mra_chunk_replicas > 0, "The amount of MRA_chunk replicas must be greater than zero");
    xbt_assert (config_mra.mra_slots[MRA_MAP] > 0, "MRA_Map slots must be greater than zero");
    xbt_assert (config_mra.amount_of_tasks_mra[MRA_REDUCE] >= 0, "The number of MRA_reduce tasks can't be negative");
    xbt_assert (config_mra.mra_slots[MRA_REDUCE] > 0, "MRA_Reduce slots must be greater than zero");
}

/**
 * @brief  Initialize the config structure.
 */
static void init_mra_config (void)
{
    const char*    process_name = NULL;
    msg_host_t     host;
    msg_process_t  process;
    size_t         mra_wid;
    unsigned int   cursor;
    w_mra_info_t       wi;
    xbt_dynar_t    process_list;

    /* Initialize hosts information. */

    config_mra.mra_number_of_workers = 0;

    process_list = MSG_processes_as_dynar ();
    xbt_dynar_foreach (process_list, cursor, process)
    {
	process_name = MSG_process_get_name (process);
	if ( strcmp (process_name, "worker_mra") == 0 )
	    config_mra.mra_number_of_workers++;
    }

    config_mra.workers_mra = xbt_new (msg_host_t, config_mra.mra_number_of_workers);

    mra_wid = 0;
    config_mra.grid_cpu_power = 0.0;
    xbt_dynar_foreach (process_list, cursor, process)
    {
	process_name = MSG_process_get_name (process);
	host = MSG_process_get_host (process);
	if ( strcmp (process_name, "worker_mra") == 0 )
	{
	    config_mra.workers_mra[mra_wid] = host;
	    /* Set the worker ID as its data. */
	    wi = xbt_new (struct mra_w_info_s, 1);
	    wi->mra_wid = mra_wid;
	    MSG_host_set_data (host, (void*)wi);
	    /* Add the worker's cpu power to the grid total. */
	    config_mra.grid_cpu_power += MSG_get_host_speed (host);
	    mra_wid++;
	}
    }
    config_mra.grid_average_speed = config_mra.grid_cpu_power / config_mra.mra_number_of_workers;
    config_mra.mra_heartbeat_interval = mra_maxval (MRA_HEARTBEAT_MIN_INTERVAL, config_mra.mra_number_of_workers / 100);
    config_mra.amount_of_tasks_mra[MRA_MAP] = config_mra.mra_chunk_count;
    config_mra.initialized = 1;
}

/**
 * @brief  Initialize the job structure.
 */
static void init_job_mra (void)
{
    int     i;
    size_t  mra_wid;

    xbt_assert (config_mra.initialized, "init_mra_config has to be called before init_job_mra");

    job_mra.finished = 0;
    job_mra.mra_heartbeats = xbt_new (struct mra_heartbeat_s, config_mra.mra_number_of_workers);
    for (mra_wid = 0; mra_wid < config_mra.mra_number_of_workers; mra_wid++)
    {
			job_mra.mra_heartbeats[mra_wid].slots_av[MRA_MAP] = config_mra.mra_slots[MRA_MAP];
			job_mra.mra_heartbeats[mra_wid].slots_av[MRA_REDUCE] = config_mra.mra_slots[MRA_REDUCE];
			job_mra.mra_heartbeats[mra_wid].wid_timestamp = sizeof(long double);
    }

    /* Initialize map information. */
    job_mra.tasks_pending[MRA_MAP] = config_mra.amount_of_tasks_mra[MRA_MAP];
    job_mra.task_status[MRA_MAP] = xbt_new0 (int, config_mra.amount_of_tasks_mra[MRA_MAP]);
    job_mra.task_instances[MRA_MAP] = xbt_new0 (int, config_mra.amount_of_tasks_mra[MRA_MAP]);
    job_mra.task_list[MRA_MAP] = xbt_new0 (msg_task_t*, config_mra.amount_of_tasks_mra[MRA_MAP]);
    job_mra.mra_task_dist[MRA_MAP] = xbt_new (int*, (config_mra.mra_number_of_workers * config_mra.amount_of_tasks_mra[MRA_MAP]) * (sizeof (int)));
    for (i = 0; i < config_mra.amount_of_tasks_mra[MRA_MAP]; i++)
    {
	  job_mra.task_list[MRA_MAP][i] = xbt_new0 (msg_task_t, MAX_SPECULATIVE_COPIES);
    job_mra.mra_task_dist[MRA_MAP][i] = xbt_new0 (int, (config_mra.mra_number_of_workers * config_mra.amount_of_tasks_mra[MRA_MAP]) * (sizeof (int)));
    }
    // Configuracao dos Reduces Inicia aqui
     if (config_mra.Fg > 1)
 			{
   			config_mra.amount_of_tasks_mra[MRA_REDUCE] = config_mra.Fg * config_mra.mra_number_of_workers;
   		//	config_mra.cpu_required_reduce_mra *= ((config_mra.mra_chunk_size*config_mra.mra_perc/100)/config_mra.amount_of_tasks_mra[MRA_REDUCE]);
 			}

    job_mra.map_output = xbt_new (size_t*, config_mra.mra_number_of_workers);
    for (i = 0; i < config_mra.mra_number_of_workers; i++)
	  job_mra.map_output[i] = xbt_new0 (size_t, config_mra.amount_of_tasks_mra[MRA_REDUCE]);

    // Initialize reduce information.

    job_mra.tasks_pending[MRA_REDUCE] = config_mra.amount_of_tasks_mra[MRA_REDUCE];
    job_mra.task_status[MRA_REDUCE] = xbt_new0 (int, config_mra.amount_of_tasks_mra[MRA_REDUCE]);
    job_mra.task_instances[MRA_REDUCE] = xbt_new0 (int, config_mra.amount_of_tasks_mra[MRA_REDUCE]);
    job_mra.task_list[MRA_REDUCE] = xbt_new0 (msg_task_t*, config_mra.amount_of_tasks_mra[MRA_REDUCE]);
    job_mra.mra_task_dist[MRA_REDUCE] = xbt_new (int*, config_mra.mra_number_of_workers * (sizeof (int)));
    for (i = 0; i < config_mra.amount_of_tasks_mra[MRA_REDUCE]; i++)
    {
	  job_mra.task_list[MRA_REDUCE][i] = xbt_new0 (msg_task_t, MAX_SPECULATIVE_COPIES);
	  job_mra.mra_task_dist[MRA_REDUCE][i] = xbt_new0 (int, (config_mra.mra_number_of_workers * config_mra.amount_of_tasks_mra[MRA_REDUCE]) * (sizeof (int)));
	  }
	 // Configuracao dos Reduces Termina aqui */

}

/**
 * @brief  Initialize the stats structure.
 */
static void init_mra_stats (void)
{
    xbt_assert (config_mra.initialized, "init_mra_config has to be called before init_mra_stats");

    stats_mra.map_local_mra 		= 0;
    stats_mra.mra_map_remote 		= 0;
    stats_mra.map_spec_mra_l 		= 0;
    stats_mra.map_spec_mra_r 		= 0;
    stats_mra.reduce_mra_normal = 0;
    stats_mra.reduce_mra_spec 	= 0;
    stats_mra.mra_map_recovery 	= 0;
    stats_mra.mra_reduce_recovery = 0;
}

/**
 * @brief  Free allocated memory for global variables.
 */
static void free_mra_global_mem (void)
{
    size_t  i;

    for (i = 0; i < config_mra.mra_chunk_count; i++)
	xbt_free_ref (&chunk_owner_mra[i]);
    xbt_free_ref (&chunk_owner_mra);

    xbt_free_ref (&config_mra.workers_mra);
    xbt_free_ref (&job_mra.task_status[MRA_MAP]);
    xbt_free_ref (&job_mra.task_instances[MRA_MAP]);
    xbt_free_ref (&job_mra.task_status[MRA_REDUCE]);
    xbt_free_ref (&job_mra.task_instances[MRA_REDUCE]);
    xbt_free_ref (&job_mra.mra_heartbeats);
    for (i = 0; i < config_mra.amount_of_tasks_mra[MRA_MAP]; i++)
	xbt_free_ref (&job_mra.task_list[MRA_MAP][i]);
	xbt_free_ref (&job_mra.mra_task_dist[MRA_MAP][i]);
    xbt_free_ref (&job_mra.task_list[MRA_MAP]);
    for (i = 0; i < config_mra.amount_of_tasks_mra[MRA_REDUCE]; i++)
	xbt_free_ref (&job_mra.task_list[MRA_REDUCE][i]);
	xbt_free_ref (&job_mra.mra_task_dist[MRA_REDUCE][i]);
    xbt_free_ref (&job_mra.task_list[MRA_REDUCE]);
}
