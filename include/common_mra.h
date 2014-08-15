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

#ifndef MRA_COMMON_H
#define MRA_COMMON_H

#include <msg/msg.h>
#include <xbt/sysdep.h>
#include <xbt/log.h>
#include <xbt/asserts.h>
#include "mra.h"

/** @brief  Initialize dist_bruta, task_exec, avg_task_exec. */

int*        dist_bruta;
double*     task_exec;
double*     avg_task_exec_map;
double*     avg_task_exec_reduce;
int					Fg;
int         mra_perc;



/* Hearbeat parameters. */
#define MRA_HEARTBEAT_MIN_INTERVAL 3
#define MRA_HEARTBEAT_TIMEOUT 600


/* Short message names. */
#define SMS_GET_MRA_CHUNK "SMS-GC"
#define SMS_GET_INTER_MRSG_PAIRS "SMS-GIP"
#define SMS_HEARTBEAT_MRA "SMS-HB"
#define SMS_TASK_MRSG "SMS-T"
#define SMS_TASK_MRSG_DONE "SMS-TD"
#define SMS_FINISH_MRA "SMS-F"

#define NONE (-1)
#define MAX_SPECULATIVE_COPIES 3

/* Mailbox related. */
#define MAILBOX_ALIAS_SIZE 256
#define MASTER_MRA_MAILBOX "MASTER_MRA"
#define DATANODE_MRA_MAILBOX "%zu:DN"
#define TASKTRACKER_MRA_MAILBOX "%zu:TT"
#define TASK_MRA_MAILBOX "%zu:%d"

/** @brief  Possible task status. */
enum task_status_e {
    /* The initial status must be the first enum. */
    T_STATUS_MRA_PENDING,
    T_STATUS_MRA_TIP,
    T_STATUS_MRA_TIP_SLOW,
    T_STATUS_MRA_DONE
};

/** @brief  Information sent by the workers with every heartbeat. */
struct mra_heartbeat_s {
    int  slots_av[2];
};

typedef struct mra_heartbeat_s* mra_heartbeat_t;


struct config_s {
    double         mra_chunk_size;
    double         grid_average_speed;
    double         grid_cpu_power;
    int            mra_chunk_count;
    int            mra_chunk_replicas;
    int            mra_heartbeat_interval;
    int            amount_of_tasks_mra[2];
    int            mra_number_of_workers;
    int            mra_slots[2];
    int            mra_perc;
    int            initialized;
    msg_host_t*    workers_mra;
} config_mra;

struct job_s {
    int           finished;
    int           tasks_pending[2];
    int*          task_instances[2];
    int*          task_status[2];
    msg_task_t**  task_list[2];
    size_t**      map_output;
    mra_heartbeat_t   mra_heartbeats;
} job_mra;

/** @brief  Information sent as the task data. */
struct task_info_s {
    enum phase_e  phase;
    size_t        mra_tid;
    size_t        mra_src;
    size_t        mra_wid;
    int           mra_pid;
    msg_task_t    mra_task;
    size_t*       map_output_copied;
    double        shuffle_mra_end;
};

typedef struct task_info_s* mra_task_info_t;

struct stats_s {
    int   map_local_mra;
    int   mra_map_remote;
    int   map_spec_mra_l;
    int   map_spec_mra_r;
    int   reduce_mra_normal;
    int   reduce_mra_spec;
} stats_mra;

struct user_s {
    double (*task_mra_cost_f)(enum phase_e phase, size_t tid, size_t mra_wid);
    void (*mra_dfs_f)(char** mra_dfs_matrix, size_t chunks, size_t workers_mra, int replicas);
    int (*map_mra_output_f)(size_t mid, size_t rid);
} user_mra;


/** 
 * @brief  Send a message/task.
 * @param  str      The message.
 * @param  cpu      The amount of cpu required by the task.
 * @param  net      The message size in bytes.
 * @param  data     Any data to attatch to the message.
 * @param  mailbox  The destination mailbox alias.
 * @return The MSG status of the operation.
 */
msg_error_t send (const char* str, double cpu, double net, void* data, const char* mailbox);

/** 
 * @brief  Send a short message, of size zero.
 * @param  str      The message.
 * @param  mailbox  The destination mailbox alias.
 * @return The MSG status of the operation.
 */
msg_error_t send_mra_sms (const char* str, const char* mailbox);

/** 
 * @brief  Receive a message/task from a mailbox.
 * @param  msg      Where to store the received message.
 * @param  mailbox  The mailbox alias.
 * @return The status of the transfer.
 */
msg_error_t receive (msg_task_t* msg, const char* mailbox);

/** 
 * @brief  Compare the message from a task with a string.
 * @param  msg  The message/task.
 * @param  str  The string to compare with.
 * @return A positive value if matches, zero if doesn't.
 */
int message_is (msg_task_t msg, const char* str);

/**
 * @brief  Return the maximum of two values.
 */
int maxval (int a, int b);

size_t map_mra_output_size (size_t mid);

size_t reduce_mra_input_size (size_t rid);

#endif /* !MRA_COMMON_H */
