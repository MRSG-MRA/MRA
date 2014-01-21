#include <math.h>
#include <msg/msg.h>
#include "common.h"
#include "dfs.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY (msg_test);

static int send_data (int argc, char* argv[]);
static int get_slot_port (m_task_t data_request);

void distribute_data (void)
{
    int      i;
    FILE*    log;
    int      counter;
    double*  p_worker_cap = NULL;
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
    int      id = 0;
    int      id1 = 0;
    int      soma_tot=0;
    double   min_max= 1;
    double   dist_min=1;
    double   minimo_task=0;
    double   min_task_exec;
    int	     max_dist;
    int      min_dist;
    int      dist,total_chunk;
    int      rep_wid;
    int      cont_avg=0;
    int      tot_dist=0;
    double   avg_t_exec=0;
   

    /* Allocate memory for the mapping matrix. */
    chunk_owner = xbt_new (char*, config.chunk_count);

    for (chunk = 0; chunk < config.chunk_count; chunk++)
    {
	chunk_owner[chunk] = xbt_new0 (char, config.number_of_workers);
    }

    /* START DISTRIBUTION - Matrix chunk_owner (chunk,worker)*/

    /**
      * @lista de workers --> workers_hosts[id] (array)
      * @pegar capacidade --> MSG_get_host_speed (worker_hosts[owner]) 
      * @p_worker_cap --> Calcula a capacidade computacional relativa de cada worker
      * baseado na capacidade total da grid.
      * @dist_bruta --> É o array com as tribuições brutas, antes do ajuste de
      * menor te_exec
      * @prev_exec  --> É o array com o valor de previsão de término de todas as tarefas distribuídas ao
      * worker;
      * @temp_corr --> É o array com o tempo que será utilizado para encontar a melhor distribuição
      * @task_exec --> É o array que contém o tempo de cada worker para executar uma tarefa computacional padrão
        
    */

    p_worker_cap = xbt_new0 (double, config.number_of_workers);

    dist_bruta = xbt_new0 (int, config.number_of_workers); 
    
    prev_exec = xbt_new0 (double, config.number_of_workers);
    
    temp_corr = xbt_new0 (double, config.number_of_workers);
    
    task_exec = xbt_new0 (double, config.number_of_workers);
    
       
   
    log = fopen ("worker_cap.log", "w");	
    for (owner = 0; owner < config.number_of_workers; owner++)              
    {
	p_worker_cap[owner] = MSG_get_host_speed(worker_hosts[owner])/config.grid_cpu_power;
	
	tasks_map = config.chunk_count/config.map_slots;
	
	task_exec[owner] = config.cpu_required_map/MSG_get_host_speed(worker_hosts[owner]);
	
	dist_bruta[owner] = (int) ceil(p_worker_cap[owner]*tasks_map);
	
	prev_exec[owner] = dist_bruta[owner]*task_exec[owner];
	
	temp_corr[owner] = prev_exec[owner]+task_exec[owner];
	
	soma_dist = soma_dist + dist_bruta[owner];
	
	soma_temp= task_exec[owner]+soma_temp;
	

	fprintf (log, " %s , ID: %zu \t Cap: %f \t Soma=%g \t te_exec= %g \t Dist_B: %u \t Soma: %u \t Pre_ex= %g \t Tem_Cor= %g\n",
		MSG_host_get_name (worker_hosts[owner]),
		owner,p_worker_cap[owner],soma_temp,task_exec[owner],dist_bruta[owner],soma_dist,prev_exec[owner],temp_corr[owner]);
    }
    fclose (log);

    /** 
    * @max_exec_total --> verifica qual é o maior tempo de execução previsto
        
    */
    
    max_prev_exec = prev_exec[0];
     while (tasks_map != soma_tot)    
     { 	
        for (owner = 0; owner < config.number_of_workers; owner++)
    	{ 
  	  if (max_prev_exec < prev_exec[owner])
     	    {
      		max_prev_exec = prev_exec[owner];
      		id = owner;
            }
    	}
    	/* Reduz 1 chunk da distribuição e recalcula Novamente 
    	prev_exe, temp_corr e soma_dist
    	
    	 */
        owner = id;
        dist_bruta[owner]= dist_bruta[owner]-1;
     	//printf("Maior Valor Prev_Exec %f \t ID:%u \t Dist_B: %u \n ", max_prev_exec,id,dist_bruta[owner] );   	
    	//log = fopen ("Dist_Bruta.log", "w");
    	id=0;   
    	max_prev_exec = prev_exec[0]; 	
    	soma_dist=0; 
    	for (owner = 0; owner < config.number_of_workers; owner++)
    	{	    	
    	//Recalcula novamente
    	prev_exec[owner] = dist_bruta[owner]*task_exec[owner];
	temp_corr[owner] = prev_exec[owner]+task_exec[owner];
	soma_dist = soma_dist + dist_bruta[owner];
	
	
    /*	fprintf (log, " %s , ID: %zu \t Dist_Recalc: %u \t Soma: %u \t Pre_ex= %g \t Tem_Cor= %g\n",
		MSG_host_get_name (worker_hosts[owner]),
		owner,dist_bruta[owner],soma_dist,prev_exec[owner],temp_corr[owner]);*/
    	}
    //	fclose (log);
    	soma_tot = soma_dist;
    	//printf("Soma total: %u \n ", soma_tot); 
      }
      
      /**
      * @Algoritmo_minMax 
      * Ajuste de Força Bruta com uma Otimização Combinatória para obter uma distribuição de chunks
      * com o menor tempo de execução possível
      *    
      */
      
      min_temp_corr = temp_corr[0];
      max_prev_exec = prev_exec[0]; 
      min_task_exec = task_exec[0];     
     while (minimo_task < dist_min )    
       {
        for (owner = 0; owner < config.number_of_workers; owner++)
    	{ 
  	  if (max_prev_exec < prev_exec[owner])
     	    {
      		max_prev_exec = prev_exec[owner];
      		id = owner;
            }
            
          if (min_task_exec > task_exec[owner])
     	    {
      		min_task_exec = task_exec[owner];
            }  
            
          if (min_temp_corr > temp_corr[owner])
     	    {
      		min_temp_corr = temp_corr[owner];
      		id1 = owner;
            }          
    	 }
    	/* Reduz 1 chunk do worker com maior prev_exec e adiciona 1 no
    	worker com menor temp_corr e recalcula novamente prev_exe, temp_corr e soma_dist
    	
    	 */
    	//printf("Min_Task_exec: %g \n ", min_task_exec);
    	//printf("Max_Prev_exec: %g \t ID: %u \n ", max_prev_exec,id);
    	//printf("Min_Temp_Corr: %g \t ID1: %u \n ", min_temp_corr,id1);  	
    	//printf("Min_Max0: %g \n ", min_max);
        min_max = max_prev_exec - min_temp_corr;
   //    printf("Min_Max1: %g \n ", min_max);

        //Troca os chunks de nó
        
        owner=id;
        dist_bruta[owner]= dist_bruta[id]-1;
        owner=id1;
        dist_bruta[owner]= dist_bruta[id1]+1;
     //	printf("Troca Minimo ID1:%u \t Troca Máximo ID:%u \n ",id1,id );
     	id=0;
    	id1=0;   
    	soma_dist=0;
    //	printf("Min_Max total: %g \n ", min_max); 
       	max_prev_exec = prev_exec[0]; 
    	min_temp_corr = temp_corr[0]; 	
    	log = fopen ("Dist_Fina.log", "w");	
    	for (owner = 0; owner < config.number_of_workers; owner++)
    	{	    	
    	//Recalcula novamente
    	prev_exec[owner] = dist_bruta[owner]*task_exec[owner];
	temp_corr[owner] = prev_exec[owner]+task_exec[owner];
	soma_dist = soma_dist + dist_bruta[owner]*config.map_slots;
	
	if (max_prev_exec < prev_exec[owner])
     	    {
      		max_prev_exec = prev_exec[owner];
            }
            
          if (min_task_exec > task_exec[owner])
     	    {
      		min_task_exec = task_exec[owner];
            }  
            
          if (min_temp_corr > temp_corr[owner])
     	    {
      		min_temp_corr = temp_corr[owner];
            }          
    	fprintf (log, " %s , ID: %zu \t Dist_Recalc: %u \t Soma: %u \t Te_exec = %g \t Pre_ex= %g \t Tem_Cor= %g\n",
		MSG_host_get_name (worker_hosts[owner]),
		owner,dist_bruta[owner],soma_dist,task_exec[owner],prev_exec[owner],temp_corr[owner]);
    	
    	}
    	fclose (log);
    	soma_tot = soma_dist;
    	dist_min = min_max;
    	minimo_task = min_task_exec;
     /*	printf("Soma total: %u \n ", soma_tot); 
     	printf("Min_Max2: %g \n ", dist_min);  	
      */  
      }
      /* 
        * @avg_task_exec -  Média dos tempos de execuções do grupo.    
      

         */
      for (owner = 0; owner < config.number_of_workers; owner++){
         avg_task_exec = xbt_new0 (double, config.number_of_workers);
         
         
         for (id=0; id < config.number_of_workers; id++){
         if (dist_bruta[owner] == dist_bruta[id] ){
         avg_t_exec = task_exec[id] + avg_t_exec;
         cont_avg++;        
         }
         }
         avg_task_exec[owner]= avg_t_exec/cont_avg;
       // printf("Owner: %zu \t Avg_task_exec: %g \n ", owner, avg_task_exec[owner]);
        avg_t_exec=0;
        cont_avg=0; 
      }
      
      

    /* Calculo do Número de Tarefas Reduce
       Pega-se os tempos de execução das tarefas dos workers
       Calcula-se quantas vezes o tempo mínimo (min_te_exec) é mais rápido que um dado worker.
       A soma destes valores dá a quantidade de tarefas reduce a serem executadas. 
       tot_tasks_reduce é o total de tarefas reduce que serão executadas.
     */
    prev_exec_reduce = xbt_new0 (double, config.number_of_workers);
    min_te_exec= task_exec[0];
    
        for (owner = 0; owner < config.number_of_workers; owner++)
    { 
  	if (min_te_exec > task_exec[owner])
     	{
      	min_te_exec = task_exec[owner];
      	// printf("Valor retornado-if %f\n", min_te_exec);
     	}
    }
    
    tasks_reduce = xbt_new0 (int, config.number_of_workers);
       
    log = fopen ("tasks_reduce.log", "w");
    for (owner = 0; owner < config.number_of_workers; owner++)    
    {
    
    if (dist_bruta[owner]>0){   
    tasks_reduce[owner] = (int) ceil(task_exec[owner]/min_te_exec);
    }
    tot_tasks_reduce = tot_tasks_reduce + tasks_reduce[owner];
    
    prev_exec_reduce[owner] = dist_bruta[owner]*task_exec[owner];
        
    fprintf (log, " %s , ID: %zu \t Reduces: %u \t Te_exec_Min: %g \t Tarefas_Reduce %u \t Prev_exec_reduce %g \n",
		MSG_host_get_name (worker_hosts[owner]),
		owner,tasks_reduce[owner],min_te_exec,tot_tasks_reduce,prev_exec_reduce[owner]);
    }
    //config.number_of_reduces = 2 * config.number_of_workers;
    //config.number_of_reduces = 3 * config.number_of_workers;
    //config.number_of_reduces = 4 * config.number_of_workers;
    //config.number_of_reduces = 5 * config.number_of_workers;
    config.number_of_reduces = 6 * config.number_of_workers;
    //config.number_of_reduces = tot_tasks_reduce - (tot_tasks_reduce % config.number_of_workers);
    
    
    fclose (log);

    //TODO alterar posicao da chamada de distribuicao de dados.
    config.cpu_required_reduce *= (config.map_out_size / config.number_of_reduces);
    
    job.tasks_pending[REDUCE] = config.number_of_reduces;
    
    job.task_state[REDUCE] = xbt_new0 (int, config.number_of_reduces);
    
    job.task_has_spec_copy[REDUCE] = xbt_new0 (int, config.number_of_reduces);

    job.task_list[REDUCE] = xbt_new0 (m_task_t*, MAX_SPECULATIVE_COPIES);
    for (i = 0; i < MAX_SPECULATIVE_COPIES; i++) {
	job.task_list[REDUCE][i] = xbt_new0 (m_task_t, config.number_of_reduces);
        }




    /*Criacao da matriz de distribuição conforme a capacidade de cada nó.
      As réplicas são divididas por grupos de dados  
      */
      
      min_dist = 9;
    for (owner = max_dist = 0; owner < config.number_of_workers; owner++)
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
           for (owner = 0; owner < config.number_of_workers; owner++){
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
       for (owner = 0; owner < config.number_of_workers; owner++)
    	{
    	 dist=0;
    	 total_chunk=0;
    	 while (dist < dist_bruta[owner]){
    	   while ( total_chunk < (dist_bruta[owner]*config.map_slots)){
    	      if (dist_bruta[owner]>0){
    	      chunk_owner[chunk][owner] = 1;
    	      total_chunk++;
             fprintf (log,"dist: %u \t dist_b:%u \t ID: %zu \t chunk: %zu \n",dist,dist_bruta[owner],owner,chunk);
	    }
	    chunk++;
	    }
	    dist++;
	   }	  
    	} 
    	fclose (log); 
      
     
      
      // Replicação dos dados - Código em estudo
 // Replicação dos dados - Código em estudo
         
      log = fopen ("replicas.log", "w");    
         chunk=0;
           for (owner = 0; owner < config.number_of_workers; owner++){    
             while (chunk < config.chunk_count && chunk_owner[chunk][owner]==1 ){
              rpl=0;	                   
               for (i=0; i < config.number_of_workers ; i++){
                                       
                if (dist_bruta[owner] == dist_bruta[i] && chunk_owner[chunk][i] != 1 ) {        
                  if (rpl < config.chunk_replicas -1){
                  chunk_owner[chunk][ i ]=1; 
                  rpl++;
                  }
                  }
                  fprintf (log,"wid: %u \t dist_b:%u \t owner: %zu \t dist_b[owner]: %u \t  chunk: %zu \t rpl:%u \n", i, dist_bruta[i], owner, dist_bruta [owner], chunk, rpl); 
                  if (rpl == config.chunk_replicas -1 && chunk < config.chunk_count -1 ){
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
    for (owner = 0; owner < config.number_of_workers; owner++)
    {
	fprintf (log, "worker %06zu | ", owner);
	counter = 0;
	for (chunk = 0; chunk < config.chunk_count; chunk++)
	{
	    fprintf (log, "%d", chunk_owner[chunk][owner]);
	    if (chunk_owner[chunk][owner])
		counter++;
	}
	fprintf (log, " | chunks owned: %d\n", counter);
    }
    fclose (log);
}

size_t find_random_chunk_owner (int cid)
{
    int     replica;
    size_t  owner = NONE;
    size_t  wid;

    replica = rand () % config.chunk_replicas;

    for (wid = 0; wid < config.number_of_workers; wid++)
    {
	if (chunk_owner[cid][wid])
	{
	    owner = wid;

	    if (replica == 0)
		break;
	    else
		replica--;
	}
    }

    xbt_assert (owner != NONE, "Aborted: chunk %d is missing.", cid);

    return owner;
}

int data_node (int argc, char* argv[])
{
    m_task_t  msg = NULL;

    while (!job.finished)
    {
	msg = receive (PORT_DATA_REQ);
	MSG_process_create ("send-data", send_data, msg, MSG_host_self ());
    }

    return 0;
}

/**
 * @brief  Process that responds to data requests.
 */
static int send_data (int argc, char* argv[])
{
    double    data_size;
    int       answer_port;
    m_host_t  dest = NULL;
    m_task_t  msg = NULL;
    size_t    maps_requested;

    msg = MSG_process_get_data (MSG_process_self ());
    dest = MSG_task_get_source (msg);
    answer_port = get_slot_port (msg);

    if (message_is (msg, SMS_GET_CHUNK))
    {
	send ("DATA-C", 0.0, config.chunk_size, NULL, dest, answer_port);
    }
    else if (message_is (msg, SMS_GET_INTER_PAIRS))
    {
	maps_requested = (size_t) MSG_task_get_data (msg);
	data_size = (config.map_out_size / config.number_of_maps / config.number_of_reduces) * maps_requested;
	send ("DATA-IP", 0.0, data_size, NULL, dest, answer_port);
    }

    MSG_task_destroy (msg);
    return 0;
}

/**
 * @brief  Get the port to respond to a data request.
 * @param  data_request  The data request message/task.
 * @return The number of the destination port.
 */
static int get_slot_port (m_task_t data_request)
{
    m_process_t  sender;
    m_task_t     task;
    task_info_t  ti;

    sender = MSG_task_get_sender (data_request);
    task = (m_task_t) MSG_process_get_data (sender);
    ti = (task_info_t) MSG_task_get_data (task);

    return ti->slot_port;
}

