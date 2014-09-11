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
#include "mra.h"


void MRA_init (void)
{
    user_mra.task_mra_cost_f = NULL;
    user_mra.mra_dfs_f = default_mra_dfs_f;
    user_mra.map_mra_output_f = NULL;
}

void MRA_set_task_mra_cost_f ( double (*f)(enum mra_phase_e mra_phase, size_t tid, size_t mra_wid) )
{
    user_mra.task_mra_cost_f = f;
}

void MRA_set_dfs_f ( void (*f)(char** mra_dfs_matrix, size_t chunks, size_t workers_mra, int replicas) )
{
    user_mra.mra_dfs_f = f;
}

void MRA_set_map_mra_output_f ( int (*f)(size_t mid, size_t rid) )
{
    user_mra.map_mra_output_f = f;
}

