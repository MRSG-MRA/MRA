/* https://github.com/ampl/gsl/blob/master/randist/
*
* Copyright (C) 1996, 1997, 1998, 1999, 2000, 2007 James Theiler, Brian Gough
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or (at
* your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/




#ifndef MRADISTFUNC_H
#define MRADISTFUNC_H

double Random(void);


/*
* @brief Distribution Function to determine availability and unavailability following
* Javadi, Bahman, Matawie, Kenan, Anderson, David: Modeling and Analysis of Resources Availability in Volunteer Computing Systems, IEEE International Performance Computing and Communications Conference, 32th edition, IEEE Computer Society, December 2013
*
*
*/

/* Lognormal*/
static double lognormal (const gsl_rng * r, const double zeta, const double sigma);
/* Lognormal Probability density function*/
static double lognormal_pdf (const double x, const double zeta, const double sigma);
/* Weibull*/
static double weibull (const gsl_rng * r, const double a, const double b);
/* Weibull Probability density function*/
static double weibull_pdf (const double x, const double a, const double b);
/* Exponential */
static double exponential (const gsl_rng * r, const double mu);
/* Exponential Probability density function*/
static double exponential_pdf (const double x, const double mu);
/* Gamma */
static double gamma (const gsl_rng * r, const double a, const double b)
static double gamma_large (const gsl_rng * r, const double a);
static double gamma_frac (const gsl_rng * r, const double a);
double gsl_ran_gamma_int (const gsl_rng * r, const unsigned int a)
double gsl_ran_gamma (const gsl_rng * r, const double a, const double b);

/* Gamma Probability density function*/
static double gamma_pdf (const double x, const double a, const double b);



#endif /* !MRADISTFUNC_H */

