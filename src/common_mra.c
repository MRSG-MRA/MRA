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

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY (msg_test);

msg_error_t send (const char* str, double cpu, double net, void* data, const char* mailbox)
{
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
}


msg_error_t send_mra_sms (const char* str, const char* mailbox)
{
    return send (str, 0.0, 0.0, NULL, mailbox);
}


msg_error_t receive (msg_task_t* msg, const char* mailbox)
{
    msg_error_t  status;

    status = MSG_task_receive (msg, mailbox);

#ifdef VERBOSE
    if (status != MSG_OK)
	XBT_INFO ("ERROR %d MRA_RECEIVING MESSAGE", status);
#endif

    return status;
}

int mra_message_is (msg_task_t msg, const char* str)
{
    if (strcmp (MSG_task_get_name (msg), str) == 0)
	return 1;

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
  //XBT_INFO (" MRA_Reduce task %zu sent %zu Bytes", rid,sum); 
    return sum;
}

/* @brief - reads the plataform bandwidth
*  @param plat file name
*/
void read_bandwidth(const char* plat)
{
		FILE * xml = fopen(plat,"r");
	char buff[255];
	char * token;

	const char s[2]= {'"','\0'};
	do{
		 fscanf(xml, "%s", buff);
   	 token = strtok(buff,"=");
   	 if(!strcmp(token,"bandwidth"))
   	 {

		 	token = strtok(NULL,"=");
		 	token = strtok(token,s);
		 	sscanf(token, "%lf", &config_mra.mra_bandwidth);	
 		 	break;
		 }
	}while(fgetc(xml)!=EOF);
	fclose(xml);
//	printf("\n\nBandwidth:%f\n\n",config_mra.mra_bandwidth);
}







