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

#ifndef MRA_H
#define MRA_H


#include <stdlib.h>

/** @brief  Possible execution phases. */
enum mra_phase_e {
    MRA_MAP,
    MRA_REDUCE
};

void MRA_init (void);

int MRA_main (const char* plat, const char* depl, const char* conf, const char* trace_cv);

void MRA_set_task_mra_cost_f ( double (*f)(enum mra_phase_e mra_phase, size_t tid, size_t mra_wid) );

void MRA_set_dfs_f ( void (*f)(char** mra_dfs_matrix, size_t chunks, size_t workers_mra, int replicas) );

void MRA_set_map_mra_output_f ( int (*f)(size_t mid, size_t rid) );

#endif /* !MRA_H */
