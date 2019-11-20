/* Copyright (c) 2014. BigHybrid Team. All rights reserved. */

/* This file is part of BigHybrid.

BigHybrid, MRSG and MRA++ are free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

BigHybrid, MRSG and MRA++ are distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BigHybrid, MRSG and MRA++.  If not, see <http://www.gnu.org/licenses/>. */

#include	<stdio.h>
#include	<math.h>
#include "common_mra.hpp"
#include "dfs_mra.hpp"
#include "worker_mra.hpp"
#include "mra_cv.hpp"

#include "xbt/log.h"
#include "xbt/asserts.h"



XBT_LOG_EXTERNAL_DEFAULT_CATEGORY (msg_test);

struct mra_work_stat_s mra_work_stat_f;
struct mra_work_stat_s* mra_w_stat_f;

static void mra_heartbeat (void);
static int mra_vc_sleep_f (int my_id, double vc_time_stamp);
static void listen_mra (/*OLD int argc, char* argv[]*/);
static void compute_mra (/*OLD int argc, char* argv[]*/ mra_task_t mra_task);
static void update_mra_map_output (/*OLD msg_host_t*/ simgrid::s4u::Host* worker, size_t mid);
static void get_mra_chunk (mra_task_info_t ti);
static void get_mra_map_output (mra_task_info_t ti);


size_t get_mra_worker_id (/*OLD msg_host_t*/ simgrid::s4u::Host* worker)
{
	const char* wid = worker->get_property("WID");
    size_t value = std::stoi(wid);
    return value;    
	
	/*OLD
    w_mra_info_t  wi;

    wi = (w_mra_info_t) MSG_host_get_data (worker);
    return wi->mra_wid;
	OLD*/
}


/**
 * @brief  Main worker function.
 *
 * This is the initial function of a worker node.
 * It creates other processes and runs a mra_heartbeat loop.
 */
int worker_mra (int argc, char* argv[])
{
    char          mailbox[MAILBOX_ALIAS_SIZE];
    /*OLD msg_host_t    me;*/
	simgrid::s4u::Host* me;
    int       		i=0;
    
    /*OLD me = MSG_host_self ();*/
	me = simgrid::s4u::Host::current();

	mra_w_stat_f = (struct mra_work_stat_s*)xbt_new(struct mra_work_stat_s*, (config_mra.mra_number_of_workers * (sizeof (struct mra_work_stat_s))));

    for (i=0; i < config_mra.mra_number_of_workers; i++ )
	{ 
		mra_w_stat_f[i].mra_work_status = ACTIVE;
	}
    
    /* Spawn a process that listens for tasks. */
    /*OLD MSG_process_create ("listen_mra", listen_mra, NULL, me);*/
	simgrid::s4u::Actor::create("listen_mra", me, listen_mra);
    /* Spawn a process to exchange data with other workers. */
    /*OLD MSG_process_create ("data-node_mra", data_node_mra, NULL, me);*/
	simgrid::s4u::Actor::create("data-node_mra", me, data_node_mra);     
    /* Start sending mra_heartbeat signals to the master node. */
    mra_heartbeat ();
    sprintf (mailbox, DATANODE_MRA_MAILBOX, get_mra_worker_id (me));
    send_mra_sms (SMS_FINISH_MRA, mailbox);
    sprintf (mailbox, TASKTRACKER_MRA_MAILBOX, get_mra_worker_id (me));
    send_mra_sms (SMS_FINISH_MRA, mailbox);
    
    return 0;
}

/**
 * @brief  The mra_heartbeat loop.
 */
static void mra_heartbeat (void)
{   
   	double vc_time_sleep;
    size_t       my_id;
  
    /*OLD my_id = get_mra_worker_id (MSG_host_self ());*/
	my_id = get_mra_worker_id(simgrid::s4u::Host::current());
    
    
    //XBT_INFO ("Work_ID %zd \n", my_id);																			
    while (!job_mra.finished)
    {	
     	/*OLD mra_vc_sleep_f (my_id, MSG_get_clock ());*/
		mra_vc_sleep_f (my_id, simgrid::s4u::Engine::get_clock());
			vc_time_sleep = vc_traces_time;
      
		/*Sends a SMS, if machine is active in initial time.*/
		if (mra_w_stat_f[my_id].mra_work_status == ACTIVE)
		{
			send_mra_sms (SMS_HEARTBEAT_MRA, MASTER_MRA_MAILBOX);
		}
		/*OLD MSG_process_sleep (vc_time_sleep);*/
		simgrid::s4u::this_actor::sleep_for(vc_time_sleep);					
    } 

/*  loop
static void mra_heartbeat (void)
{
    while (!job_mra.finished)
    {
	send_mra_sms (SMS_HEARTBEAT_MRA, MASTER_MRA_MAILBOX);
	MSG_process_sleep (config_mra.mra_heartbeat_interval);
    }
}*/  
}


/**
 * @brief  Volatility function from the traces file..
 */
static int mra_vc_sleep_f (int my_id, double vc_time_stamp)
{   
   int j=0; 
   int i=0;
   int hosts_vc_traces;
   int mra_vc_job_hosts;
              
    /* Number of hosts from trace archive*/
    hosts_vc_traces = vc_node[config_mra_vc_file_line[0]][0];         
    /*Number of volatile hosts defined by user. The result is saved on element array config_mra_vc_file_line[1]*/
    mra_vc_job_hosts = (int)(ceil(config_mra.mra_number_of_workers * (double)config_mra.perc_vc_node/100)) ;
    config_mra_vc_file_line[1] = mra_vc_job_hosts;
    if ( my_id > hosts_vc_traces && mra_vc_job_hosts > hosts_vc_traces ){
    XBT_INFO ("Atention - Number of volatile host %d, in log file is insufficient to simulation \n", hosts_vc_traces);
    return 0;
   
    } 
    
    if ( my_id < config_mra_vc_file_line[1]) 
    		{
        	for (i=0; i < config_mra_vc_file_line[0] ; i++)
        	    	{  
        	    	  if ((my_id + 1 == vc_node[i][0]) && my_id < mra_vc_job_hosts ) 
        	    	  {
        	    	  	j=i;
                  	while ((j < config_mra_vc_file_line[0] )) 
                  	{
                      if (vc_time_stamp >= vc_start[j][0] && vc_time_stamp < vc_end[j][0])
                    		{
                      		if (vc_type[j][0] == 1)
                      			{
                        			vc_traces_time = config_mra.mra_heartbeat_interval;
                        		
                        			/*OLD job_mra.mra_heartbeats[my_id].wid_timestamp = MSG_get_clock ();*/
									job_mra.mra_heartbeats[my_id].wid_timestamp =  simgrid::s4u::Engine::get_clock();
                        			if (mra_w_stat_f[my_id].mra_work_status == INACTIVE) {
                        	// XBT_INFO (" Volat_node %zd ON - Traces_time %Lg, Hearbeat %Lg \n",my_id,vc_traces_time, job_mra.mra_heartbeats[my_id].wid_timestamp);       
                        	    }
		  												mra_w_stat_f[my_id].mra_work_status = ACTIVE;			
                        			return vc_traces_time;
                        			break;
                      			}
                      		else
                      			{
                      			  vc_traces_time = (vc_end[j][0] - vc_time_stamp);
                       				if ((config_mra.mra_heartbeat_interval < vc_traces_time) && mra_w_stat_f[my_id].mra_work_status != INACTIVE) 
                       				{
                       						/*OLD job_mra.mra_heartbeats[my_id].wid_timestamp = MSG_get_clock ();*/
											job_mra.mra_heartbeats[my_id].wid_timestamp =  simgrid::s4u::Engine::get_clock();
                                	}         
                       			   // XBT_INFO (" Volat_node %zd OFF - Traces_time %Lg, Hearbeat %Lg, EOL %f  \n", my_id,vc_traces_time, job_mra.mra_heartbeats[my_id].wid_timestamp,(double)vc_end[j][0]);
                       			    //XBT_INFO (" Volat_node %zd OFF - Traces_time %Lg, Hearbeat %Lg  \n", my_id,vc_traces_time, job_mra.mra_heartbeats[my_id].wid_timestamp);
									mra_w_stat_f[my_id].mra_work_status = INACTIVE;
                       				return vc_traces_time;
                       				break;
                      			}
                    		}
	                     j++;
                     } 
                   }
    			      }
    		}
    else  
      	{
       	 	vc_traces_time = config_mra.mra_heartbeat_interval;
       	 	/*OLD job_mra.mra_heartbeats[my_id].wid_timestamp = MSG_get_clock ();*/
			job_mra.mra_heartbeats[my_id].wid_timestamp = simgrid::s4u::Engine::get_clock();
       		//XBT_INFO (" Host %zd ON - Traces_time %g, Hearbeat %Lg \n", my_id, vc_traces_time, job_mra.mra_heartbeats[my_id].wid_timestamp);
    		}
    return vc_traces_time;
}


/**
 * @brief  Process that listens for tasks.
 */
/* OLD int*/
static void listen_mra (/* OLD int argc, char* argv[] */)
{
    char         mailbox[MAILBOX_ALIAS_SIZE];
    /*OLD msg_error_t  status; */
	/*OLD msg_task_t   msg = NULL;*/
    /*OLD msg_host_t   me;*/
	simgrid::s4u::Host* me;    
	mra_task_t msg;

    /*OLD me = MSG_host_self ();*/
	me = simgrid::s4u::Host::current();
    sprintf (mailbox, TASKTRACKER_MRA_MAILBOX, get_mra_worker_id (me));

    while (!job_mra.finished)
    {
		/*OLD
		msg = NULL;
		status = receive (&msg, mailbox);
		OLD*/
		msg = receive(mailbox);

		/*OLD if (status == MSG_OK && mra_message_is (msg, SMS_TASK_MRA))*/
		if (msg && mra_message_is (msg, SMS_TASK_MRA))
		{
			/*OLD MSG_process_create ("compute_mra", compute_mra, msg, me);*/
			simgrid::s4u::Actor::create("compute_mra", me, compute_mra, msg);
		}
		else if (mra_message_is (msg, SMS_FINISH_MRA))
		{
			/*OLD MSG_task_destroy (msg);*/
			msg->destroy();
			break;
		}
    }

    /* OLD return 0; */
}

/**
 * @brief  Process that computes a task.
 */
/*OLD int*/
static void compute_mra (/*int argc, char* argv[]*/ mra_task_t mra_task)
{
    /*OLD msg_error_t  status;*/
    /*OLD msg_task_t   mra_task;*/
    /*OLD xbt_ex_t     e;*/
	mra_task_info_t  ti;
    
	/*OLD
    mra_task = (msg_task_t) MSG_process_get_data (MSG_process_self ());
    ti = (mra_task_info_t) MSG_task_get_data (mra_task);
	ti->mra_pid = MSG_process_self_PID ();
	OLD*/
	ti = (mra_task_info_t) mra_task->getData();
	ti->mra_pid = simgrid::s4u::this_actor::get_pid();

    switch (ti->mra_phase)
    {
	case MRA_MAP:
	    get_mra_chunk (ti);
	    mra_task_ftm[ti->mra_tid].mra_ft_pid[MRA_MAP] = ti->mra_pid;
	    //XBT_INFO ("INFO:Processo Map %d - Tarefa %zd", mra_task_ftm[ti->mra_tid].mra_ft_pid[MRA_MAP], ti->mra_tid );
	    break;

	case MRA_REDUCE:
	    get_mra_map_output (ti);
	    mra_task_ftm[ti->mra_tid].mra_ft_pid[MRA_REDUCE] = ti->mra_pid;
	    //XBT_INFO ("INFO:Processo Reduce %d - Tarefa %zd", mra_task_ftm[ti->mra_tid].mra_ft_pid[MRA_REDUCE],ti->mra_tid );
	    break;
    }

    if (job_mra.task_status[ti->mra_phase][ti->mra_tid] != T_STATUS_MRA_DONE 
	  /*&& mra_task_ftm[ti->mra_tid].mra_ft_wid == ti->mra_wid)*/ )
    {
		/*OLD status = MSG_task_execute (mra_task);*/
		mra_task->execute();

	    if (ti->mra_phase == MRA_MAP /*OLD && status == MSG_OK*/)
			update_mra_map_output (simgrid::s4u::Host::current(), ti->mra_tid);
			/*OLD update_mra_map_output (MSG_host_self (), ti->mra_tid);*/

	/*OLD
 	TRY
 	{
	    status = MSG_task_execute (mra_task);

	    if (ti->mra_phase == MRA_MAP && status == MSG_OK)
		update_mra_map_output (MSG_host_self (), ti->mra_tid);

  	}
	CATCH (e)
 	{
   	    xbt_assert (e.category == cancel_error, "%s", e.msg);
 	    xbt_ex_free (e);
 	}
	OLD*/
    }

    job_mra.mra_heartbeats[ti->mra_wid].slots_av[ti->mra_phase]++;
    
    if (!job_mra.finished)
		send (SMS_TASK_MRA_DONE, 0.0, 0.0, ti, MASTER_MRA_MAILBOX);

    /*OLD return 0;*/
}

/**
 * @brief  Update the amount of data produced by a mapper.
 * @param  worker  The worker that finished a map task.
 * @param  mid     The ID of map task.
 */
static void update_mra_map_output (/*OLD msg_host_t*/simgrid::s4u::Host* worker, size_t mid)
{
    size_t  rid;
    size_t  mra_wid;

    mra_wid = get_mra_worker_id (worker);

    for (rid = 0; rid < config_mra.amount_of_tasks_mra[MRA_REDUCE]; rid++)
	job_mra.map_output[mra_wid][rid] += user_mra.map_mra_output_f (mid, rid);
}

/**
 * @brief  Get the chunk associated to a map task.
 * @param  ti  The task information.
 */
static void get_mra_chunk (mra_task_info_t ti)
{
    char         mailbox[MAILBOX_ALIAS_SIZE];
    /*OLD msg_error_t  status;*/
    /*OLD msg_task_t   data = NULL;*/
    size_t       my_id;
	mra_task_t data;

    /*OLD my_id = get_mra_worker_id (MSG_host_self ());*/
	my_id = get_mra_worker_id(simgrid::s4u::Host::current());

    /* Request the chunk to the source node. */
    if (ti->mra_src != my_id)
    {
		sprintf (mailbox, DATANODE_MRA_MAILBOX, ti->mra_src);
		/*OLD status = send_mra_sms (SMS_GET_MRA_CHUNK, mailbox);*/
		send_mra_sms (SMS_GET_MRA_CHUNK, mailbox);
		/*OLD
		if (status == MSG_OK)
		{
			sprintf (mailbox, TASK_MRA_MAILBOX, my_id, MSG_process_self_PID ());
			status = receive (&data, mailbox);
			if (status == MSG_OK)
				MSG_task_destroy (data);
		}
		OLD*/

		sprintf (mailbox, TASK_MRA_MAILBOX, my_id, simgrid::s4u::this_actor::get_pid());
	    data = receive(mailbox);

		if(data)
			data->destroy();
	}
}

/**
 * @brief  Copy the itermediary pairs for a reduce task.
 * @param  ti  The task information.
 */
static void get_mra_map_output (mra_task_info_t ti)
{
    char         mailbox[MAILBOX_ALIAS_SIZE];
    /*OLD msg_error_t  status;*/
    /*OLD msg_task_t   data = NULL;*/
    size_t       total_copied, must_copy;
    size_t       my_id;
    int			 mra_wid;
    size_t*      data_copied;
	mra_task_t data;

    /*OLD my_id = get_mra_worker_id (MSG_host_self ());*/
	my_id = get_mra_worker_id(simgrid::s4u::Host::current());
    data_copied = xbt_new0 (size_t, config_mra.mra_number_of_workers);
    ti->map_output_copied = data_copied;
    total_copied = 0;
    must_copy = reduce_mra_input_size (ti->mra_tid);
    
    
#ifdef VERBOSE
    XBT_INFO ("INFO: start copy");
#endif    
    while (total_copied < must_copy)
    {
		for (mra_wid = 0; mra_wid < config_mra.mra_number_of_workers; mra_wid++)
		{
	 		if (job_mra.task_status[MRA_REDUCE][ti->mra_tid] == T_STATUS_MRA_DONE)
	    	{
				xbt_free_ref (&data_copied);
				return;
	    	}

	    	if (job_mra.map_output[mra_wid][ti->mra_tid] > data_copied[mra_wid])
	    	{
				sprintf (mailbox, DATANODE_MRA_MAILBOX, (size_t) mra_wid);
				send(SMS_GET_INTER_MRA_PAIRS, 0.0, 0.0, ti, mailbox);
				/*OLD
				status = send (SMS_GET_INTER_MRA_PAIRS, 0.0, 0.0, ti, mailbox);
				if (status == MSG_OK)
				{
					sprintf (mailbox, TASK_MRA_MAILBOX, my_id, MSG_process_self_PID ());
					data = NULL;

					status = receive (&data, mailbox);
		    		if (status == MSG_OK)
		    		{
						data_copied[mra_wid] += MSG_task_get_bytes_amount (data);
						total_copied += MSG_task_get_bytes_amount (data);				
						MSG_task_destroy (data);
		    		}
				}
				OLD*/
				
		    	sprintf (mailbox, TASK_MRA_MAILBOX, my_id, simgrid::s4u::this_actor::get_pid());
		    	//TODO Set a timeout: reduce.copy.backoff
				data = receive(mailbox);  					
		    	
				if (data)
		    	{
					data_copied[mra_wid] += data->getBytesAmount();
					total_copied += data->getBytesAmount();				
					data->destroy();
		    	}
			}
	    }		    
		/* (Hadoop 0.20.2) mapred/ReduceTask.java:1979 */
		/*OLD MSG_process_sleep (5);*/
		simgrid::s4u::this_actor::sleep_for(5);

    }
 
#ifdef VERBOSE
    XBT_INFO ("INFO: copy finished");
#endif
    /*OLD ti->shuffle_mra_end = MSG_get_clock ();*/
	ti->shuffle_mra_end = simgrid::s4u::Engine::get_clock();

    xbt_free_ref (&data_copied);
}

