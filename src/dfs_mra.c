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

#include <math.h>
#include <msg/msg.h>
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
    int*     total_dist; 
    double*  prev_exec_reduce=NULL;  
    int      counter, i;
    int      soma_dist = 0;
    int      tot_tasks_reduce=0;
    int      mra_tid, idmin = 0;
    int      id1, idmax = 0;
    int      soma_tot=0;
    int	     max_dist, min_dist, total_chunk, rep_wid;
    int      cont_avg=0;
    int      tot_dist=0;
    int      adjust=0;
    int 	   dist=0;
    double   soma_temp = 0;
    double   min_te_exec ; 
    double   max_prev_exec;
    double   min_temp_corr;
    double   min_max= 1;
    double   dist_min=1;
    double   minimo_task=0;
    double   min_task_exec;
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
      
    }

    dist_bruta 		= xbt_new (int, 		(config_mra.mra_number_of_workers * sizeof (int))); 
    tasks_reduce 	= xbt_new (int, 		(config_mra.mra_number_of_workers * sizeof (int)));
    avg_task_exec_reduce = xbt_new (double, (config_mra.mra_number_of_workers * sizeof (double))); 
    prev_exec_reduce = xbt_new (double, (config_mra.mra_number_of_workers * sizeof (double)));
    mra_affinity  		= xbt_new (int, (config_mra.mra_chunk_count * sizeof (int))); 
   
   /* Vectors initialize. */
   for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
   	{
       dist_bruta[owner]						=	0; 
       tasks_reduce[owner]					=	0;
       avg_task_exec_reduce[owner]	=	0; 
       prev_exec_reduce[owner]			=	0;
   	}
   
    for (chunk = 0; chunk < config_mra.mra_chunk_count; chunk++)
    	{
       	mra_affinity[chunk]	=	0; 
   		}   
   
    log = fopen ("worker_cap.log", "w");	
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)              
    	{
				mra_dfs_dist->p_cap_wid = MSG_get_host_speed(config_mra.workers_mra[owner])/config_mra.grid_cpu_power;
				mra_dfs_dist[owner].p_cap_wid = mra_dfs_dist->p_cap_wid;
				
				/* FIXME Make a funcion chunk vs cost*/
				mra_dfs_dist->task_exec[MRA_MAP] = user_mra.task_mra_cost_f (MRA_MAP,0, owner)/MSG_get_host_speed(config_mra.workers_mra[owner]);
				mra_dfs_dist[owner].task_exec[MRA_MAP] = mra_dfs_dist->task_exec[MRA_MAP];
				
				dist_bruta[owner] = (int) ceil(mra_dfs_dist[owner].p_cap_wid * config_mra.mra_chunk_count);
				
				mra_dfs_dist->prev_exec[MRA_MAP] = ((dist_bruta[owner]*mra_dfs_dist[owner].task_exec[MRA_MAP])/config_mra.mra_slots[MRA_MAP]);
				mra_dfs_dist[owner].prev_exec[MRA_MAP] = mra_dfs_dist->prev_exec[MRA_MAP];
				
				mra_dfs_dist->temp_corr[MRA_MAP] = mra_dfs_dist[owner].prev_exec[MRA_MAP] + mra_dfs_dist[owner].task_exec[MRA_MAP];
				mra_dfs_dist[owner].temp_corr[MRA_MAP] = mra_dfs_dist->temp_corr[MRA_MAP];
				
				soma_dist = soma_dist + dist_bruta[owner];

				fprintf (log, " %s , ID: %zu \t Cap_Per: %f \t te_ex: %g \t Dist_B: %u \t Soma: %u \t Pre_ex: %g \t Tem_Cor: %g\n", 
				MSG_host_get_name (config_mra.workers_mra[owner]), owner, mra_dfs_dist[owner].p_cap_wid,mra_dfs_dist[owner].task_exec[MRA_MAP],
				dist_bruta[owner], soma_dist,mra_dfs_dist[owner].prev_exec[MRA_MAP], mra_dfs_dist[owner].temp_corr[MRA_MAP]);
    	}
    fclose (log);
    
  /**
    * @brief Min_Max Algorithm: It is a brute-force adjustment with a combinatorial optimization to find a chunk distribution with the smaller runtime possible.
    * @brief max_exec_total --> Finds the higher runtime prediction, reduces one unit from chunk number and recalculates again prev_exe, temp_corr and soma_dist.
    * @brief max_prev_exec--> Finds the higher runtime prediction for each worker. 
    */ 
		max_prev_exec = mra_dfs_dist[0].prev_exec[MRA_MAP];
		    /** @brief max_prev_exec calculation*/
         adjust = (soma_dist - config_mra.mra_chunk_count);	
       		 		for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
       	      	{ 
  	      				if (max_prev_exec < mra_dfs_dist[owner].prev_exec[MRA_MAP])
  	           			{
      		     				max_prev_exec = mra_dfs_dist[owner].prev_exec[MRA_MAP];
      		     				mra_tid = owner;
                		}
    	     			}
    	/** @brief Reduces one unit from chunk number and recalculates again prev_exe, temp_corr and soma_dist.       	 */
       			log = fopen ("Dist_Bruta.log", "w");
        		owner = mra_tid;
        		soma_temp=0;
        		if (adjust > 0) 
        			{
        				for (owner = 0; adjust != soma_temp; owner++)
        					{
        		  			dist_bruta[owner]= dist_bruta[owner]-1;
        		  			soma_temp=(soma_temp+1);
        					}  
        			}		
        		else 
        		  {
        		  	for (owner = 0; adjust != soma_temp; owner++)
        		  		{
        		  			dist_bruta[owner]= dist_bruta[owner]+1;
        		  			soma_temp=(soma_temp-1);
        					}
        			}	
        			
    	      soma_dist=0;
    				for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    	   				{	    	
    	     				mra_dfs_dist[owner].prev_exec[MRA_MAP] = ((dist_bruta[owner]*mra_dfs_dist[owner].task_exec[MRA_MAP])/config_mra.mra_slots[MRA_MAP]);
	     						mra_dfs_dist[owner].temp_corr[MRA_MAP] = mra_dfs_dist[owner].prev_exec[MRA_MAP] + mra_dfs_dist[owner].task_exec[MRA_MAP];
	     						soma_dist = soma_dist + dist_bruta[owner];
	              
	               /* Print Dist_Bruta.log*/
	     						fprintf (log, " %s , ID: %zu \t Dist_Recalc: %u \t Soma: %u \t Chunks %u \t Pre_ex= %g \t Tem_Cor= %g\n ",
	     						MSG_host_get_name (config_mra.workers_mra[owner]),owner,dist_bruta[owner],soma_dist,config_mra.mra_chunk_count, mra_dfs_dist[owner].prev_exec[MRA_MAP],mra_dfs_dist[owner].temp_corr[MRA_MAP]);  
    	    			} 
    	  		fclose (log);

      /**
      * @brief Algoritmo_minMax adjust
      * Find a chunk distribution with the smaller runtime possible.   
      */ 
      min_temp_corr = mra_dfs_dist[0].temp_corr[MRA_MAP];
      max_prev_exec = mra_dfs_dist[0].prev_exec[MRA_MAP]; 
      min_task_exec = mra_dfs_dist[0].task_exec[MRA_MAP];
	    while ((minimo_task < dist_min) && soma_tot < config_mra.mra_number_of_workers )    
       	{ 
					/** @brief Reduces one unit from chunk number and recalculates again prev_exe, temp_corr and soma_dist.       	 */
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
    	 
        min_max = max_prev_exec - min_temp_corr;
        
        /* Change chunk owner. */
        dist_bruta[mra_tid]= dist_bruta[mra_tid]-1;
        dist_bruta[id1]= dist_bruta[id1]+1; 	
    		soma_dist=0;
	
    		log = fopen ("Dist_Fina.log", "w");  
    	
    	/* Recalculates again prev_exe, temp_corr and soma_dist.  */
    		for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    			{	    	
    				mra_dfs_dist[owner].prev_exec[MRA_MAP] = dist_bruta[owner]*mra_dfs_dist[owner].task_exec[MRA_MAP];
						mra_dfs_dist[owner].temp_corr[MRA_MAP] = mra_dfs_dist[owner].prev_exec[MRA_MAP]+ mra_dfs_dist[owner].task_exec[MRA_MAP];
						soma_dist = soma_dist + dist_bruta[owner];

    				fprintf (log, " %s , ID: %zu \t Dist_Recalc: %u \t Soma: %u \t Te_exec = %g \t Pre_ex= %g \t Tem_Cor= %g\n",
		  			MSG_host_get_name (config_mra.workers_mra[owner]),owner,dist_bruta[owner],soma_dist,mra_dfs_dist[owner].task_exec[MRA_MAP],mra_dfs_dist[owner].prev_exec[MRA_MAP],mra_dfs_dist[owner].temp_corr[MRA_MAP]);
    	
    			}

    		fclose (log);
    		dist_min = min_max;
    		minimo_task = min_task_exec;
    		soma_tot++;
    		min_temp_corr = mra_dfs_dist[0].temp_corr[MRA_MAP];
      	max_prev_exec = mra_dfs_dist[0].prev_exec[MRA_MAP]; 
      	min_task_exec = mra_dfs_dist[0].task_exec[MRA_MAP];
    	}
      /** @brief avg_task_exec_map - average runtime from each Map task related with the owner. */
      log_avg = fopen ("avg_tasks_map.log", "w"); 
      for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
      	{
         	for (mra_tid=0; mra_tid < config_mra.mra_number_of_workers; mra_tid++)
         		{
         			if (dist_bruta[owner] == dist_bruta[mra_tid] )
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
         		 if (dist_bruta[owner] == dist_bruta[mra_tid] )
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
    	 	if (dist_bruta[owner]>0)
    	 		{   
    				tasks_reduce[owner] = (int) ceil(mra_dfs_dist[owner].task_exec[MRA_MAP]/min_te_exec);
    			}
    	 	tot_tasks_reduce = tot_tasks_reduce + tasks_reduce[owner];
    	 	prev_exec_reduce[owner] = dist_bruta[owner]*mra_dfs_dist[owner].task_exec[MRA_MAP];
    	 	fprintf (log, " %s , ID: %zu \t Reduces: %u \t Te_exec_Min: %g \t Tarefas_Reduce %u \t Prev_exec_reduce %g \n",
			 	MSG_host_get_name (config_mra.workers_mra[owner]), owner, tasks_reduce[owner], min_te_exec, tot_tasks_reduce, prev_exec_reduce[owner]);
     	}   
		fclose (log);

    /** @brief Distribution array of capacity for each node. The same replica number are grouped.  
      */
      
    min_dist = 99999;
    for (owner = max_dist = 0; owner < config_mra.mra_number_of_workers; owner++)
    	{
    	 	if (max_dist < dist_bruta[owner])
     	    {
      			max_dist = dist_bruta[owner];
      			mra_dist_manage.max_tot_dist = max_dist;
      			//printf ("Max_dist: %d \n", max_dist );
          }
        if (dist_bruta[owner] <= min_dist && dist_bruta[owner] > 0 )
     	    {
      			min_dist = dist_bruta [owner];
      			mra_dist_manage.min_tot_dist = min_dist;
      			//printf ("Min_dist: %d \n", min_dist );
          }        
      }
            
		total_dist = xbt_new (int, ((max_dist + 1) * sizeof (int))); 
		for (i=0; i < (max_dist + 1); i++)
		{
		 total_dist[i]=0;
		}
		log = fopen ("total_dist.log", "w");
			
		while (tot_dist < max_dist + 1 )
			{
      	rep_wid = 1;
      	for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
      		{
           	if ( tot_dist == dist_bruta [owner] )
           		{
           			total_dist [tot_dist] = rep_wid++;
           		}
           	
          }
     		//printf("Total Dist %u : %u \n ", tot_dist, total_dist [tot_dist]);
     		fprintf(log,"Machine Numb. Dist %u : %u \n ", tot_dist, total_dist [tot_dist]);
        tot_dist++;
      }
		fclose (log); 
       
		chunk = 0;
		log = fopen ("map_chunk.log", "w");
		for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
			{
    		dist=0;
    		total_chunk=0;
    		while (dist < dist_bruta[owner])
					{
						while ( total_chunk < dist_bruta[owner])
							{
    	   				if (dist_bruta[owner]>0)
    	   					{
    	   						chunk_owner_mra[chunk][owner] = 1;
    	   						chunk++;
         						fprintf (log,"dist: %u \t dist_b:%u \t ID: %zu \t chunk: %zu \t total_chunk: %u \n",dist, dist_bruta[owner], owner, chunk, total_chunk);
	             		}
	           		total_chunk++;
	     				}
	      		dist++;
	   			}	  
 				}          
 		fclose (log); 
         
    /** @brief Data Replication Algorithm
      */     
      mra_replica_f();
	 

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
				fprintf (log, " | chunks owned: %d\n", counter);
    	}
    fclose (log);
    	  	
      
}

/** @brief Data Replication Algorithm
  * @brief Affinity function maintains the replica factor for all chunks.
  * @brief First: Creates a data replica from owner in a same group.
  * @brief Second: If replica factor isn't equal to user defined, it creates a data replica from owner in different groups.
  */
void mra_replica_f (void)
{ 
    size_t 			owner;
    size_t 			chunk;
    int 				j = 0, i = 0, k = 0;
   	FILE*    log;	
   	log = fopen ("replicas.log", "w");
   
		// Affinity test - Begin  
    for (k = 0; k < config_mra.mra_chunk_count; k++)
				{   					
			  			mra_affinity_f(k);	
    		}
    // Affinity test - End
     /** @brief Creates a data replica from owner in a same group */
		  for (owner = 0; owner < config_mra.mra_number_of_workers  ; owner++)
				{        		
			   	for (chunk = 0; (chunk < config_mra.mra_chunk_count) ; chunk++) 
			   		{
			  	  	if (chunk_owner_mra[chunk][owner] == 1) 
			  	   	 {           
           				for (i = 0; i < config_mra.mra_number_of_workers ; i++)
           					{                            
       					  		if (dist_bruta[owner] == dist_bruta[i] && chunk_owner_mra[chunk][i] == 0 ) 
             						{        
              	    			if ((mra_affinity[chunk] < config_mra.mra_chunk_replicas) &&  owner != i )
              	    				{
              	    					chunk_owner_mra[chunk][i] = 1; 
              	    					mra_affinity[chunk] = mra_affinity[chunk] + 1;
              	    				}
              	    			}
              	    }
         					if(mra_affinity[chunk] == config_mra.mra_chunk_replicas)
       							{
           						fprintf (log,"mra_wid: %u \t chunk: %zu \t Replicas:%u \n", i, chunk, mra_affinity[chunk]);
       							}
       				}
       			}
       	}	
     /* @brief Creates a replica to owner in different groups */
		for (owner = 0; owner < config_mra.mra_number_of_workers  ; owner++)
			{        		
			  for (chunk = 0; (chunk < config_mra.mra_chunk_count) ; chunk++) 
			   	{
			  	  if (chunk_owner_mra[chunk][owner] == 1) 
			  	   	{          
           			while (mra_affinity[chunk] < config_mra.mra_chunk_replicas)
           			 {
           					j = rand () % config_mra.mra_number_of_workers + 0;        				   
           					while (owner == j)
           				  	{
           				  		j = rand () % mra_dist_manage.max_tot_dist + mra_dist_manage.min_tot_dist;
           				  	}                                 
       				  		if (dist_bruta[owner] != dist_bruta[j] && chunk_owner_mra[chunk][j] == 0 ) 
             					{        
                  			if ((mra_affinity[chunk] < config_mra.mra_chunk_replicas)  )
                  				{
                  					chunk_owner_mra[chunk][j] = 1; 
                  					mra_affinity[chunk] = mra_affinity[chunk] + 1;
                  				}
                  		}
                 }
         				if(mra_affinity[chunk] == config_mra.mra_chunk_replicas)
       						{
           					fprintf (log,"mra_wid: %u \t chunk: %zu \t Replicas:%u \n", i, chunk, mra_affinity[chunk]);
       						}
       				}
       		}
       	}
   fclose (log); 
}  	


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
						if (chunk_owner_mra[cid][mra_wid] )
							{
	    					owner = mra_wid;
	    						    			
	    					if (replica == 0)
	    						{
										break;
									}
	    					else 
	    						{ 
										replica--;
									}
							}	
    	}

    xbt_assert (owner != NONE, "MRA_Aborted: chunk %d is missing.", cid);

    return owner;
 	}


/** @brief  DataNode main function. */

int data_node_mra (int argc, char* argv[])
 {
    char         mailbox[MAILBOX_ALIAS_SIZE];
    msg_error_t  status;
    msg_task_t   msg = NULL;
    sprintf (mailbox, DATANODE_MRA_MAILBOX, get_mra_worker_id (MSG_host_self ()));

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

