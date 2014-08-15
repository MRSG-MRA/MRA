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
#include "dfs_mra.h"
#include "worker_mra.h"



XBT_LOG_EXTERNAL_DEFAULT_CATEGORY (msg_test);

static void mra_heartbeat (void);
static int listen_mra (int argc, char* argv[]);
static int compute_mra (int argc, char* argv[]);
static void update_mra_map_output (msg_host_t worker, size_t mid);
static void get_mra_chunk (mra_task_info_t ti);
static void get_mra_map_output (mra_task_info_t ti);

size_t get_mra_worker_id (msg_host_t worker)
{
    w_mra_info_t  wi;

    wi = (w_mra_info_t) MSG_host_get_data (worker);
    return wi->mra_wid;
}

/**
 * @brief  Main worker function.
 *
 * This is the initial function of a worker node.
 * It creates other processes and runs a mra_heartbeat loop.
 */
int worker_mra (int argc, char* argv[])
{
    char           mailbox[MAILBOX_ALIAS_SIZE];
    msg_host_t     me;

    me = MSG_host_self ();

    /* Spawn a process that listens for tasks. */
    MSG_process_create ("listen_mra", listen_mra, NULL, me);
    /* Spawn a process to exchange data with other workers. */
    MSG_process_create ("data-node_mra", data_node_mra, NULL, me);
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
    while (!job_mra.finished)
    {
	send_mra_sms (SMS_HEARTBEAT_MRA, MASTER_MRA_MAILBOX);
	MSG_process_sleep (config_mra.mra_heartbeat_interval);
    }
}

/**
 * @brief  Process that listens for tasks.
 */
static int listen_mra (int argc, char* argv[])
{
    char         mailbox[MAILBOX_ALIAS_SIZE];
    msg_error_t  status;
    msg_host_t   me;
    msg_task_t   msg = NULL;

    me = MSG_host_self ();
    sprintf (mailbox, TASKTRACKER_MRA_MAILBOX, get_mra_worker_id (me));

    while (!job_mra.finished)
    {
	msg = NULL;
	status = receive (&msg, mailbox);

	if (status == MSG_OK && message_is (msg, SMS_TASK_MRSG))
	{
	    MSG_process_create ("compute_mra", compute_mra, msg, me);
	}
	else if (message_is (msg, SMS_FINISH_MRA))
	{
	    MSG_task_destroy (msg);
	    break;
	}
    }

    return 0;
}

/**
 * @brief  Process that computes a task.
 */
static int compute_mra (int argc, char* argv[])
{
    msg_error_t  status;
    msg_task_t   mra_task;
    mra_task_info_t  ti;
    xbt_ex_t     e;

    mra_task = (msg_task_t) MSG_process_get_data (MSG_process_self ());
    ti = (mra_task_info_t) MSG_task_get_data (mra_task);
    ti->mra_pid = MSG_process_self_PID ();

    switch (ti->phase)
    {
	case MRA_MAP:
	    get_mra_chunk (ti);
	    break;

	case MRA_REDUCE:
	    get_mra_map_output (ti);
	    break;
    }

    if (job_mra.task_status[ti->phase][ti->mra_tid] != T_STATUS_MRA_DONE)
    {
	TRY
	{
	    status = MSG_task_execute (mra_task);

	    if (ti->phase == MRA_MAP && status == MSG_OK)
		update_mra_map_output (MSG_host_self (), ti->mra_tid);
	}
	CATCH (e)
	{
	    xbt_assert (e.category == cancel_error, "%s", e.msg);
	    xbt_ex_free (e);
	}
    }

    job_mra.mra_heartbeats[ti->mra_wid].slots_av[ti->phase]++;
    
    if (!job_mra.finished)
	send (SMS_TASK_MRSG_DONE, 0.0, 0.0, ti, MASTER_MRA_MAILBOX);

    return 0;
}

/**
 * @brief  Update the amount of data produced by a mapper.
 * @param  worker  The worker that finished a map task.
 * @param  mid     The ID of map task.
 */
static void update_mra_map_output (msg_host_t worker, size_t mid)
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
    msg_error_t  status;
    msg_task_t   data = NULL;
    size_t       my_id;

    my_id = get_mra_worker_id (MSG_host_self ());

    /* Request the chunk to the source node. */
    if (ti->mra_src != my_id)
    {
	sprintf (mailbox, DATANODE_MRA_MAILBOX, ti->mra_src);
	status = send_mra_sms (SMS_GET_MRA_CHUNK, mailbox);
	if (status == MSG_OK)
	{
	    sprintf (mailbox, TASK_MRA_MAILBOX, my_id, MSG_process_self_PID ());
	    status = receive (&data, mailbox);
	    if (status == MSG_OK)
		MSG_task_destroy (data);
	}
    }
}

/**
 * @brief  Copy the itermediary pairs for a reduce task.
 * @param  ti  The task information.
 */
static void get_mra_map_output (mra_task_info_t ti)
{
    char         mailbox[MAILBOX_ALIAS_SIZE];
    msg_error_t  status;
    msg_task_t   data = NULL;
    size_t       total_copied, must_copy;
    size_t       my_id;
    size_t       mra_wid;
    size_t*      data_copied;

    my_id = get_mra_worker_id (MSG_host_self ());
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
							sprintf (mailbox, DATANODE_MRA_MAILBOX, mra_wid);
							status = send (SMS_GET_INTER_MRSG_PAIRS, 0.0, 0.0, ti, mailbox);
							if (status == MSG_OK)
								{
		    					sprintf (mailbox, TASK_MRA_MAILBOX, my_id, MSG_process_self_PID ());
		    					data = NULL;
		    					//TODO Set a timeout: reduce.copy.backoff
		    					status = receive (&data, mailbox);
		    					if (status == MSG_OK)
		    						{
											data_copied[mra_wid] += MSG_task_get_data_size (data);
											total_copied += MSG_task_get_data_size (data);				
											MSG_task_destroy (data);
		    						}
								}
	    			}	
				}	    
	/* (Hadoop 0.20.2) mapred/ReduceTask.java:1979 */
	MSG_process_sleep (5);
    }

 
#ifdef VERBOSE
    XBT_INFO ("INFO: copy finished");
#endif
    ti->shuffle_mra_end = MSG_get_clock ();

    xbt_free_ref (&data_copied);
}

