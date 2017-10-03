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
#include <string.h>
#include "xbt/sysdep.h"
#include "common_mra.h"
#include "worker_mra.h"
#include "dfs_mra.h"
#include "mra_cv.h"



XBT_LOG_EXTERNAL_DEFAULT_CATEGORY (msg_test);

static FILE*       tasks_log;
static FILE*    	 failure_log;
//static FILE*    	 dist_b_log;
static void print_mra_config (void);
static void print_mra_stats (void);
static int is_straggler_mra (enum mra_phase_e mra_phase, msg_host_t worker);

static int task_time_elapsed_mra (msg_task_t mra_task);
static void set_mra_speculative_tasks (enum mra_phase_e mra_phase, msg_host_t worker);

static void send_map_to_mra_worker (msg_host_t dest);
static void send_reduce_to_mra_worker (msg_host_t dest);
static void send_mra_task (enum mra_phase_e mra_phase, size_t tid, size_t data_src, msg_host_t dest);
static void finish_all_mra_task_copies (mra_task_info_t ti);

static void get_vc_behavior (double control_timestamp, size_t mra_wid, double last_hb_wid_timestamp);
static void mra_status_tfm (enum mra_phase_e mra_phase, size_t ft_task_id, size_t ft_mra_wid, enum mra_task_status_e  mra_ft_task_status );
static void mra_tfm_recover (size_t mra_ftm_vc_wid, int total_tasks);

void mra_tfm_new_task (int chunk); // @vinicius- new replication for lost chunks

/** @brief  Main master function. */
int master_mra (int argc, char* argv[])
{
    mra_heartbeat_t  mra_heartbeat;
    msg_error_t  	status;
    msg_host_t   	worker;
    msg_task_t   	msg = NULL;
    size_t       	mra_wid;
    //int t_wait = config_mra.mra_heartbeat_interval; 
    mra_task_info_t  ti;
    int 					i;
    //int  time_ctl=0;
    double*       mra_vc_lhb_fdt;
    // Begin - Variable to detector test
    int 					k=0;
    size_t				mra_vc_wid;
    //End - Variable to detector test


    mra_vc_last_hb= xbt_new0(double, (config_mra.mra_number_of_workers * sizeof (double)));
    mra_vc_fail_timeout_period = xbt_new0(double, (config_mra.mra_number_of_workers * sizeof (double)));
    mra_vc_state_failure = xbt_new0(int, (config_mra.mra_number_of_workers * sizeof (int)));
    vc_state_working = xbt_new0(int, (config_mra.mra_number_of_workers * sizeof (int)));
    mra_vc_lhb_fdt= xbt_new0(double, (config_mra.mra_number_of_workers * sizeof (double)));
    worker_reduce_tasks = xbt_new0 (int, (config_mra.mra_number_of_workers * sizeof (int)));

    for (i=0; i < config_mra.mra_number_of_workers; i++ )
    {
        mra_vc_last_hb[i] = 0;
        mra_vc_state_failure[i] = 0;
        vc_state_working[i] = 0;
        mra_vc_fail_timeout_period[i] = 0;
        mra_vc_lhb_fdt[i] = 0;
        worker_reduce_tasks[i]=0;

    }

    stats_mra.mra_reduce_recovery	=	0;
    stats_mra.mra_map_recovery		= 0;

    failure_log = fopen ("failure-mra.csv", "w");
    print_mra_config ();
    XBT_INFO ("JOB_MRA BEGIN");
    XBT_INFO (" ");

    tasks_log = fopen ("tasks-mra.csv", "w");
    fprintf (tasks_log, "mra_phase,task_id,worker_id,time,action,shuffle_mra_end\n");

    /** @brief Allocate memory for the behavior. */
    behavior = (enum mra_vc_status_e*)xbt_new(enum mra_vc_status_e*, (config_mra.mra_number_of_workers * (sizeof (enum mra_vc_status_e))));

    /** @brief Status initialization for behavior */

    for (i=0; i < config_mra.mra_number_of_workers; i++ )
    {
        behavior[i] = NEW_WID;
    }

    /** @brief Allocate memory for the failure tolerance mechanism. */

    total_tasks =  config_mra.amount_of_tasks_mra[MRA_MAP] + config_mra.amount_of_tasks_mra[MRA_REDUCE];


    mra_task_ftm = (struct mra_ftsys_s*)xbt_new(struct mra_ftsys_s*, (total_tasks * (sizeof (struct mra_ftsys_s))));

    mra_ftm_done_s = (struct mra_ftsys_s*)xbt_new(struct mra_ftsys_s*, (total_tasks * (sizeof (struct mra_ftsys_s))));

    /** @brief Failure Tolerance Mechanism initialization */
    for (i=0; i < total_tasks; i++ )
    {
        mra_task_ftm[i].mra_ft_phase 						= 0;
        mra_task_ftm[i].mra_ft_wid   						= 0;
        mra_task_ftm[i].mra_ft_task_status 			= 0;
        mra_task_ftm[i].mra_ft_task_id 					= 0;
        mra_task_ftm[i].mra_ft_msg    					= 0;
        mra_ftm_done_s[i].mra_ft_task_status 		= 0;
        mra_task_ftm[i].mra_ft_pid[MRA_MAP] 		= 0;
        mra_task_ftm[i].mra_ft_pid[MRA_REDUCE] 	= 0;
        mra_ftm_done_s[i].mra_ft_task_id				= 0;
    }

    for (i=0; i < config_mra.mra_number_of_workers; i++ )
    {
        mra_ftm_done_s[i].mra_ft_nwid = NEW_WID;

    }

    while (job_mra.tasks_pending[MRA_MAP] + job_mra.tasks_pending[MRA_REDUCE] > 0)
    {

        /*Define msg*/
        msg = NULL;
        status = receive (&msg, MASTER_MRA_MAILBOX);
        /** @brief Control_timestamp is timestamp to failure detection. It is a trigger for the failure detector.  */
        control_timestamp = MSG_get_clock ();

        if (status == MSG_OK)
        {
            worker = MSG_task_get_source (msg);
            mra_wid = get_mra_worker_id (worker);
            /** @brief Identifies a new Worker */
            //time_ctl = (int) control_timestamp;
           // if (time_ctl == 0 || behavior[mra_wid] != NEW_WID)
            if (behavior[mra_wid]== NEW_WID)
            {
                mra_ftm_done_s[mra_wid].mra_ft_nwid = OPERATION;
                XBT_INFO ("%s, @ in %s \n", MSG_host_get_name (worker), (mra_ftm_done_s[mra_wid].mra_ft_nwid==OPERATION?"OPERATION":"NEW_WID"));
            }

            //=====
            if (mra_message_is (msg, SMS_HEARTBEAT_MRA) )
            {

                /** @brief Last valid heartbeat */
                mra_vc_last_hb[mra_wid] = job_mra.mra_heartbeats[mra_wid].wid_timestamp;
                /** @brief Fail Timeout Period to worker*/
                mra_vc_fail_timeout_period[mra_wid] = ((config_mra.failure_timeout_conf * config_mra.mra_heartbeat_interval) + mra_vc_last_hb[mra_wid]);

                /** @brief Call to get volunteer computing behavior */
                get_vc_behavior (control_timestamp, mra_wid, job_mra.mra_heartbeats[mra_wid].wid_timestamp);

                //  Begin - Detector test

       for (k=0; (k < config_mra.mra_number_of_workers); k++ )
        {
           mra_vc_wid = k;
          if ((behavior[mra_vc_wid] != VC_NORMAL ) && (behavior[mra_vc_wid ] != NEW_WID))
          {
            if (behavior[mra_vc_wid] == VC_FAILURE && mra_ftm_done_s[mra_vc_wid].mra_ft_nwid == OPERATION && config_mra_vc_file_line[1] > 0 )
            {
              if (mra_vc_lhb_fdt [mra_vc_wid ] == 0 || mra_vc_lhb_fdt [mra_vc_wid ]==-1)
              {
                mra_vc_lhb_fdt[k] = control_timestamp;
                XBT_INFO ("%s, Failure Detected %zd @ %g - %zd \n", MSG_host_get_name (config_mra.workers_mra[mra_vc_wid]),mra_vc_wid, mra_vc_lhb_fdt[k], mra_wid);

                //Adjust of tasks on each group
                mra_dfs_dist[mra_vc_wid].prev_exec[MRA_MAP] = 999999999;
                mra_tfm_recover( mra_vc_wid, total_tasks);
                mra_dfs_dist[mra_vc_wid].dist_bruta = 0;

              }
            }
            else
            {
              if (behavior[mra_vc_wid] == NEW_WID && control_timestamp > config_mra.mra_heartbeat_interval)
              {
                if (mra_vc_lhb_fdt [k] == 0)
                {
                  mra_vc_lhb_fdt[k] = -1;
                  XBT_INFO ("%s, Late Node Detected - Wid %zd @ %g \n", MSG_host_get_name (config_mra.workers_mra[mra_vc_wid]),
                  mra_vc_wid, mra_vc_lhb_fdt[k]);
                }
              }
            }
          }

        }
                                
               // End - Detector test

            }
            //=====

            if (mra_message_is (msg, SMS_HEARTBEAT_MRA))
            {
                //if(mra_dfs_dist[mra_wid].dist_bruta==0)
                //      	XBT_INFO("DEBBUGING - DIST IS 0. %s",MSG_host_get_name (config_mra.workers_mra[mra_wid]));
                mra_heartbeat = &job_mra.mra_heartbeats[mra_wid];

                /** @brief Fail or slow performance to Map */

                if (is_straggler_mra (MRA_MAP, worker))
                {

                    set_mra_speculative_tasks (MRA_MAP, worker);
                }
                else
                {
                    /* Remove machine of execution list */
                    if (mra_heartbeat->slots_av[MRA_MAP] > 0
                            && (mra_dfs_dist[mra_wid].dist_bruta > 0
                                && behavior[mra_wid] == VC_NORMAL))
                    {
                        send_map_to_mra_worker (worker);
                    }
                }

                /** @brief Fail or slow performance to Reduce */
                if (is_straggler_mra (MRA_REDUCE, worker) )
                {

                    set_mra_speculative_tasks (MRA_REDUCE, worker);
                }
                else
                {
                    /* Remove machine of execution list */
                    if (mra_heartbeat->slots_av[MRA_REDUCE] > 0
                            && (mra_dfs_dist[mra_wid].dist_bruta > 0
                                && behavior[mra_wid] == VC_NORMAL))
                    {
                        send_reduce_to_mra_worker (worker);
                    }
                }
            }

            else if (mra_message_is (msg, SMS_TASK_MRA_DONE))
            {
                ti = (mra_task_info_t) MSG_task_get_data (msg);

                if (job_mra.task_status[ti->mra_phase][ti->mra_tid] != T_STATUS_MRA_DONE)
                {
                    if (behavior[ti->mra_wid] == VC_NORMAL)
                    {

                        job_mra.task_status[ti->mra_phase][ti->mra_tid] = T_STATUS_MRA_DONE;

                        /* Begin Tolerance Failure Mechanism*/
                        mra_ftm_done_f.mra_ft_phase 		= ti->mra_phase;
                        mra_ftm_done_f.mra_ft_wid   		= ti->mra_wid;
                        mra_ftm_done_f.mra_ft_task_id 	= ti->mra_tid;
                        mra_status_tfm (mra_ftm_done_f.mra_ft_phase, mra_ftm_done_f.mra_ft_task_id, mra_ftm_done_f.mra_ft_wid, T_STATUS_MRA_DONE);

                        // XBT_INFO ("Work_id %zd @ Phase %s,Task %zd Done \n", mra_ftm_done_f.mra_ft_wid, (mra_ftm_done_f.mra_ft_phase==MRA_MAP?"MRA_MAP":"MRA_REDUCE"),mra_ftm_done_f.mra_ft_task_id );
                        /* End Tolerance Failure Mechanism*/ 
                        
                        if(ti->mra_phase==MRA_MAP && mra_dfs_dist[ti->mra_wid].mra_dist_data[MRA_MAP]>0)
                        {
                          mra_dfs_dist[ti->mra_wid].mra_dist_data[MRA_MAP]--;
                        }
                        job_mra.task_status[ti->mra_phase][ti->mra_tid] = T_STATUS_MRA_DONE;
                        finish_all_mra_task_copies (ti);
                        job_mra.tasks_pending[ti->mra_phase]--;
                        XBT_INFO ("MRA_%s task %zd done", (ti->mra_phase==MRA_MAP?"MRA_MAP":"MRA_REDUCE"),ti->mra_tid );
                        if (job_mra.tasks_pending[ti->mra_phase] <= 0)
                         {
                            XBT_INFO (" ");
                            if(ti->mra_phase==MRA_MAP)
                            {
                                stats_mra.map_time=MSG_get_clock ();
                                XBT_INFO ("MRA_MAP PHASE DONE");
                                XBT_INFO (" ");
                            }
                            else
                            {
                                stats_mra.reduce_time = MSG_get_clock () - stats_mra.map_time;
                                XBT_INFO ("MRA_REDUCE PHASE DONE");
                                XBT_INFO (" ");
                            }
                          }
                     }                                     
                }
                xbt_free_ref (&ti);
            }
            MSG_task_destroy (msg);
        }
    }

    fclose (tasks_log);
    fclose (failure_log);


    job_mra.finished = 1;

   /* dist_b_log = fopen ("total_dist_new.log", "w");
    for (mra_wid=0; mra_wid < config_mra.mra_number_of_workers; mra_wid++ )
    {
        fprintf (dist_b_log, " %s , ID: %zu \t Speed: %g \t Dist_Calc: %d \t Dist_Recalc: %u \t Before_fail: %d \t After_fail: %d \t Data Distribution: %d \n",
                 MSG_host_get_name (config_mra.workers_mra[mra_wid]),mra_wid,mra_dfs_dist[mra_wid].speed ,mra_dfs_dist[mra_wid].mra_calc_dist,
                 mra_dfs_dist[mra_wid].dist_bruta,mra_dfs_dist[mra_wid].mra_dist_data[MRA_MAP],mra_dfs_dist[mra_wid].mra_dist_fail[MRA_MAP],
                 (mra_dfs_dist[mra_wid].mra_dist_data[MRA_MAP]+ mra_dfs_dist[mra_wid].mra_dist_fail[MRA_MAP]));
    }
    fclose(dist_b_log); */

    print_mra_config ();
    print_mra_stats ();


    XBT_INFO ("JOB END");
    /*
    FIXME SimGrid Error
    finish_all_pids (); */
    return 0;
    //exit(0);
}

/**
* @brief Failure Tolerance Mechanism from volatile machine.
*   @param mra_phase: phase identification.
*   @param ft_task_id: task number.
*   @param ft_mra_wid: worker identification.
*   @param mra_ft_task_status: task status.
* mra_status_tfm (MRA_MAP, tid , sid, T_STATUS_MRA_PENDING );
*/

static void mra_status_tfm (enum mra_phase_e mra_phase, size_t ft_task_id, size_t ft_mra_wid, enum mra_task_status_e  mra_ft_task_status )
{
    size_t    k ;

    switch (mra_phase)
    {
    case MRA_MAP:
        k = ft_task_id;
        break;
    case MRA_REDUCE:
        k = ft_task_id + config_mra.amount_of_tasks_mra[MRA_MAP];
        break;
    }
    if (mra_ft_task_status == T_STATUS_MRA_DONE && mra_phase == mra_task_ftm[k].mra_ft_phase)
    {
        mra_ftsys_f.mra_ft_task_status = mra_ft_task_status;
        mra_task_ftm[k].mra_ft_task_status = mra_ftsys_f.mra_ft_task_status;
    }

    else
    {
        /*Task Phase*/
        mra_ftsys_f.mra_ft_phase = mra_phase;
        mra_task_ftm[k].mra_ft_phase = mra_ftsys_f.mra_ft_phase;
        /*Task Status*/
        mra_ftsys_f.mra_ft_task_status = mra_ft_task_status;
        mra_task_ftm[k].mra_ft_task_status = mra_ftsys_f.mra_ft_task_status;
        /*Task Work_id*/
        mra_ftsys_f.mra_ft_wid = ft_mra_wid;
        mra_task_ftm[k].mra_ft_wid = mra_ftsys_f.mra_ft_wid;
        /*Task Number*/
        if (mra_phase == MRA_MAP)
        {
            mra_ftsys_f.mra_ft_task_id = ft_task_id;
            mra_task_ftm[k].mra_ft_task_id = mra_ftsys_f.mra_ft_task_id;
        }
        else
        {
            mra_ftsys_f.mra_ft_task_id = ft_task_id;
            mra_task_ftm[k].mra_ft_task_id = mra_ftsys_f.mra_ft_task_id;
        }
        /*Dispatch Task*/
        mra_ftm_done_s[k].mra_ft_task_status = T_STATUS_MRA_DISP;

    }
}

/**
* @brief Failure Tolerance Mechanism from volatile machine.
* The mra_tfm_new_task function maintains the #replicas
* constat when a node go to shutdown 
* @param chunk: chunk for replica fixing.
*
*/
void mra_tfm_new_task (int chunk)
{
    int aux;
    for (aux = 0; aux < config_mra.mra_number_of_workers; aux++)
    {
        if(behavior[aux]==VC_NORMAL && mra_affinity[chunk] < config_mra.mra_chunk_replicas && mra_dfs_dist[aux].dist_bruta !=0)
        {
            chunk_owner_mra[chunk][aux] = 1;
            mra_affinity_f (chunk);
        }
    }
}

/**
* @brief Recovery is a failure tolerance module to task recovery.
* @brief Finds both Map and Reduce tasks from worker that failed.
* @param mra_ftm_vc_wid: worker identification.
* @param total_tasks: total task.
*
*/
static void mra_tfm_recover (size_t mra_ftm_vc_wid, int total_tasks)
{
    int k, kr,aux;
    size_t  owner;
 
    for (k=0; k < config_mra.amount_of_tasks_mra[MRA_MAP]; k++)
    {
        if (mra_task_ftm[k].mra_ft_wid == mra_ftm_vc_wid && mra_ftm_done_s[k].mra_ft_task_status == T_STATUS_MRA_DISP)
        {

            mra_task_ftm[k].mra_task_attrib = T_STATUS_MRA_FAILURE;

            if (mra_task_ftm[k].mra_ft_phase == MRA_MAP && job_mra.task_status[MRA_MAP][k] != T_STATUS_MRA_DONE  )
            {

                XBT_INFO ("FTM Recovery -> Map Task : %d", k);

                stats_mra.mra_map_recovery++;
 		//val = (mra_task_info_t) MSG_task_get_data(job_mra.task_list[MRA_MAP][k][0]);	

              //  free(job_mra.task_list[MRA_MAP][k][0]->data);
               // free(val);
             //   MSG_task_destroy (job_mra.task_list[MRA_MAP][k][0]);
            //    job_mra.task_list[MRA_MAP][k][0] = NULL;

                for ( owner = 0; owner < config_mra.mra_number_of_workers ; owner++ )
                {
                  if((behavior[owner] == NEW_WID || behavior[owner] == VC_NORMAL) && mra_dfs_dist[owner].mra_dist_data[MRA_MAP] < mra_dfs_dist[owner].dist_bruta)
                  {
                     mra_ftm_done_s[k].mra_ft_task_id = owner;
                     mra_vc_task_assing ( owner, k);
                     mra_dfs_dist[owner].mra_dist_data[MRA_MAP]++;
                      break;
                  }
                }
                mra_dfs_dist[mra_ftm_vc_wid].mra_dist_data[MRA_MAP]--;
                job_mra.task_status[MRA_MAP][k] = T_STATUS_MRA_PENDING;
                chunk_owner_mra[k][mra_ftm_vc_wid] = 0;
                /*
                * 04/04/16
                * if a chunk affinity goes to 0 (lost all replicas) a new replcation is needed for that chunk
               
                mra_affinity[k]--;
                if(mra_affinity[k] == 0)
                {
                    send_new_task(k);
                }
		*/

            }
        }
    }
    for (kr=0; kr < config_mra.amount_of_tasks_mra[MRA_REDUCE]; kr++)
    {
        aux= config_mra.amount_of_tasks_mra[MRA_MAP]+kr;
        if (mra_task_ftm[aux].mra_ft_wid == mra_ftm_vc_wid && mra_ftm_done_s[aux].mra_ft_task_status == T_STATUS_MRA_DISP
                && mra_task_ftm[aux].mra_ft_phase == MRA_REDUCE && job_mra.task_status[MRA_REDUCE][kr] != T_STATUS_MRA_DONE  )
        {
            if(job_mra.task_instances[MRA_REDUCE][kr]>0)
                job_mra.task_instances[MRA_REDUCE][kr]--;
            XBT_INFO ("FTM Recovery -> Reduce Task : %d", kr);

            /*for (i = 0; i < MAX_SPECULATIVE_COPIES; i++)
            {
            if (job_mra.task_list[MRA_REDUCE][kr][i] != NULL)
            {
            MSG_task_cancel (job_mra.task_list[MRA_REDUCE][kr][i]);
            job_mra.task_list[MRA_REDUCE][kr][i] = NULL;
            }
            }
            */
            stats_mra.mra_reduce_recovery++;

            /*Set speculative task to worker*/
            //   job_mra.task_status[MRA_REDUCE][kr] = T_STATUS_MRA_TIP_SLOW;
            job_mra.task_status[MRA_REDUCE][kr] = T_STATUS_MRA_PENDING;
        }
    }
}

/**
* @brief Behavior Detector from volatile machine.
* @brief Status from behavior (FAILURE; NEW_WID, TRANSIENT; NORMAL; UP_TRANSIENT;) and Volunteer wid
* @param control_timestamp: master timestamp.
* @param mra_wid: worker identification.
* @param last_hb_wid_timestamp: last timestamp from heartbeat, before worker failure.
*/

static void get_vc_behavior (double control_timestamp, size_t mra_wid, double last_hb_wid_timestamp )
{
    char comportamento[30];
    char estado_trab[20];
    char estado_fail[20];
    int     	i=0;
    /*
    char temp[20];
    sprintf(temp,"%.11f",control_timestamp);
    int v_dec= (temp[6] - '0')*100 + (temp[7] - '0')*10+temp[8] - '0';
    if(v_dec>500)
    {
    control_timestamp=control_timestamp+1e-4;
    }
    */
    while (i < config_mra.mra_number_of_workers )
    {
        if(mra_wid == i)
        {
            //Update Last valid heartbeat
            mra_vc_last_hb[i] = last_hb_wid_timestamp;

            // Update Fail Timeout Period to worker
            mra_vc_fail_timeout_period[i] = ((config_mra.failure_timeout_conf  * config_mra.mra_heartbeat_interval) + mra_vc_last_hb[i] );
            break;
        }
        i++;
    }

    for (i=0; i < config_mra.mra_number_of_workers; i++)
    {
        /** @brief Determine the machine status - ative or inactive*/
        if( (int)(control_timestamp - mra_vc_last_hb[i]) > config_mra.mra_heartbeat_interval )
        {
            //worker inactive
            vc_state_working[i] = 0;
            strcpy(estado_trab,"inactive"); //vinicius - debugging purpose

        }
        else
        {
            //worker active
            vc_state_working[i] = 1;
            strcpy(estado_trab,"active"); //vinicius - debugging purpose

        }
        /** @brief Determine Machine Failure */
        if ((control_timestamp > mra_vc_fail_timeout_period[i] ))
            //    && (control_timestamp - mra_vc_last_hb[i]) > config_mra.mra_heartbeat_interval)

        {
            // Failure state
            mra_vc_state_failure[i] = 0;
            strcpy(estado_fail,"Failure");  //vinicius - debugging purpose
        }
        else
        {
            // Operation
            mra_vc_state_failure[i] = 1;
            strcpy(estado_fail,"Operation"); //vinicius - debugging purpose
        }

        /** @brief Information about Machine behavior */

        if ((vc_state_working[i] == 0 && mra_vc_state_failure[i] == 0) && (int)mra_vc_fail_timeout_period[i] != 0)
        {

            mra_f_detec_f.mra_vc_status = VC_FAILURE;
            mra_f_detec_f.mra_vc_wid = i;
            behavior[mra_f_detec_f.mra_vc_wid] = mra_f_detec_f.mra_vc_status;
            mra_f_detec_f.mra_last_hbtime = mra_vc_last_hb[i];

            //	XBT_INFO ("%s, Failure detected %d  \n", MSG_host_get_name(config_mra.workers_mra[i]), mra_vc_status);
        }

        else if ((int)mra_vc_fail_timeout_period[i] == 0 && (int)mra_vc_last_hb[i] == 0)
        {
            mra_f_detec_f.mra_vc_status = NEW_WID;
            mra_f_detec_f.mra_vc_wid = i;
            behavior[mra_f_detec_f.mra_vc_wid] = mra_f_detec_f.mra_vc_status;
            mra_f_detec_f.mra_last_hbtime = mra_vc_last_hb[i];

            //	XBT_INFO ("%s, Failure detected %d  \n", MSG_host_get_name(config_mra.workers_mra[i]), mra_vc_status);
        }
        else if (vc_state_working[i] == 0 && mra_vc_state_failure[i] == 1 && mra_vc_last_hb[i] != 0 )
        {
            mra_f_detec_f.mra_vc_status = VC_TRANSIENT;
            mra_f_detec_f.mra_vc_wid = i;
            behavior[mra_f_detec_f.mra_vc_wid] = mra_f_detec_f.mra_vc_status;
            mra_f_detec_f.mra_last_hbtime = mra_vc_last_hb[i];
            //	XBT_INFO ("%s, Transient detected %d \n", MSG_host_get_name(config_mra.workers_mra[i]), mra_vc_status);
     
        }

        else if ((vc_state_working[i] == 1 && mra_vc_state_failure[i] == 1 ) && (int)mra_vc_fail_timeout_period[i] != 0)
        {

            if (behavior[i] == VC_TRANSIENT )
            {
                mra_f_detec_f.mra_vc_status = VC_NORMAL;
                mra_f_detec_f.mra_vc_wid = i;
                behavior[mra_f_detec_f.mra_vc_wid] = mra_f_detec_f.mra_vc_status;
                mra_f_detec_f.mra_last_hbtime = mra_vc_last_hb[i];
            }
            else if (behavior[i] ==  NEW_WID)

            {

                mra_f_detec_f.mra_vc_status = VC_NORMAL;
                mra_f_detec_f.mra_vc_wid = i;
                behavior[mra_f_detec_f.mra_vc_wid] = mra_f_detec_f.mra_vc_status;
                mra_f_detec_f.mra_last_hbtime = mra_vc_last_hb[i];

            }

        }


        switch(behavior[i])
        {
        case NEW_WID:
            strcpy(comportamento,"NEW_WID");
            break;
        case VC_NORMAL:
            strcpy(comportamento,"VC_NORMAL") ;
            break;
        case VC_FAILURE:
            strcpy(comportamento,"VC_FAILURE") ;
            break;
        case VC_TRANSIENT:
            strcpy(comportamento,"VC_TRANSIENT") ;
            break;
        case VC_UP_TRANSIENT:
            strcpy(comportamento,"VC_UP_TRANSIENT") ;
            break;
        case OPERATION:
            strcpy(comportamento,"OPERATION") ;
            break;

        default:
            break;
        }
        /*   @brief Failure Log archive - debug purpose


        if(behavior[i]==VC_TRANSIENT){
          fprintf(failure_log,"Control_timestamp %.6f, %s, Last_heartb %.6f, Fta %g, Machine State %s, Failure State %s, Status %s, Trigger %zd, Dist: %d,TRANSIENT_ERROR:%.6f ,hb: %d\n",
          control_timestamp, MSG_host_get_name(config_mra.workers_mra[i]), mra_vc_last_hb[i],mra_vc_fail_timeout_period[i], estado_trab, estado_fail, comportamento, mra_wid,mra_dfs_dist[mra_wid].dist_bruta,(control_timestamp - mra_vc_last_hb[i]),config_mra.mra_heartbeat_interval);
        }else{
          fprintf(failure_log,"Control_timestamp %g, %s, Last_heartb %g, Fta %g, Machine State %s, Failure State %s, Status %s, Trigger %zd	Dist: %d \n",
          control_timestamp, MSG_host_get_name(config_mra.workers_mra[i]), mra_vc_last_hb[i],
          mra_vc_fail_timeout_period[i], estado_trab, estado_fail, comportamento, mra_wid,mra_dfs_dist[mra_wid].dist_bruta );
        }
        */

    }

}

/*Adjust */


/** @brief  Print the job configuration. */
static void print_mra_config (void)
{
    XBT_INFO ("MRA_JOB CONFIGURATION:");
    XBT_INFO ("slots_mra: %d map, %d reduce", config_mra.mra_slots[MRA_MAP], config_mra.mra_slots[MRA_REDUCE]);
    XBT_INFO ("MRA_chunk replicas: %d", config_mra.mra_chunk_replicas);
    XBT_INFO ("MRA_chunk size: %.0f MB", config_mra.mra_chunk_size/1024/1024);
    XBT_INFO ("MRA_input chunks: %d", config_mra.mra_chunk_count);
    XBT_INFO ("MRA_input size: %d MB", config_mra.mra_chunk_count * (int)(config_mra.mra_chunk_size/1024/1024));
    XBT_INFO ("MRA_maps: %d", config_mra.amount_of_tasks_mra[MRA_MAP]);
    XBT_INFO ("MRA_reduces: %d", config_mra.amount_of_tasks_mra[MRA_REDUCE]);
    XBT_INFO ("grain factor: %d", config_mra.Fg);
    XBT_INFO ("MRA_map_output size: %.0f Bytes", (((config_mra.mra_chunk_size*(config_mra.mra_perc/100))/config_mra.amount_of_tasks_mra[MRA_REDUCE])));
    XBT_INFO ("MRA_workers: %d", config_mra.mra_number_of_workers);
    XBT_INFO ("MRA_grid power: %g flops", config_mra.grid_cpu_power);
    XBT_INFO ("MRA_average power: %g flops/s", config_mra.grid_average_speed);
    XBT_INFO ("MRA_heartbeat interval: %ds", config_mra.mra_heartbeat_interval);
    XBT_INFO ("MRA_Failure Timeout Period: %lg", config_mra.failure_timeout_conf);
    XBT_INFO ("Total Volatile Nodes: %d", (int)(ceil(config_mra.mra_number_of_workers * (double)config_mra.perc_vc_node/100)));

    XBT_INFO (" ");
}

/** @brief  Print job statistics. */
static void print_mra_stats (void)
{
    char nome_arquivo[50];
    int num_vol=(int) config_mra.perc_vc_node;
    snprintf(nome_arquivo, sizeof(nome_arquivo), "num_volatil_%d_%d.csv", config_mra.mra_number_of_workers,num_vol);

    FILE * stats_log=fopen(nome_arquivo,"a");
    fseek(stats_log, 0, SEEK_END);
    fprintf(stats_log,"%.0f;%d;%d;-;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%f;%f\n",config_mra.mra_chunk_size/1024/1024,
            config_mra.mra_chunk_count,config_mra.Fg,//config_mra.failure_timeout_conf,
            stats_mra.map_local_mra,stats_mra.mra_map_remote,stats_mra.map_spec_mra_l, stats_mra.map_spec_mra_r,
            stats_mra.mra_map_recovery, stats_mra.mra_map_remote + stats_mra.map_spec_mra_r, stats_mra.map_spec_mra_l + stats_mra.map_spec_mra_r , stats_mra.reduce_mra_normal,
            stats_mra.reduce_mra_spec,stats_mra.mra_reduce_recovery, stats_mra.map_time,stats_mra.reduce_time);

    XBT_INFO ("MRA_JOB STATISTICS:");
    XBT_INFO ("local maps: %d", stats_mra.map_local_mra);
    XBT_INFO ("non-local maps: %d", stats_mra.mra_map_remote);
    XBT_INFO ("speculative maps (local): %d", stats_mra.map_spec_mra_l);
    XBT_INFO ("speculative maps (remote): %d", stats_mra.map_spec_mra_r);
    XBT_INFO ("recovery maps: %d", stats_mra.mra_map_recovery);
    XBT_INFO ("total non-local maps: %d", stats_mra.mra_map_remote + stats_mra.map_spec_mra_r);
    XBT_INFO ("total speculative maps: %d", stats_mra.map_spec_mra_l + stats_mra.map_spec_mra_r);
    XBT_INFO ("normal reduces: %d", stats_mra.reduce_mra_normal);
    XBT_INFO ("speculative reduces: %d", stats_mra.reduce_mra_spec);
    XBT_INFO ("recovery reduces: %d", stats_mra.mra_reduce_recovery);
    XBT_INFO ("Map Time: %f",stats_mra.map_time);
    XBT_INFO ("Reduce Time: %f",stats_mra.reduce_time);
    fclose(stats_log);
}

/* @brief  Checks if a worker is a straggler.
* @param  worker  The worker to be probed.
* @return 1 if true, 0 if false.
*/

static int is_straggler_mra (enum mra_phase_e mra_phase, msg_host_t worker)
{
    int     task_count;
    size_t  mra_wid;


    mra_wid = get_mra_worker_id (worker);

    task_count = (config_mra.mra_slots[MRA_MAP] + config_mra.mra_slots[MRA_REDUCE]) - (job_mra.mra_heartbeats[mra_wid].slots_av[MRA_MAP] + job_mra.mra_heartbeats[mra_wid].slots_av[MRA_REDUCE]);

    switch (mra_phase)
    {
    case MRA_MAP:
        if (MSG_get_host_speed (worker) < mra_dfs_dist[mra_wid].avg_task_exec[MRA_MAP] && task_count > 0)
        {
            return 1;
        }
        break;

    case MRA_REDUCE:
        if (MSG_get_host_speed (worker) < mra_dfs_dist[mra_wid].avg_task_exec[MRA_MAP] && task_count > 0)
        {
            //  if (MSG_get_host_speed (worker) < config_mra.grid_average_speed && task_count > 0){
            return 1;
        }
        break;
    }
    return 0;
}



/* @brief  Returns for how long a task is running.
* @param  mra_task  The task to be probed.
* @return The amount of seconds since the beginning of the computation.
*/
static int task_time_elapsed_mra (msg_task_t mra_task)
{
    mra_task_info_t  ti;

    ti = (mra_task_info_t) MSG_task_get_data (mra_task);

    return (MSG_task_get_compute_duration (mra_task) - MSG_task_get_remaining_computation (mra_task))/ MSG_get_host_speed (config_mra.workers_mra[ti->mra_wid]);
}

/* @brief  Mark the tasks of a straggler as possible speculative tasks.
* @param  worker  The straggler worker.
*/

static void set_mra_speculative_tasks (enum mra_phase_e mra_phase, msg_host_t worker)
{
    size_t       tid;
    size_t       mra_wid;
    mra_task_info_t  ti;

    mra_wid = get_mra_worker_id (worker);

    switch (mra_phase)
    {
    case MRA_MAP:
        if (is_straggler_mra (MRA_MAP, worker) == 1 )
        {
            if (job_mra.mra_heartbeats[mra_wid].slots_av[MRA_MAP] < config_mra.mra_slots[MRA_MAP])
            {
                for (tid = 0; tid < config_mra.amount_of_tasks_mra[MRA_MAP]; tid++)
                {
                    if (job_mra.task_list[MRA_MAP][tid][0] != NULL)
                    {
                        ti = (mra_task_info_t) MSG_task_get_data (job_mra.task_list[MRA_MAP][tid][0]);
                        if (ti->mra_wid == mra_wid && task_time_elapsed_mra (job_mra.task_list[MRA_MAP][tid][0]) > 60)
                        {
                            job_mra.task_status[MRA_MAP][tid] = T_STATUS_MRA_TIP_SLOW;
                        }
                    }
                }
            }
        }
        break;

    case MRA_REDUCE:
        if (is_straggler_mra (MRA_REDUCE, worker) == 1 )
        {
            if (job_mra.mra_heartbeats[mra_wid].slots_av[MRA_REDUCE] < config_mra.mra_slots[MRA_REDUCE])
            {
                for (tid = 0; tid < config_mra.amount_of_tasks_mra[MRA_REDUCE]; tid++)
                {
                    if (job_mra.task_list[MRA_REDUCE][tid][0] != NULL)
                    {
                        ti = (mra_task_info_t) MSG_task_get_data (job_mra.task_list[MRA_REDUCE][tid][0]);
                        if (ti->mra_wid == mra_wid && task_time_elapsed_mra (job_mra.task_list[MRA_REDUCE][tid][0]) > 60)
                        {
                            job_mra.task_status[MRA_REDUCE][tid] = T_STATUS_MRA_TIP_SLOW;
                        }
                    }
                }
            }
        }
        break;
    }
}


/**
* @brief  Choose a map task, and send it to a worker.
* @param  dest  The destination worker.
*/
static void send_map_to_mra_worker (msg_host_t dest)
{
    char*   flags;
    int     task_type;
    size_t  chunk;
    size_t  sid = NONE; // chunk owner
    size_t  tid = NONE;// task id
    size_t  mra_wid;


    if (job_mra.tasks_pending[MRA_MAP] <= 0)
        return;

    enum { LOCAL, REMOTE, LOCAL_SPEC, REMOTE_SPEC,RECOVERY, NO_TASK };
    task_type = NO_TASK;

    mra_wid = get_mra_worker_id (dest);

    /* Look for a task for the worker. */

    for (chunk = 0; chunk < config_mra.mra_chunk_count; chunk++)
    {
        if (job_mra.task_status[MRA_MAP][chunk] == T_STATUS_MRA_PENDING )
        {

            if (((mra_dfs_dist[mra_wid].mra_dist_data[MRA_MAP] < mra_dfs_dist[mra_wid].dist_bruta ) && (behavior[mra_wid] != VC_FAILURE))
                    || (job_mra.task_status[MRA_MAP][chunk] == T_STATUS_MRA_TIP_SLOW
                        || mra_task_ftm[chunk].mra_task_attrib == T_STATUS_MRA_FAILURE))
            {
                if (chunk_owner_mra[chunk][mra_wid] )
                {
                  task_type = LOCAL;
                  tid = chunk;
                  mra_dfs_dist[mra_wid].mra_dist_data[MRA_MAP]++;
                  break;
                 }
                else
                {
                    sid = find_random_mra_chunk_owner (chunk);
                   if ( mra_dfs_dist[mra_wid].prev_exec[MRA_MAP] > (mra_dfs_dist[sid].prev_exec[MRA_MAP] + (config_mra.mra_chunk_size/ config_mra.mra_bandwidth)))
                      {
                        task_type = REMOTE;
                        tid = chunk;
                      }
                }
                         
             }
        else if (job_mra.task_status[MRA_MAP][chunk] == T_STATUS_MRA_TIP_SLOW
                 && task_type > REMOTE
                 && job_mra.task_instances[MRA_MAP][chunk] < 2)
        	{
            if (chunk_owner_mra[chunk][mra_wid])
            {
                task_type = LOCAL_SPEC;
                tid = chunk;
            }
            else if (task_type > LOCAL_SPEC)
            {
                task_type = REMOTE_SPEC;
                tid = chunk;
            }
        	}
        	}
    }

    switch (task_type)
    {
    case LOCAL:
        flags = "";
        sid = mra_wid;
        stats_mra.map_local_mra++;
        break;

    case REMOTE:
        if (mra_ftm_done_s[mra_wid].mra_ft_nwid == NEW_WID)
        {
            flags = "(move data non-local from new worker)";
            sid = find_random_mra_chunk_owner (tid);
            mra_ftm_done_s[tid].mra_ft_task_id = 0;
            mra_dfs_dist[mra_wid].mra_dist_fail[MRA_MAP]++;
        }
        else
        {
            flags = "(non-local)";
            sid = find_random_mra_chunk_owner (tid);
            mra_dfs_dist[mra_wid].mra_dist_data[MRA_MAP]++;
        }
        stats_mra.mra_map_remote++;
        break;

    case LOCAL_SPEC:
        flags = "(speculative)";
        sid = mra_wid;
        stats_mra.map_spec_mra_l++;
        break;

    case REMOTE_SPEC:
        flags = "(non-local, speculative)";
        sid = find_random_mra_chunk_owner (tid);
        stats_mra.map_spec_mra_r++;
        break;

    default:
        return;
    }

    XBT_INFO ("MRA_map %zu assigned to %s %s", tid, MSG_host_get_name (dest), flags);

    /* Tolerance Failure Mechanism*/
    //TODO change to if (sid != mra_wid)
    if (!strcmp(flags,"(non-local)") || !strcmp(flags,"(move data non-local from new worker)") || !strcmp(flags,"(non-local, speculative)"))
    {
        //  XBT_INFO("Non-local destination is %s ",MSG_host_get_name (dest));
        mra_status_tfm (MRA_MAP, tid , (size_t) mra_wid, T_STATUS_MRA_PENDING );

    }
    else
        mra_status_tfm (MRA_MAP, tid , (size_t) sid, T_STATUS_MRA_PENDING );

    //mra_ftm_done_f.mra_disp[MRA_MAP]++;

    /* Send mra_task*/
    send_mra_task (MRA_MAP, tid, sid, dest);
}

/**
* @brief  Choose a reduce task, and send it to a worker.
* @param  dest  The destination worker.
*/
static void send_reduce_to_mra_worker (msg_host_t dest)
{
    char*   flags;
    int     task_type;
    size_t  t;
    size_t  tid = NONE;
    size_t	mra_wid;
    /**
    * @brief Hadoop code transfer initialize on 5% task concluded
    * @brief DEFAULT_COMPLETED_MAPS_PERCENT_FOR_REDUCE_SLOWSTART = 0.05f
    */

    if (job_mra.tasks_pending[MRA_REDUCE] <= 0 || (float)job_mra.tasks_pending[MRA_MAP]/config_mra.amount_of_tasks_mra[MRA_MAP] > 0.95)
        return;
    enum { NORMAL, SPECULATIVE, NO_TASK };
    task_type = NO_TASK;

    for (t = 0; t < config_mra.amount_of_tasks_mra[MRA_REDUCE]; t++)
    {
        if (job_mra.task_status[MRA_REDUCE][t] == T_STATUS_MRA_PENDING)
        {
            task_type = NORMAL;
            tid = t;
            break;
        }
        else if (job_mra.task_status[MRA_REDUCE][t] == T_STATUS_MRA_TIP_SLOW && job_mra.task_instances[MRA_REDUCE][t] < 2)
        {
            task_type = SPECULATIVE;
            tid = t;
        }
    }
    switch (task_type)
    {
    case NORMAL:
        flags = "";
        stats_mra.reduce_mra_normal++;
        break;

    case SPECULATIVE:
        flags = "(speculative)";
        stats_mra.reduce_mra_spec++;
        break;

    default:
        return;
    }

    XBT_INFO ("MRA_reduce %zu assigned to %s %s", tid, MSG_host_get_name (dest), flags);

    mra_wid = get_mra_worker_id (dest);
    worker_reduce_tasks[mra_wid]++;
    /* Tolerance Failure Mechanism*/
    mra_status_tfm (MRA_REDUCE, tid , mra_wid , T_STATUS_MRA_PENDING );

    /* Send mra_task*/
    send_mra_task (MRA_REDUCE, tid, NONE, dest);
}

/** send_mra_task (MRA_MAP, tid, sid, dest);
* @brief  Send a task to a worker.
* @param  mra_phase	The current job phase.
* @param  tid       	The task ID.
* @param  data_src  	The ID of the DataNode that owns the task data.
* @param  dest      	The destination worker.
*/
static void send_mra_task (enum mra_phase_e mra_phase, size_t tid, size_t data_src, msg_host_t dest)
{
    char         			mailbox[MAILBOX_ALIAS_SIZE];
    int          			i;
    double       			cpu_required = 0.0;
    msg_task_t   			mra_task = NULL;
    mra_task_info_t  	task_info;
    size_t       			mra_wid;

    mra_wid = get_mra_worker_id (dest);

    cpu_required = user_mra.task_mra_cost_f (mra_phase, tid, mra_wid);

		if ( mra_phase == MRA_REDUCE )
		{
		  cpu_required *= config_mra.mra_chunk_count;
		}


    task_info = xbt_new (struct mra_task_info_s, 1);
    mra_task = MSG_task_create (SMS_TASK_MRA, cpu_required, 0.0, (void*) task_info);

    task_info->mra_phase = mra_phase;
    task_info->mra_tid = tid;
    task_info->mra_src = data_src;
    task_info->mra_wid = mra_wid;
    task_info->mra_task = mra_task;
    task_info->shuffle_mra_end = 0.0;

    // for tracing purposes...
    MSG_task_set_category (mra_task, (mra_phase==MRA_MAP?"MRA_MAP":"MRA_REDUCE"));

    if (job_mra.task_status[mra_phase][tid] != T_STATUS_MRA_TIP_SLOW)
        job_mra.task_status[mra_phase][tid] = T_STATUS_MRA_TIP;

    job_mra.mra_heartbeats[mra_wid].slots_av[mra_phase]--;

    for (i = 0; i < MAX_SPECULATIVE_COPIES; i++)
    {
        if (job_mra.task_list[mra_phase][tid][i] == NULL)
        {
            job_mra.task_list[mra_phase][tid][i] = mra_task;
            break;
        }
    }

    fprintf (tasks_log, "%d_%zu_%d,%s,%zu,%.3f,START,\n", mra_phase, tid, i, (mra_phase==MRA_MAP?"MRA_MAP":"MRA_REDUCE"), mra_wid, MSG_get_clock ());

#ifdef VERBOSE
    XBT_INFO ("TX: %s > %s", SMS_TASK_MRA, MSG_host_get_name (dest));
#endif

    sprintf (mailbox, TASKTRACKER_MRA_MAILBOX, mra_wid);
    xbt_assert (MSG_task_send (mra_task, mailbox) == MSG_OK, "ERROR SENDING MESSAGE");

    job_mra.task_instances[mra_phase][tid]++;
}

/**
* @brief  Kill all copies of a task.
* @param  ti  The task information of any task instance.
*/
static void finish_all_mra_task_copies (mra_task_info_t ti)
{
    int     i;
    int     mra_phase = ti->mra_phase;
    size_t  tid = ti->mra_tid;


    for (i = 0; i < MAX_SPECULATIVE_COPIES; i++)
    {
        if (job_mra.task_list[mra_phase][tid][i] != NULL)
        {
            //MSG_task_cancel (job.task_list[phase][tid][i]);
            MSG_task_destroy (job_mra.task_list[mra_phase][tid][i]);
            job_mra.task_list[mra_phase][tid][i] = NULL;
            fprintf (tasks_log, "%d_%zu_%d,%s,%zu,%.3f,END,%.3f\n", ti->mra_phase, tid, i, (ti->mra_phase==MRA_MAP?"MRA_MAP":"MRA_REDUCE"), ti->mra_wid, MSG_get_clock (), ti->shuffle_mra_end);
        }
    }
}

/* Kill all PIDs - SIMGRID Problem
static void finish_all_pids (void)
{
int 	k, ftm_pid;

for (k=0; k < total_tasks; k++)
{
ftm_pid = mra_task_ftm[k].mra_ft_pid[MRA_MAP];
MSG_process_killall (ftm_pid);
}

} */
