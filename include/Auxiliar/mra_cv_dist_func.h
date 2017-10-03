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


enum type_pdf_e {
    WEIBULL,
    GAMMA
};


/** @brief  Matrix that maps the worker available. */
char**  availab_owner_mra;

/**
 * @brief  Distribute chunks (and replicas) to DataNodes.
 */
void distrib_availab_mra (void);

/**
 * @brief  Default data distribution algorithm.
 */
void default_mrsg_dfs_f (char** dfs_matrix, size_t chunks, size_t workers_mrsg, int replicas);

/*
* @brief Distribution Function to determine availability and unavailability following
* Javadi, Bahman, Matawie, Kenan, Anderson, David: Modeling and Analysis of Resources Availability in Volunteer Computing Systems, 
* IEEE International Performance Computing and Communications Conference, 32th edition, IEEE Computer Society, December 2013
*
*/
/* @brief Lognormal
 * static double gsl_ran_lognormal_pdf (double x, double zeta, double sigma); 
 */
static lognormal (double logn.x, double zeta, double sigma);

/* @brief  Weibull
 * static double gsl_ran_weibull_pdf (double x, double a, double b); 
 */
static weibull (double weib.x, double weib.scale, double weib.look);
/* @brief  Gamma
 * static double gsl_ran_gamma_pdf (double x, double a, double b);  
 */
static gamma (double gam.x, double gam.scale, double gam.look);
/* @brief  Exponential 
 * static double gsl_ran_exponential_pdf (double x, double mu); 
 */
static exp (double exp.x, double exp.avg);


struct func_pdf {
		char mra_work_id;
		double x;
		double scale;
		double look;
		double avg;
}logn, weib, gam, exp;


#endif /* !MRACV_H */

