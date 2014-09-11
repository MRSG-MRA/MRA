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


       /* START DISTRIBUTION */

    int      i;
    FILE*    log;
    FILE*    log_avg;
    int      counter;
    double*  p_worker_cap = 0;
    size_t   chunk;
    size_t   owner;
    int      tasks_map=0;
    int      soma_dist = 0;
    double*  prev_exec = NULL;
    double*  temp_corr = NULL;
    double   soma_temp = 0;
    int*     tasks_reduce = NULL;
    int*     total_dist=NULL;
    int      rpl;
    double*  prev_exec_reduce=NULL;
    double   min_te_exec ; 
    int      tot_tasks_reduce=0;
    double   max_prev_exec;
    double   min_temp_corr;
    int      mra_tid, idmin = 0;
    int      id1, idmax = 0;
    int      soma_tot=0;
    double   min_max= 1;
    double   dist_min=1;
    double   minimo_task=0;
    double   min_task_exec;
    int	     max_dist;
    int      min_dist;
    int      total_chunk;
    int      rep_wid;
    int      cont_avg=0;
    int      tot_dist=0;
    double   avg_t_exec=0;
    int      soma_dist_adjust=0;
    int      adjust=0;
    int 	   dist=0;
   // double   adjust_min_prev_exec=0;
    //int      achunk=0;



    /* START DISTRIBUTION - Matrix chunk_owner_mra (chunk,worker)*/

    /**
      * lista de workers --> workers_hosts[id] (array)
      * pegar capacidade --> MSG_get_host_speed (config_mra.workers[owner]) 
      * p_worker_cap --> Calcula a capacidade computacional relativa de cada worker
      * baseado na capacidade total da grid.
      * dist_bruta --> É o array com as tribuições brutas, antes do ajuste de
      * menor te_exec
      * prev_exec  --> É o array com o valor de previsão de término de todas as tarefas distribuídas ao
      * worker;
      * temp_corr --> É o array com o tempo que será utilizado para encontar a melhor distribuição
      * task_exec --> É o array que contém o tempo de cada worker para executar uma tarefa computacional padrão
        
    */

    p_worker_cap = xbt_new0 (double, config_mra.mra_number_of_workers);

    dist_bruta = xbt_new0 (int, config_mra.mra_number_of_workers); 
    
    prev_exec = xbt_new0 (double, config_mra.mra_number_of_workers);
    
    temp_corr = xbt_new0 (double, config_mra.mra_number_of_workers);
    
    task_exec = xbt_new0 (double, config_mra.mra_number_of_workers);
    
       
   
    log = fopen ("worker_cap.log", "w");	
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)              
    {
	p_worker_cap[owner] = MSG_get_host_speed(config_mra.workers_mra[owner])/config_mra.grid_cpu_power;

	tasks_map = config_mra.mra_chunk_count;///config_mra.slots_mra[MRA_MAP];
	
	task_exec[owner] = config_mra.mra_chunk_size /MSG_get_host_speed(config_mra.workers_mra[owner]);
	
	dist_bruta[owner] = (int) ceil(p_worker_cap[owner]*tasks_map);
	
	prev_exec[owner] = ((dist_bruta[owner]*task_exec[owner])/config_mra.mra_slots[MRA_MAP]);
	
	temp_corr[owner] = prev_exec[owner]+task_exec[owner];
	
	soma_dist = soma_dist + dist_bruta[owner];
		
	soma_temp= task_exec[owner]+soma_temp;

	fprintf (log, " %s , ID: %zu \t  Cap_Perc: %f \t Soma=%g \t te_exec= %g \t Dist_B: %u \t Soma: %u \t teste_t: %u \t Pre_ex= %g \t soma_dist_adjust= %u \t Tem_Cor= %g\n", 
	MSG_host_get_name (config_mra.workers_mra[owner]), owner,p_worker_cap[owner],soma_temp,task_exec[owner],dist_bruta[owner],soma_dist,config_mra.mra_chunk_count,prev_exec[owner],soma_dist_adjust,temp_corr[owner]);
    }
    fclose (log);

    /*
    * 
    * @brief max_exec_total --> verifica qual é o maior tempo de execução previsto
    * @brief Reduz 1 chunk da distribuição e recalcula Novamente 
    	prev_exe, temp_corr e soma_dist */

        /** 
    * max_prev_exec--> verifica qual é o maior tempo de execução previsto   
    */ 
		max_prev_exec = prev_exec[0];
		    /* @brief max_prev_exec--> verifica qual é o maior tempo de execução previsto */
         adjust = (soma_dist - config_mra.mra_chunk_count);	
       		 		for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
       	      	{ 
  	      				if (max_prev_exec < prev_exec[owner])
  	           			{
      		     				max_prev_exec = prev_exec[owner];
      		     				mra_tid = owner;
                		}
    	     			}
    	/* @brief Reduz 1 chunk da distribuição e recalcula Novamente prev_exe, temp_corr e soma_dist       	 */
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
    	     				prev_exec[owner] = ((dist_bruta[owner]*task_exec[owner])/config_mra.mra_slots[MRA_MAP]);
	     						temp_corr[owner] = prev_exec[owner]+task_exec[owner];
	     						soma_dist = soma_dist + dist_bruta[owner];
	              
	               /* Print Dist_Bruta.log*/
	     						fprintf (log, " %s , ID: %zu \t Dist_Recalc: %u \t Soma: %u \t tasks_map: %u \t Pre_ex= %g \t Tem_Cor= %g\n ",
	     						MSG_host_get_name (config_mra.workers_mra[owner]),owner,dist_bruta[owner],soma_dist,tasks_map, prev_exec[owner],temp_corr[owner]);  
    	    			} 
    	  		fclose (log);

      /**
      * @brief Algoritmo_minMax 
      * Ajuste de Força Bruta com uma Otimização Combinatória para obter uma distribuição de chunks
      * com o menor tempo de execução possível
      *    
      */
      
      min_temp_corr = temp_corr[0];
      max_prev_exec = prev_exec[0]; 
      min_task_exec = task_exec[0];
	    	while ((minimo_task < dist_min) && soma_tot < config_mra.mra_number_of_workers )    
       	{ 
    	/* @brief Reduz 1 chunk do worker com maior prev_exec e adiciona 1 no
    	worker com menor temp_corr e recalcula novamente prev_exe, temp_corr e soma_dist*/
    	
         for (idmax = 0; idmax < config_mra.mra_number_of_workers; idmax++)
    			{ 
  	  			if (max_prev_exec <= prev_exec[idmax])
     	    		{
      					max_prev_exec = prev_exec[idmax];
      					mra_tid = idmax;
            	}
            }
          for (idmin = 0; idmin < config_mra.mra_number_of_workers; idmin++)
    			{
          	if (min_task_exec >= task_exec[idmin])
     	    		{
      					min_task_exec = task_exec[idmin];
            	}  
            
          	if (min_temp_corr >= temp_corr[idmin])
     	    		{
      					min_temp_corr = temp_corr[idmin];
      					id1 = idmin;
            	}          
    	 		}
    	 
    	
        min_max = max_prev_exec - min_temp_corr;
        
        //Troca os chunks de nó
        
        dist_bruta[mra_tid]= dist_bruta[mra_tid]-1;
        dist_bruta[id1]= dist_bruta[id1]+1; 	

    		soma_dist=0;
	
    		log = fopen ("Dist_Fina.log", "w");  
    	/* Recalcula novamente */
    		for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    			{	    	
    				prev_exec[owner] = dist_bruta[owner]*task_exec[owner];
						temp_corr[owner] = prev_exec[owner]+task_exec[owner];
						soma_dist = soma_dist + dist_bruta[owner];

    			fprintf (log, " %s , ID: %zu \t Dist_Recalc: %u \t Soma: %u \t Te_exec = %g \t Pre_ex= %g \t Tem_Cor= %g\n",
		  		MSG_host_get_name (config_mra.workers_mra[owner]),owner,dist_bruta[owner],soma_dist,task_exec[owner],prev_exec[owner],temp_corr[owner]);
    	
    		}

    	fclose (log);
    	dist_min = min_max;
    	minimo_task = min_task_exec;
    	soma_tot++;
    	min_temp_corr = temp_corr[0];
      max_prev_exec = prev_exec[0]; 
      min_task_exec = task_exec[0];
    }
      /* 
        @brief avg_task_exec_map -  Média dos tempos de execuções da tarefas MAP do grupo.             */
        
      log_avg = fopen ("avg_tasks_map.log", "w"); 
      for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
      	{
         avg_task_exec_map = xbt_new0 (double, config_mra.mra_number_of_workers);     
         for (mra_tid=0; mra_tid < config_mra.mra_number_of_workers; mra_tid++)
         	{
         		if (dist_bruta[owner] == dist_bruta[mra_tid] )
         			{
         				avg_t_exec = task_exec[mra_tid] + avg_t_exec;
         				cont_avg++;        
         			}
         	}
         avg_task_exec_map[owner]= (avg_t_exec/cont_avg)*100;
        fprintf (log_avg,"Owner: %zu \t Avg_task_exec(ms): %g \n ", owner, avg_task_exec_map[owner]);
        avg_t_exec=0;
        cont_avg=0; 
      	}
      fclose (log_avg);
      

    /* @brief Calculo do Número de Tarefas Reduce
       Pega-se os tempos de execução das tarefas dos workers
       Calcula-se quantas vezes o tempo mínimo (min_te_exec) é mais rápido que um dado worker.
       A soma destes valores dá a quantidade de tarefas reduce a serem executadas. 
       tot_tasks_reduce é o total de tarefas reduce que serão executadas.
     */
    prev_exec_reduce = xbt_new0 (double, config_mra.mra_number_of_workers);
    min_te_exec= task_exec[0];
    
        for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
    { 
  	if (min_te_exec > task_exec[owner])
     	{
      	min_te_exec = task_exec[owner];
      	// printf("Valor retornado-if %f\n", min_te_exec);
     	}
    }
    
    tasks_reduce = xbt_new0 (int, config_mra.mra_number_of_workers);
       // FIXME  Corrigir Reduce 

    log_avg = fopen ("avg_tasks_reduce.log", "w");
     /* 
        * avg_task_exec_reduce -  Média dos tempos de execuções da tarefas REDUCE do grupo.             */
    for (owner = 0; owner < config_mra.mra_number_of_workers; owner++)
      	{
          avg_task_exec_reduce = xbt_new0 (double, config_mra.mra_number_of_workers);     
          for (mra_tid=0; mra_tid < config_mra.mra_number_of_workers; mra_tid++)
         	 {
         		 if (dist_bruta[owner] == dist_bruta[mra_tid] )
         			{
         				avg_t_exec = task_exec[mra_tid] + avg_t_exec;
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
    			tasks_reduce[owner] = (int) ceil(task_exec[owner]/min_te_exec);
    		}
    	 tot_tasks_reduce = tot_tasks_reduce + tasks_reduce[owner];
    
    	 prev_exec_reduce[owner] = dist_bruta[owner]*task_exec[owner];
    
    	 fprintf (log, " %s , ID: %zu \t Reduces: %u \t Te_exec_Min: %g \t Tarefas_Reduce %u \t Prev_exec_reduce %g \n",
			 MSG_host_get_name (config_mra.workers_mra[owner]),
			 owner,tasks_reduce[owner],min_te_exec,tot_tasks_reduce,prev_exec_reduce[owner]);
     }

    
    
    fclose (log);

	
    /* @brief Criacao da matriz de distribuição conforme a capacidade de cada nó.
      As réplicas são divididas por grupos de dados  
      */
      
      min_dist = 99999;
    for (owner = max_dist = 0; owner < config_mra.mra_number_of_workers; owner++)
    	{
    	 if (max_dist < dist_bruta[owner])
     	    {
      		max_dist = dist_bruta[owner];
      		//printf ("Max_dist: %d \n", max_dist );
            }
            
         if (dist_bruta[owner] <= min_dist && dist_bruta[owner] > 0 )
     	    {
      		min_dist = dist_bruta [owner];
      		//printf ("Min_dist: %d \n", min_dist );
               }        
        }
        
        
        total_dist = xbt_new0 (int, max_dist+1);
            log = fopen ("total_dist.log", "w");
           tot_dist=0;
           while (tot_dist < max_dist + 1 ){
           rep_wid = 1;
           for (owner = 0; owner < config_mra.mra_number_of_workers; owner++){
           if ( tot_dist == dist_bruta [owner] ){
           total_dist [tot_dist] = rep_wid++;
           }
           }
           //printf("Total Dist %u : %u \n ", tot_dist, total_dist [tot_dist]);
           fprintf(log,"Total Dist %u : %u \n ", tot_dist, total_dist [tot_dist]);
           // fprintf (log,"dist: %u \t dist_b:%u \t ID: %zu \t chunk: %zu \n",dist,dist_bruta[owner],owner,chunk);
                      
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
                        fprintf (log,"dist: %u \t dist_b:%u \t ID: %zu \t chunk: %zu \t total_chunk: %u \n",dist,dist_bruta[owner],owner,chunk,total_chunk);
	                   	}
	                 	total_chunk++;
	               	}
	            	dist++;
	          	}	  
    	   		}
                
    	fclose (log); 
      
     
      
      // @brief Replicação dos dados - Código em estudo

         
      log = fopen ("replicas.log", "w");    
         chunk=0;
           for (owner = 0; owner < config_mra.mra_number_of_workers; owner++){    
             while (chunk < config_mra.mra_chunk_count && chunk_owner_mra[chunk][owner]==1 ){
              rpl=0;	                   
               for (i=0; i < config_mra.mra_number_of_workers ; i++){
                                       
                if (dist_bruta[owner] == dist_bruta[i] && chunk_owner_mra[chunk][i] != 1 ) {        
                  if (rpl < config_mra.mra_chunk_replicas -1){
                  chunk_owner_mra[chunk][ i ]=1; 
                  rpl++;
                  }
                  }
                  fprintf (log,"mra_wid: %u \t dist_b:%u \t owner: %zu \t dist_b[owner]: %u \t  chunk: %zu \t rpl:%u \n", i, dist_bruta[i], owner, dist_bruta [owner], chunk, rpl); 
                  if (rpl == config_mra.mra_chunk_replicas -1 && chunk < config_mra.mra_chunk_count -1 ){
                  chunk++;
                  rpl=0;
                  }              
                }
              chunk++;
                }
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
    for (mra_wid = 0; mra_wid < config_mra.mra_number_of_workers; mra_wid++)
    	{
				if (chunk_owner_mra[cid][mra_wid])
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

