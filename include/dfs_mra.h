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

#ifndef MRADFS_H
#define MRADFS_H



struct mra_dfs_het_s {
    enum mra_phase_e			mra_phase;
    double								task_exec[2]; 
    double								avg_task_exec[2];
    double                prev_exec[2];
    double                temp_corr[2];
    double								p_cap_wid;
    int										mra_dist_data[2];
    int                   mra_dist_fail[2];
    int   								mra_calc_dist;
    int 									dist_sum;
		int 									dist_bruta;
		double								speed;
		int 									adjust;
		int                   offset_dist;
		int                   min_dist;
		int 									max_dist;
		

} mra_dfs_het_f;

struct mra_dfs_het_s *mra_dfs_dist;




/** @brief  Matrix that maps chunks to workers. */
char**  chunk_owner_mra;

/**
 * @brief  Distribute chunks (and replicas) to DataNodes.
 */
void distribute_data_mra (void);

/**
 * @brief  Default data distribution algorithm.
 */
void default_mra_dfs_f (char** mra_dfs_matrix, size_t chunks, size_t workers_mra, int replicas);

/**
*  @brief Affinity and replica funcions.
*/
void mra_affinity_f (size_t chunk);

void mra_replica_f (int *totalOwnedChunks);

/** @brief Reset dfs structure for a node delay*/
void mra_vc_clean_rpl (size_t owner);

/** @brief Assign a task recovery to worker has been late. */
//void mra_vc_task_assing (size_t owner, size_t chunk);


/**@brief Min_max algorithm */
void min_max_f(int adjust, int chunk_num, int fault_mode);

/**
 * @brief  Choose a random DataNode that owns a specific chunk.
 * @param  cid  The chunk ID.
 * @return The ID of the DataNode.
 */
size_t find_random_mra_chunk_owner (int cid);

/**
 * @brief  DataNode main function.
 *
 * Process that listens for data requests.
 */
int data_node_mra (int argc, char *argv[]);





#endif /* !MRADFS_H */
