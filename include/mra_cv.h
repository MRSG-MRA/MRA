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

#ifndef MRACV_H
#define MRACV_H

#include <stdlib.h>

int 				**vc_node;
int 				**vc_type;
long double **vc_start;
long double **vc_end;
int 				config_mra_vc_file_line[4];
double 			vc_traces_time;
char*    		vc_status;
double 			control_timestamp; 

double*  			mra_vc_last_hb;
double*				mra_vc_fail_timeout_period;	
int* 					mra_vc_state_failure;
int* 					vc_state_working;

int*				mra_affinity;

int         total_tasks;

/** @brief  Matrix that VC to workers. */
char**  func_mra_vc;



/** @brief  Information of availability . */

enum mra_vc_status_e {
		VC_NORMAL,
    VC_FAILURE,
    VC_TRANSIENT,
    VC_UP_TRANSIENT
};

enum mra_vc_status_e *behavior;

//enum mra_vc_status_e behavior_vc_status;

/** @brief  Information of failure detection system. */
struct mra_fd_s {
    enum mra_vc_status_e		mra_vc_status;
    size_t									mra_vc_wid; 
    double									mra_last_hbtime;
} mra_f_detec_f;

/** @brief  Information of failure tolerance system. */
struct mra_ftsys_s {
    enum mra_phase_e				mra_ft_phase;
    enum mra_task_status_e 	mra_ft_task_status;
    size_t									mra_ft_wid; 
    size_t									mra_ft_task_id;
    size_t 									mra_ft_msg;
    int  										mra_ft_pid[2];
    enum mra_task_status_e  mra_task_attrib;
} mra_ftsys_f, mra_ftm_done_f;

struct mra_ftsys_s *mra_task_ftm;

struct mra_ftsys_s *mra_ftm_done_s;



/** @brief  Information of traces . */
typedef struct mra_vc_traces
{
    int    mra_vc_node_id;
    int    mra_vc_status_type;
    long double mra_vc_start;
    long double mra_vc_end;
} VC_TRACE;





#endif /* !MRACV_H */

