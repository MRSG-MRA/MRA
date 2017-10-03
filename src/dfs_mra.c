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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <simgrid/msg.h>
#include "common_mra.h"
#include "worker_mra.h"
#include "dfs_mra.h"
#include "mra_cv.h"



XBT_LOG_EXTERNAL_DEFAULT_CATEGORY (msg_test);

static void send_mra_data (msg_task_t msg);

void distribute_data_mra (void)
{
    size_t  chunk;

    /* Allocate memory for the mapping matrix. */
    chunk_owner_mra = xbt_new (char*, config_mra.mra_chunk_count);
    for (chunk = 0; chunk < config_mra.mra_chunk_count; chunk++)
    {
        chunk_owner_mra[chunk] = xbt_new0 (char, config_mra.mra_number_of_workers);
    }

    /* Call the distribution function. */
    user_mra.mra_dfs_f (chunk_owner_mra, config_mra.mra_chunk_count, config_mra.mra_number_of_workers, config_mra.mra_chunk_replicas);
}

void default_mra_dfs_f (char** mra_dfs_matrix, size_t chunks, size_t workers_mra, int replicas)
{
    FILE*    log;
    FILE*    log_avg;

    int*     tasks_reduce = NULL;
   // int*     total_dist;
    double*  prev_exec_reduce=NULL;
    int      counter, i;
    int      tot_tasks_reduce=0;
    int      mra_tid;
    int	     max_dist, min_dist, total_chunk, rep_wid;
    int      cont_avg=0;
    int      tot_dist=0;
    int 	   dist=0;
    double   min_te_exec ;
    double   avg_t_exec=0;
    size_t   chunk;
    size_t   owner;


    /* START DISTRIBUTION - Matrix chunk_owner_mra (chunk,worker)*/
    /**
      * @brief lista de workers -> workers_hosts[id] (array);
      * @brief pegar capacidade -> MSG_get_host_speed (config_mra.workers[owner]) ;
      * @brief p_worker_cap -> Calculate the computacional capacity for each worker;
      * @brief dist_bruta -> Vector with the chunk numbers for each group in order to find the lower te_exec;
      * @brief prev_exec  -> Runtime prediction for each worker.;
      * @brief temp_corr -> Calculates runtime deviation;
      * @brief task_exec -> Runtime for each task;
    */

    //mra_task_ftm = (struct mra_ftsys_s*)xbt_new(struct mra_ftsys_s*, (total_tasks * (sizeof (struct mra_ftsys_s))));

    mra_dfs_dist = (struct mra_dfs_het_s*)xbt_new(struct mra_dfs_het_s*, (config_mra.mra_number_of_workers * (sizeof (struct mra_dfs_het_s))));

    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    {
        mra_dfs_dist[owner].task_exec[MRA_MAP] 				= 0;
        mra_dfs_dist[owner].task_exec[MRA_REDUCE] 		= 0;
        mra_dfs_dist[owner].avg_task_exec[MRA_MAP] 		= 0;
        mra_dfs_dist[owner].avg_task_exec[MRA_REDUCE] = 0;
        mra_dfs_dist[owner].prev_exec[MRA_MAP] 				= 0;
        mra_dfs_dist[owner].prev_exec[MRA_REDUCE] 		= 0;
        mra_dfs_dist[owner].temp_corr[MRA_MAP] 				= 0;
        mra_dfs_dist[owner].temp_corr[MRA_REDUCE] 		= 0;
        mra_dfs_dist[owner].mra_dist_data[MRA_MAP] 		= 0;
        mra_dfs_dist[owner].mra_dist_data[MRA_REDUCE] = 0;
        mra_dfs_dist[owner].p_cap_wid 								= 0;
        mra_dfs_dist[owner].dist_bruta								=	0;
        mra_dfs_dist[owner].speed											= 0;
        mra_dfs_dist[owner].mra_dist_fail[MRA_MAP]		= 0;
        mra_dfs_dist[owner].mra_dist_fail[MRA_REDUCE]	= 0;
        mra_dfs_dist[owner].mra_calc_dist							= 0;

    }

//    dist_bruta 		= xbt_new (int, 		(config_mra.mra_number_of_workers * sizeof (int)));
    tasks_reduce 	= xbt_new (int, 		(config_mra.mra_number_of_workers * sizeof (int)));
    avg_task_exec_reduce = xbt_new (double, (config_mra.mra_number_of_workers * sizeof (double)));
    prev_exec_reduce = xbt_new (double, (config_mra.mra_number_of_workers * sizeof (double)));
    mra_affinity  		= xbt_new (int, (config_mra.mra_chunk_count * sizeof (int)));

    /* Vectors initialize. */
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    {
        tasks_reduce[owner]					=	0;
        avg_task_exec_reduce[owner]	=	0;
        prev_exec_reduce[owner]			=	0;
    }

    for (chunk = 0; chunk < config_mra.mra_chunk_count; chunk++)
    {
        mra_affinity[chunk]	=	0;
    }

    mra_dfs_het_f.dist_sum = 0;

    log = fopen ("worker_cap.log", "w");
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    {

        mra_dfs_dist->speed = MSG_get_host_speed(config_mra.workers_mra[owner]);
        mra_dfs_dist[owner].speed = mra_dfs_dist->speed;

        /* Calculate a capacity fraction  from grid to worker */
        mra_dfs_dist->p_cap_wid = mra_dfs_dist[owner].speed/config_mra.grid_cpu_power;
        mra_dfs_dist[owner].p_cap_wid = mra_dfs_dist->p_cap_wid;

        /* FIXME Make a funcion chunk vs cost*/
        /* Calculates the runtime for a task into worker*/
        mra_dfs_dist->task_exec[MRA_MAP] = user_mra.task_mra_cost_f (MRA_MAP,0, owner)/mra_dfs_dist[owner].speed;
        mra_dfs_dist[owner].task_exec[MRA_MAP] = mra_dfs_dist->task_exec[MRA_MAP];

        /* Defines a dist_bruta for worker */
        mra_dfs_dist[owner].dist_bruta = (int) ceil(mra_dfs_dist[owner].p_cap_wid * config_mra.mra_chunk_count);

        /* Calculates a runtime preview for a worker*/
        mra_dfs_dist->prev_exec[MRA_MAP] = ((mra_dfs_dist[owner].dist_bruta*mra_dfs_dist[owner].task_exec[MRA_MAP])/config_mra.mra_slots[MRA_MAP]);
        mra_dfs_dist[owner].prev_exec[MRA_MAP] = mra_dfs_dist->prev_exec[MRA_MAP];

        /* Calculates a offset error preview for a worker*/
        mra_dfs_dist->temp_corr[MRA_MAP] = mra_dfs_dist[owner].prev_exec[MRA_MAP] + mra_dfs_dist[owner].task_exec[MRA_MAP];
        mra_dfs_dist[owner].temp_corr[MRA_MAP] = mra_dfs_dist->temp_corr[MRA_MAP];

        mra_dfs_het_f.dist_sum = mra_dfs_het_f.dist_sum + mra_dfs_dist[owner].dist_bruta;

        fprintf (log, " %s , ID: %zu \t Cap_Per: %f \t te_ex: %g \t Dist_B: %u \t Soma: %u \t Pre_ex: %g \t Tem_Cor: %g\n",
                 MSG_host_get_name (config_mra.workers_mra[owner]), owner, mra_dfs_dist[owner].p_cap_wid,mra_dfs_dist[owner].task_exec[MRA_MAP],
                 mra_dfs_dist[owner].dist_bruta, mra_dfs_het_f.dist_sum,mra_dfs_dist[owner].prev_exec[MRA_MAP], mra_dfs_dist[owner].temp_corr[MRA_MAP]);
    }
    fclose (log);

    /** @brief Define a offset to min_max algorithm. */
    mra_dfs_het_f.adjust = mra_dfs_het_f.dist_sum - config_mra.mra_chunk_count;

    /** @brief Executes a mim_max algorithm to adjust the distribution data. */
    min_max_f(mra_dfs_het_f.adjust, config_mra.mra_chunk_count, NORMAL );


    /** @brief avg_task_exec_map - average runtime from each Map task related with the owner. */
    log_avg = fopen ("avg_tasks_map.log", "w");
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    {

       // mra_dfs_dist[owner].mra_dist_data[MRA_MAP] = mra_dfs_dist[owner].dist_bruta;

        for (mra_tid=0; mra_tid < config_mra.mra_number_of_workers; mra_tid++)
        {
            if (mra_dfs_dist[owner].dist_bruta == mra_dfs_dist[mra_tid].dist_bruta )
            {
                avg_t_exec = mra_dfs_dist[mra_tid].task_exec[MRA_MAP] + avg_t_exec;
                cont_avg++;
            }
        }

        mra_dfs_dist->avg_task_exec[MRA_MAP] = (avg_t_exec/cont_avg)*100;
        mra_dfs_dist[owner].avg_task_exec[MRA_MAP] = mra_dfs_dist->avg_task_exec[MRA_MAP];
        fprintf (log_avg,"Owner: %zu \t Avg_task_exec(ms): %g \n ", owner, mra_dfs_dist[owner].avg_task_exec[MRA_MAP]);
        avg_t_exec=0;
        cont_avg=0;
    }
    fclose (log_avg);


    /** @brief Calculation of the Reduce Task number: gets the task runtime of workers and calculates relation between the minimum time (min_te_exec)  and the faster worker.
    	* The value sum  (tot_tasks_reduce) defines the amount of Reduce task going to execute.
     */

    min_te_exec= mra_dfs_dist[0].task_exec[MRA_MAP];
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    {
        if (min_te_exec > mra_dfs_dist[owner].task_exec[MRA_MAP])
        {
            min_te_exec = mra_dfs_dist[owner].task_exec[MRA_MAP];
            // printf("Valor retornado-if %f\n", min_te_exec);
        }
    }

    log_avg = fopen ("avg_tasks_reduce.log", "w");

    /** @brief avg_task_exec_reduce -  average runtime Reduce from each group.  */
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    {
        for (mra_tid=0; mra_tid < config_mra.mra_number_of_workers; mra_tid++)
        {
            if (mra_dfs_dist[owner].dist_bruta == mra_dfs_dist[mra_tid].dist_bruta )
            {
                avg_t_exec = mra_dfs_dist[mra_tid].task_exec[MRA_MAP] + avg_t_exec;
                cont_avg++;
            }
        }
        avg_task_exec_reduce[owner]= (avg_t_exec/cont_avg)*100;
        fprintf (log_avg,"Owner: %zu \t Avg_task_exec (ms): %g \n ", owner, avg_task_exec_reduce[owner]);
        avg_t_exec=0;
        cont_avg=0;
    }
    fclose (log_avg);

    log = fopen ("tasks_reduce.log", "w");
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    {
        if (mra_dfs_dist[owner].dist_bruta > 0)
        {
            tasks_reduce[owner] = (int) ceil(mra_dfs_dist[owner].task_exec[MRA_MAP]/min_te_exec);
        }
        tot_tasks_reduce = tot_tasks_reduce + tasks_reduce[owner];
        prev_exec_reduce[owner] = mra_dfs_dist[owner].dist_bruta*mra_dfs_dist[owner].task_exec[MRA_MAP];
        fprintf (log, " %s , ID: %zu \t Dist_Bruta: %u \t Reduces: %u \t Te_exec_Min: %g \t Tarefas_Reduce %u \t Prev_exec_reduce %g \n",
                 MSG_host_get_name (config_mra.workers_mra[owner]), owner,mra_dfs_dist[owner].dist_bruta, tasks_reduce[owner], min_te_exec, tot_tasks_reduce, prev_exec_reduce[owner]);
    }
    fclose (log);

    /** @brief Distribution array of capacity for each node. The same replica number are grouped.
      */

    min_dist = 99999;
    for (owner = max_dist = 0; owner < config_mra.mra_number_of_workers; owner++)
    {
        if (max_dist < mra_dfs_dist[owner].dist_bruta)
        {
            max_dist = mra_dfs_dist[owner].dist_bruta;
            mra_dist_manage.max_tot_dist = max_dist;
            //printf ("Max_dist: %d \n", max_dist );
        }
        if (mra_dfs_dist[owner].dist_bruta <= min_dist && mra_dfs_dist[owner].dist_bruta > 0 )
        {
            min_dist = mra_dfs_dist[owner].dist_bruta;
            mra_dist_manage.min_tot_dist = min_dist;
            //printf ("Min_dist: %d \n", min_dist );
        }
        mra_dfs_het_f.max_dist = mra_dist_manage.max_tot_dist;
        mra_dfs_het_f.min_dist = mra_dist_manage.min_tot_dist;
    }


    mra_dist_manage.total_dist = xbt_new (int, ((mra_dist_manage.max_tot_dist + 1) * sizeof (int)));
    for (i=0; i < (mra_dist_manage.max_tot_dist + 1); i++)
    {
        mra_dist_manage.total_dist[i]=0;
    }
    log = fopen ("total_dist.log", "w");

    while (tot_dist < mra_dist_manage.max_tot_dist + 1 )
    {
        rep_wid = 1;
        for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
        {
            if ( tot_dist == mra_dfs_dist[owner].dist_bruta )
            {
                mra_dist_manage.total_dist [tot_dist] = rep_wid++;
            }
        }
        fprintf(log,"Machine Numb. Dist %u : %u \n ", tot_dist, mra_dist_manage.total_dist [tot_dist]);
        tot_dist++;
    }
    fclose (log);
    int maxDistBruta=0;
	for (owner = 0; owner < config_mra.mra_number_of_workers  ; owner++)
	{
		if(mra_dfs_dist[owner].dist_bruta > maxDistBruta)
			maxDistBruta = mra_dfs_dist[owner].dist_bruta ;
	}
     int *totalOwnedChunks= (int *) calloc(config_mra.mra_number_of_workers,sizeof(int));
    chunk = 0;
    log = fopen ("map_chunk.log", "w");
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++) //for each owner in machine
    {
        total_chunk=0;
        for (dist=0; dist < mra_dfs_dist[owner].dist_bruta; dist++) // while group number < owner group number
        {
            while ( total_chunk < mra_dfs_dist[owner].dist_bruta) // while total chunks < owner group number
            {
                if (mra_dfs_dist[owner].dist_bruta >0 && chunk < config_mra.mra_chunk_count)
                {
                    chunk_owner_mra[chunk][owner] = 1;
                    job_mra.mra_task_dist[MRA_MAP][owner][total_chunk] = chunk;
                    chunk++;


//                    fprintf (log,"dist: %u \t dist_b:%u \t ID: %zu \t chunk: %zu \t total_chunk: %u \n",dist, mra_dfs_dist[owner].dist_bruta, owner, chunk, total_chunk);
			fprintf (log,"dist: %u \t dist_b:%u \t ID: %zu \t chunk: %zu  \n",dist, mra_dfs_dist[owner].dist_bruta, owner, chunk);
                }
                total_chunk++;
            }
        }
        fprintf(log,"ID : %zu total_Chunk: %u dist_b: %u \n", owner, total_chunk,mra_dfs_dist[owner].dist_bruta);
        totalOwnedChunks[owner]=total_chunk;

    }
    fclose (log);

    /** @brief Data Replication Algorithm
      */
 //   mra_replica_f();
    mra_replica_f (totalOwnedChunks);

    log = fopen ("affinity.log", "w");
    for (chunk = 0; chunk < config_mra.mra_chunk_count; chunk++)
    {
        mra_affinity_f(chunk);
        fprintf (log, "Affinity chunks owned: %zd = %d \n", chunk, mra_affinity[chunk]);
    }
    fclose (log);


    /* Save the distribution to a log file. */
    log = fopen ("chunks.log", "w");
    xbt_assert (log != NULL, "Error creating log file.");
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    {
        fprintf (log, "worker %06zu | ", owner);
        counter = 0;
        for (chunk = 0; chunk < config_mra.mra_chunk_count; chunk++)
        {
            fprintf (log, "%d", chunk_owner_mra[chunk][owner]);
            if (chunk_owner_mra[chunk][owner])
            {
                counter++;
            }
        }
        fprintf (log, " | chunks owned: %d | Dist Bruta: %d \n ", counter,mra_dfs_dist[owner].dist_bruta);
    }

    fclose (log);

}

/** @brief Data Replication Algorithm
  * @brief Affinity function maintains the replica factor for all chunks.
  * @brief First: Creates a data replica from owner in a same group.
  * @brief Second: If replica factor isn't equal to user defined, it creates a data replica from owner in different groups.
  */
void mra_replica_f (int *totalOwnedChunks)
{
    size_t 			owner;
    size_t 			chunk;
    int         j,i = 0, k = 0, dist_test = 0;
    FILE*       log;

    log = fopen ("replicas.log", "w");
    // Affinity test - Begin
    for (k = 0; k < config_mra.mra_chunk_count - 1; k++)
    {
        mra_affinity_f(k);
    }
    // Affinity test - End
    // Test total dist
    for (i=0; i < (mra_dist_manage.max_tot_dist + 1); i++)
    {
        if (mra_dist_manage.total_dist[i] == config_mra.amount_of_tasks_mra[MRA_MAP])
        {
          dist_test = 1;
        }
    }



    /** @brief Creates a data replica from owner in a same group */

    for (owner = 0; owner < config_mra.mra_number_of_workers  ; owner++)
    {
        for (chunk = 0; (chunk < config_mra.mra_chunk_count) ; chunk++)
        {
            if (chunk_owner_mra[chunk][owner] == 1 && mra_dfs_dist[owner].dist_bruta != 0)
            {

                for (i = config_mra.mra_number_of_workers-1; i >= 0 ; i--)
                {
                  if ((mra_affinity[chunk] < config_mra.mra_chunk_replicas) &&  owner != i
                     	 && (totalOwnedChunks[i] <= (config_mra.mra_chunk_count/config_mra.mra_number_of_workers))
                       && chunk_owner_mra[chunk][i] == 0 && mra_dfs_dist[i].dist_bruta!=0)
                       {
  		                    fprintf (log,"Owner: %zu \t chunk: %zu \t Replica to:%d \t DistBruta: %d \n",owner,chunk, i, mra_dfs_dist[owner].dist_bruta);
  		                    chunk_owner_mra[chunk][i] = 1;
  		                    totalOwnedChunks[i]++;
  		                    mra_affinity[chunk] = mra_affinity[chunk] + 1;
		                    }

                	}

            }
        }
    }

    fprintf (log,"\n Replicas to owner in different groups \n");
    /* @brief Creates a replica to owner in different groups */
    //mesmo laço, porém distribuindo para workers de distBruta diferente

    for (owner = 0; owner < config_mra.mra_number_of_workers  ; owner++)
    {
        for (chunk = 0; (chunk < config_mra.mra_chunk_count) ; chunk++)
        {
            if (chunk_owner_mra[chunk][owner] == 1 && mra_dfs_dist[owner].dist_bruta !=0 && dist_test != 1)
            {
                while (mra_affinity[chunk] < config_mra.mra_chunk_replicas)
                {
                    j = rand () % config_mra.mra_number_of_workers;
                    while (owner == j)
                    {
                        j = rand () % config_mra.mra_number_of_workers;
                       // j = rand () % (mra_dist_manage.max_tot_dist + mra_dist_manage.min_tot_dist) ;
                    }
                    if (mra_dfs_dist[owner].dist_bruta != mra_dfs_dist[j].dist_bruta && chunk_owner_mra[chunk][j] == 0 && mra_dfs_dist[j].dist_bruta!=0 )
                    {
                        if ((mra_affinity[chunk] < config_mra.mra_chunk_replicas)  )
                        {
                            fprintf (log,"Owner: %zu \t chunk: %zu \t Replica to:%d \n",owner,chunk, j);
                            chunk_owner_mra[chunk][j] = 1;
                            mra_affinity[chunk] = mra_affinity[chunk] + 1;
                        }
                    }
                }
            }
        }
    }

    fclose (log);
}

/**
   * @brief Min_max Algorithm: It is a brute-force adjustment with a combinatorial optimization to find a chunk distribution with the smaller runtime possible.
   * @brief max_exec_total --> Finds the higher runtime prediction, reduces one unit from chunk number and recalculates again prev_exe, temp_corr and dist_sum.
   * @brief max_prev_exec--> Finds the higher runtime prediction for each worker.
   * @param adjust: Offset
   * @param chunk_num: total number of chunks to be distributed.
   * @param fault_mode: operation mode.
   */

void min_max_f(int adjust, int chunk_num, int fault_mode)
{
    size_t 		mra_tid;
    size_t 		owner;
    double   	max_prev_exec;
    double   	time_sum = 0;
    int      	idmin = 0;
    int      	dist_sum = 0;
    double   	minimal_task = 0;
    double   	min_task_exec;
    int      id1, idmax = 0;
    int      tot_sum = 0;
    double   min_temp_corr;
    double   min_max = 1;
    double   dist_min =1;

    FILE*			log;

    //char filename[24];
    //sprintf (filename, "Dist_Bruta%d.log", adjust);

    max_prev_exec = mra_dfs_dist[0].prev_exec[MRA_MAP];
    /** @brief max_prev_exec calculation*/

    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    {
        if (max_prev_exec < mra_dfs_dist[owner].prev_exec[MRA_MAP])
        {
            max_prev_exec = mra_dfs_dist[owner].prev_exec[MRA_MAP];
            mra_tid = owner;
        }
    }
    /** @brief Reduces one unit from chunk number and recalculates again prev_exe, temp_corr and dist_sum.       	 */
    //log = fopen (filename, "w");
    log = fopen ("Dist_Bruta.log","w");
    owner = mra_tid;

    if (adjust > 0)
    {
        for (owner = 0; adjust != time_sum ; owner++)
        {

            switch (fault_mode)
            {
            case NORMAL:
                mra_dfs_dist[owner].dist_bruta= mra_dfs_dist[owner].dist_bruta - 1;
                time_sum=(time_sum + 1);
                break;

            case FAILURE:
                if (mra_dfs_dist[owner].dist_bruta > 0)
                {
                    mra_dfs_dist[owner].dist_bruta= mra_dfs_dist[owner].dist_bruta - 1;
                    time_sum=(time_sum + 1);
                }
                break;
            }
        }
    }
    else
    {
        for (owner = 0; adjust != time_sum  ; owner++)
        {
            switch (fault_mode)
            {
            case NORMAL:
                mra_dfs_dist[owner].dist_bruta= mra_dfs_dist[owner].dist_bruta + 1;
                time_sum=(time_sum - 1);
                break;

            case FAILURE:
                if (mra_dfs_dist[owner].dist_bruta > 0)
                {
                    mra_dfs_dist[owner].dist_bruta= mra_dfs_dist[owner].dist_bruta + 1;
                    time_sum=(time_sum - 1);
                }
                break;
            }
        }
    }

    dist_sum=0;
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    {
        mra_dfs_dist[owner].prev_exec[MRA_MAP] = ((mra_dfs_dist[owner].dist_bruta*mra_dfs_dist[owner].task_exec[MRA_MAP])/config_mra.mra_slots[MRA_MAP]);
        mra_dfs_dist[owner].temp_corr[MRA_MAP] = mra_dfs_dist[owner].prev_exec[MRA_MAP] + mra_dfs_dist[owner].task_exec[MRA_MAP];

        dist_sum = dist_sum + mra_dfs_dist[owner].dist_bruta;
        /* Print Dist_Bruta.log*/
        fprintf (log, " %s , ID: %zu \t Dist_Recalc: %u \t Soma: %u \t Chunks %u \t Pre_ex= %g \t Tem_Cor= %g\n ",
                 MSG_host_get_name (config_mra.workers_mra[owner]),owner,mra_dfs_dist[owner].dist_bruta,dist_sum, chunk_num, mra_dfs_dist[owner].prev_exec[MRA_MAP],mra_dfs_dist[owner].temp_corr[MRA_MAP]);
    }
    fclose (log);

    /**
    * @brief Algoritmo_minMax adjust
    * Find a chunk distribution with the smaller runtime possible.
    */
    min_temp_corr = mra_dfs_dist[0].temp_corr[MRA_MAP];
    max_prev_exec = mra_dfs_dist[0].prev_exec[MRA_MAP];
    min_task_exec = mra_dfs_dist[0].task_exec[MRA_MAP];
    while ((minimal_task < dist_min) && tot_sum < config_mra.mra_number_of_workers )
    {
        /** @brief Reduces one unit from chunk number and recalculates again prev_exe, temp_corr and dist_sum.       	 */
        for (idmax = 0; idmax < config_mra.mra_number_of_workers; idmax++)
        {
            if (max_prev_exec <= mra_dfs_dist[idmax].prev_exec[MRA_MAP])
            {
                max_prev_exec = mra_dfs_dist[idmax].prev_exec[MRA_MAP];
                mra_tid = idmax;
            }
        }
        for (idmin = 0; idmin < config_mra.mra_number_of_workers; idmin++)
        {
            if (mra_dfs_dist[idmin].task_exec[MRA_MAP] > 0)
            {

                if (min_task_exec >= mra_dfs_dist[idmin].task_exec[MRA_MAP])
                {
                    min_task_exec = mra_dfs_dist[idmin].task_exec[MRA_MAP];
                }
                if (min_temp_corr >= mra_dfs_dist[idmin].temp_corr[MRA_MAP])
                {
                    min_temp_corr = mra_dfs_dist[idmin].temp_corr[MRA_MAP];
                    id1 = idmin;
                }
            }
        }

        min_max = max_prev_exec - min_temp_corr;

        /* Change chunk owner. */
        mra_dfs_dist[mra_tid].dist_bruta= mra_dfs_dist[mra_tid].dist_bruta - 1;
        mra_dfs_dist[id1].dist_bruta= mra_dfs_dist[id1].dist_bruta+1;
        dist_sum=0;

        //sprintf (filename, "Dist_Fina%d.log", adjust);

        log = fopen ("Dist_Fina.log", "w");

        /* Recalculates again prev_exe, temp_corr and dist_sum.  */
        for (owner = 0; owner < config_mra.mra_number_of_workers ; owner++)
        {
            mra_dfs_dist[owner].prev_exec[MRA_MAP] = mra_dfs_dist[owner].dist_bruta*mra_dfs_dist[owner].task_exec[MRA_MAP];
            mra_dfs_dist[owner].temp_corr[MRA_MAP] = mra_dfs_dist[owner].prev_exec[MRA_MAP]+ mra_dfs_dist[owner].task_exec[MRA_MAP];
            dist_sum = dist_sum + mra_dfs_dist[owner].dist_bruta;
            if (fault_mode == NORMAL)
            {
                mra_dfs_dist[owner].mra_calc_dist = mra_dfs_dist[owner].dist_bruta;
            }
            fprintf (log, " %s , ID: %zu \t Dist_Recalc: %u \t Soma: %u \t Te_exec = %g \t Pre_ex= %g \t Tem_Cor= %g\n",
                     MSG_host_get_name (config_mra.workers_mra[owner]),owner,mra_dfs_dist[owner].dist_bruta,dist_sum,mra_dfs_dist[owner].task_exec[MRA_MAP],mra_dfs_dist[owner].prev_exec[MRA_MAP],mra_dfs_dist[owner].temp_corr[MRA_MAP]);

        }

        fclose (log);
        dist_min = min_max;
        minimal_task = min_task_exec;
        tot_sum++;
        min_temp_corr = mra_dfs_dist[0].temp_corr[MRA_MAP];
        max_prev_exec = mra_dfs_dist[0].prev_exec[MRA_MAP];
        min_task_exec = mra_dfs_dist[0].task_exec[MRA_MAP];
    }


}

/** @brief mra_affinity: maintains a replica factor for a chunk.
*
*/

void mra_affinity_f (size_t chunk)
{
    int rpl=0;
    int i=0 ;
    i=0;
    while(i < config_mra.mra_number_of_workers)
    {
        if (chunk_owner_mra[chunk][i] == 1 )
        {
            rpl++;
        }
        i++;
    }
    mra_affinity[chunk] = rpl;
}


/** @brief ftm_mra_affinity: recovery a replica factor for a chunk.
*
*/

void ftm_mra_affinity_f (int mra_id_task, size_t mra_ftm_vc_wid)
{
    int rpl=0, chunk, i=0;
    size_t  mra_wid = mra_ftm_vc_wid;

    chunk = job_mra.mra_task_dist[MRA_MAP][mra_ftm_vc_wid][mra_id_task];

    while(i < config_mra.mra_number_of_workers )
    {
        if (chunk_owner_mra[chunk][i] == 1 && (mra_ftm_done_s[i].mra_ft_vcstat != VC_FAILURE))
        {
            rpl++;
        }
        i++;
    }
    i=0;
    for (i=0; i < config_mra.mra_number_of_workers; i++ )
    {
    if ((mra_dfs_dist[mra_wid].dist_bruta == mra_dfs_dist[i].dist_bruta ) && rpl < mra_affinity[chunk] && (mra_ftm_done_s[i].mra_ft_vcstat != VC_FAILURE))
    {
       chunk_owner_mra[chunk][i] = 1 ;
       //XBT_INFO ("FTM chunk_owner %d and i %d",chunk, i);
       rpl++;
    }
    else if ((mra_dfs_dist[mra_wid].dist_bruta + 1 == mra_dfs_dist[i].dist_bruta ) && rpl < mra_affinity[chunk] && (mra_ftm_done_s[i].mra_ft_vcstat != VC_FAILURE))
     {
        chunk_owner_mra[chunk][i] = 1 ;
        rpl++;
     }
    else if ((mra_dfs_dist[mra_wid].dist_bruta - 1 == mra_dfs_dist[i].dist_bruta ) && rpl < mra_affinity[chunk] && (mra_ftm_done_s[i].mra_ft_vcstat != VC_FAILURE))
     {
        chunk_owner_mra[chunk][i] = 1 ;
        rpl++;
     }
     }
    mra_affinity[chunk] = rpl;
    //XBT_INFO ("\n FTM Chunk Replica %d : %d restored \n", chunk, mra_affinity[chunk]);

}

/** @brief Assign a map task recovery to worker has been late. */
void mra_vc_task_assing (size_t owner, size_t chunk)
{
    FILE*    		log;
    int         counter;

    if (chunk_owner_mra[chunk][owner] == 0)
    {
        chunk_owner_mra[chunk][owner] = 1;
    }
    log = fopen ("recovery_chunks.log", "w");
    xbt_assert (log != NULL, "Error creating log file.");
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    {
        fprintf (log, "worker %06zu | ", owner);
        counter = 0;
        for (chunk = 0; chunk < config_mra.mra_chunk_count; chunk++)
        {
            fprintf (log, "%d", chunk_owner_mra[chunk][owner]);
            if (chunk_owner_mra[chunk][owner])
            {
                counter++;
            }
        }
        fprintf (log, " | chunks owned: %d\n", counter);
    }

    fclose (log);

}

/**
* @brief  Choose a random DataNode that owns a specific chunk.
* @brief  Distribution of Data Replication
* @param  cid  The chunk ID.
* @return The ID of the DataNode.
*/
size_t find_random_mra_chunk_owner (int cid)
{
    int     replica;
    size_t  owner = NONE;
    size_t  mra_wid;

    replica = rand () % config_mra.mra_chunk_replicas;
    for (mra_wid = 0; mra_wid < config_mra.mra_number_of_workers ; mra_wid++)
    {
        if (chunk_owner_mra[cid][mra_wid] && (mra_ftm_done_s[mra_wid].mra_ft_vcstat != VC_FAILURE))
        {
            owner = mra_wid;

            if (replica == 0)
          break;
            else
            replica--;
       }
    }
   /* if(behavior[mra_wid]== NEW_WID)
    {
        xbt_assert (owner != NONE, "MRA_Aborted: chunk %d is missing.", cid);
    }
    else
    { */

   xbt_assert (owner != NONE, "MRA_Aborted: chunk %d is missing", cid);

   return owner;
}

/** @brief  DataNode main function. */

int data_node_mra (int argc, char* argv[])
{
    char         mailbox[MAILBOX_ALIAS_SIZE];
    msg_error_t  status;
    msg_task_t   msg = NULL;
    sprintf (mailbox, DATANODE_MRA_MAILBOX, get_mra_worker_id (MSG_host_self ()));

    size_t wid =get_mra_worker_id (MSG_host_self ())+1 ;
    mra_task_pid.data_node[wid] = MSG_process_self_PID ();
    
    while (!job_mra.finished)
    {
        msg = NULL;
        status = receive (&msg, mailbox);
        if (status == MSG_OK)
        {
            if (mra_message_is (msg, SMS_FINISH_MRA))
            {
                MSG_task_destroy (msg);
                break;
            }
            else
            {
                send_mra_data (msg);
            }
        }
    }

    return 0;
}

/**
* @brief  Process that responds to data requests.
*/

static void send_mra_data (msg_task_t msg)
{
    char         mailbox[MAILBOX_ALIAS_SIZE];
    double       data_size;
    size_t       my_id;
    mra_task_info_t  ti;


    my_id = get_mra_worker_id (MSG_host_self ());
    sprintf (mailbox, TASK_MRA_MAILBOX, get_mra_worker_id (MSG_task_get_source (msg)), MSG_process_get_PID (MSG_task_get_sender (msg)));
    if (mra_message_is (msg, SMS_GET_MRA_CHUNK))
    {
        MSG_task_dsend (MSG_task_create ("DATA-C", 0.0, config_mra.mra_chunk_size, NULL), mailbox, NULL);
    }
    else if (mra_message_is (msg, SMS_GET_INTER_MRA_PAIRS))
    {
        ti = (mra_task_info_t) MSG_task_get_data (msg);
        data_size = job_mra.map_output[my_id][ti->mra_tid] - ti->map_output_copied[my_id];
        MSG_task_dsend (MSG_task_create ("DATA-IP", 0.0, data_size, NULL), mailbox, NULL);

    }

    MSG_task_destroy (msg);
}
