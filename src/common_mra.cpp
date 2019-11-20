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

#include "common_mra.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY (msg_test);


double*     avg_task_exec_map; 
double*     avg_task_exec_reduce;

struct mra_dist_mang_s mra_dist_manage;
struct mra_config_s config_mra;
struct mra_job_s job_mra;
struct mra_stats_s stats_mra;
struct mra_user_s user_mra;


void send (const char* str, double cpu, double net, void* data, const char* mailbox)
{
    Task_MRA* task = new Task_MRA(std::string(str), cpu, net, data);
    task->setSender(simgrid::s4u::Actor::self());
    task->setSource(simgrid::s4u::Host::current());
    task->setData(data);

    simgrid::s4u::Mailbox *mailbox_ptr = simgrid::s4u::Mailbox::by_name(mailbox);

    mailbox_ptr->put(task, net);
/*OLD
    msg_error_t  status;
    msg_task_t   msg = NULL;

    msg = MSG_task_create (str, cpu, net, data);

#ifdef VERBOSE
    if (!mra_message_is (msg, SMS_HEARTBEAT_MRA))
	    XBT_INFO ("TX (%s): %s", mailbox, str);
#endif

    status = MSG_task_send (msg, mailbox);

#ifdef VERBOSE
    if (status != MSG_OK)
	XBT_INFO ("ERROR %d MRA_SENDING MESSAGE: %s", status, str);
#endif

    return status;
OLD*/
}


void send_mra_sms (const char* str, const char* mailbox)
{
    send (str, 0.0, 0.0, NULL, mailbox);
}


mra_task_t receive (/*OLD msg_task_t* msg,*/ const char* mailbox)
{
    simgrid::s4u::Mailbox *mailbox_ptr = simgrid::s4u::Mailbox::by_name(mailbox);

    mra_task_t task_ptr = (Task_MRA*) mailbox_ptr->get();
    xbt_assert(task_ptr, "mailbox->get() failed");  
    return task_ptr;
    
/*OLD
    msg_error_t  status;

    status = MSG_task_receive (msg, mailbox);

#ifdef VERBOSE
    if (status != MSG_OK)
	XBT_INFO ("ERROR %d MRA_RECEIVING MESSAGE", status);
#endif

    return status;
OLD*/
}


int mra_message_is (mra_task_t msg, const char* str)
{
    /*OLD
    if (strcmp (MSG_task_get_name (msg), str) == 0)
	OLD*/
    std::string aux_str (str);
    
    if(aux_str.compare(msg->getName()) == 0)
        return 1;
    else
        return 0;
}


int mra_maxval (int a, int b)
{
    if (b > a)
	return b;

    return a;
}

/**
 * @brief  Return the output size of a map task.
 * @param  mid  The map task ID.
 * @return The task output size in bytes.
 */
size_t map_mra_output_size (size_t mid)
{
    size_t  rid;
    size_t  sum = 0;
    
    for (rid = 0; rid < config_mra.amount_of_tasks_mra[MRA_REDUCE]; rid++)
    {
    	sum += (user_mra.map_mra_output_f (mid, rid));
	}
	    
    return sum;
}

/**
 * @brief  Return the input size of a reduce task.
 * @param  rid  The reduce task ID.
 * @return The task input size in bytes.
 */
size_t reduce_mra_input_size (size_t rid)
{
    size_t  mid;
    size_t  sum = 0;

    for (mid = 0; mid < config_mra.amount_of_tasks_mra[MRA_MAP]; mid++)
    {
	sum += (user_mra.map_mra_output_f (mid, rid));
    }
  XBT_INFO (" MRA_Reduce task %zu sent %zu Bytes", rid,sum); 
    return sum;
}









