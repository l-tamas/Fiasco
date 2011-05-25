/*
 *  approx.c:		Approximation of range images with matching pursuit
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:50:51 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#include "config.h"

#include <math.h>

#include "types.h"
#include "macros.h"
#include "error.h"

#include "cwfa.h"
#include "ip.h"
#include "rpf.h"
#include "domain-pool.h"
#include "misc.h"
#include "list.h"
#include "approx.h"
#include "coeff.h"
#include "wfalib.h"

/*****************************************************************************

			     local variables
  
*****************************************************************************/

typedef struct mp
{
   word_t exclude [MAXEDGES];
   word_t indices [MAXEDGES + 1];
   word_t into [MAXEDGES + 1];
   real_t weight [MAXEDGES];
   real_t matrix_bits;
   real_t weights_bits;
   real_t err;
   real_t costs;
} mp_t;

/*****************************************************************************

			     prototypes
  
*****************************************************************************/

static void
orthogonalize (unsigned index, unsigned n, unsigned level, real_t min_norm,
	       const word_t *domain_blocks, const coding_t *c);
static void 
matching_pursuit (mp_t *mp, bool_t full_search, real_t price,
		  unsigned max_edges, int y_state, const range_t *range,
		  const domain_pool_t *domain_pool, const coeff_t *coeff,
		  const wfa_t *wfa, const coding_t *c);

/*****************************************************************************

				public code
  
*****************************************************************************/

real_t 
approximate_range (real_t max_costs, real_t price, int max_edges,
		   int y_state, range_t *range, domain_pool_t *domain_pool,
		   coeff_t *coeff, const wfa_t *wfa, const coding_t *c)
/*
 *  Approximate image block 'range' by matching pursuit. This functions
 *  calls the matching pursuit algorithm several times (with different
 *  parameters) in order to find the best approximation. Refer to function
 *  'matching_pursuit()' for more details about parameters.
 *
 *  Return value:
 *	approximation costs
 */
{
   mp_t	  mp;
   bool_t success = NO;

   /*
    *  First approximation attempt: default matching pursuit algorithm.
    */
   mp.exclude [0] = NO_EDGE;
   matching_pursuit (&mp, c->options.full_search, price, max_edges,
		     y_state, range, domain_pool, coeff, wfa, c);

   /*
    *  Next approximation attempt: remove domain block mp->indices [0]
    *  from domain pool (vector with smallest costs) and run the
    *  matching pursuit again.
    */
   if (c->options.second_domain_block)
   {
      mp_t tmp_mp = mp;
      
      tmp_mp.exclude [0] = tmp_mp.indices [0];
      tmp_mp.exclude [1] = NO_EDGE;
	    
      matching_pursuit (&tmp_mp, c->options.full_search, price, max_edges,
			y_state, range, domain_pool, coeff, wfa, c);
      if (tmp_mp.costs < mp.costs)	/* success */
      {
	 success = YES;
	 mp      = tmp_mp;
      }
   }

   /*
    *  Next approximation attempt: check whether some coefficients have
    *  been quantized to zero. Vectors causing the underflow are
    *  removed from the domain pool and then the matching pursuit
    *  algorithm is run again (until underflow doesn't occur anymore).
    */
   if (c->options.check_for_underflow)
   {
      int  iteration = -1;
      mp_t tmp_mp    = mp;
      
      do
      {
	 int i;
 
	 iteration++;
	 tmp_mp.exclude [iteration] = NO_EDGE;
	 
	 for (i = 0; isdomain (tmp_mp.indices [i]); i++)
	    if (tmp_mp.weight [i] == 0)
	    {
	       tmp_mp.exclude [iteration] = tmp_mp.indices [i];
	       break;
	    }
      
	 if (isdomain (tmp_mp.exclude [iteration])) /* try again */
	 {
	    tmp_mp.exclude [iteration + 1] = NO_EDGE;
	    
	    matching_pursuit (&tmp_mp, c->options.full_search, price,
			      max_edges, y_state, range, domain_pool,
			      coeff, wfa, c);
	    if (tmp_mp.costs < mp.costs)	/* success */
	    {
	       success = YES;
	       mp      = tmp_mp;
	    }
	 }
      } while (isdomain (tmp_mp.exclude [iteration])
	       && iteration < MAXEDGES - 1);
   }

   /*
    *  Next approximation attempt: check whether some coefficients have
    *  been quantized to +/- max-value. Vectors causing the overflow are
    *  removed from the domain pool and then the matching pursuit
    *  algorithm is run again (until overflow doesn't occur anymore).
    */
   if (c->options.check_for_overflow)
   {
      int  iteration = -1;
      mp_t tmp_mp    = mp;
      
      do
      {
	 int i;
 
	 iteration++;
	 tmp_mp.exclude [iteration] = NO_EDGE;
	 
	 for (i = 0; isdomain (tmp_mp.indices [i]); i++)
	 {
	    rpf_t *rpf = tmp_mp.indices [i] ? coeff->rpf : coeff->dc_rpf;
	    
	    if (tmp_mp.weight [i] == btor (rtob (200, rpf), rpf)
		|| tmp_mp.weight [i] == btor (rtob (-200, rpf), rpf))
	    {
	       tmp_mp.exclude [iteration] = tmp_mp.indices [i];
	       break;
	    }
	 }
      
	 if (isdomain (tmp_mp.exclude [iteration])) /* try again */
	 {
	    tmp_mp.exclude [iteration + 1] = NO_EDGE;
	    
	    matching_pursuit (&tmp_mp, c->options.full_search, price,
			      max_edges, y_state, range, domain_pool,
			      coeff, wfa, c);
	    if (tmp_mp.costs < mp.costs)	/* success */
	    {
	       success = YES;
	       mp      = tmp_mp;
	    }
	 }
      } while (isdomain (tmp_mp.exclude [iteration])
	       && iteration < MAXEDGES - 1);
   }

   /*
    *  Finally, check whether the best approximation has costs
    *  smaller than 'max_costs'.
    */
   if (mp.costs < max_costs) 
   {
      int    edge;
      bool_t overflow  = NO;
      bool_t underflow = NO;
      int    new_index, old_index;

      new_index = 0;
      for (old_index = 0; isdomain (mp.indices [old_index]); old_index++)
	 if (mp.weight [old_index] != 0)
	 {
	    rpf_t *rpf = mp.indices [old_index] ? coeff->rpf : coeff->dc_rpf;
	    
	    if (mp.weight [old_index] == btor (rtob (200, rpf), rpf)
		|| mp.weight [old_index] == btor (rtob (-200, rpf), rpf))
	       overflow = YES;
	    
	    mp.indices [new_index] = mp.indices [old_index];
	    mp.into [new_index]    = mp.into [old_index];
	    mp.weight [new_index]  = mp.weight [old_index];
	    new_index++;
	 }
	 else
	    underflow = YES;
      
      mp.indices [new_index] = NO_EDGE;
      mp.into  [new_index]   = NO_EDGE;

      /*
       *  Update of probability models
       */
      {
	 word_t *domain_blocks = domain_pool->generate (range->level, y_state,
							wfa,
							domain_pool->model);
	 domain_pool->update (domain_blocks, mp.indices,
			      range->level, y_state, wfa, domain_pool->model);
	 coeff->update (mp.weight, mp.into, range->level, coeff);
	 
	 Free (domain_blocks);
      }
      
      for (edge = 0; isedge (mp.indices [edge]); edge++)
      {
	 range->into [edge]   = mp.into [edge];
	 range->weight [edge] = mp.weight [edge];
      }
      range->into [edge]  = NO_EDGE;
      range->matrix_bits  = mp.matrix_bits;
      range->weights_bits = mp.weights_bits;
      range->err          = mp.err;
   }
   else
   {
      range->into [0] = NO_EDGE;
      mp.costs	      = MAXCOSTS;
   }
   
   return mp.costs;
}

/*****************************************************************************

			     local variables
  
*****************************************************************************/

static real_t norm_ortho_vector [MAXSTATES];     
/*
 *  Square-norm of the i-th vector of the orthogonal basis (OB)
 *  ||o_i||^2; i = 0, ... ,n
 */
static real_t ip_image_ortho_vector [MAXEDGES];
/* 
 *  Inner product between the i-th vector of the OB and the given range: 
 *  <b, o_i>; i = 0, ... ,n 
 */
static real_t ip_domain_ortho_vector [MAXSTATES][MAXEDGES];
/* 
 *  Inner product between the i-th vector of the OB and the image of domain j: 
 *  <s_j, o_i>; j = 0, ... , wfa->states; i = 0, ... ,n, 
 */
static real_t rem_denominator [MAXSTATES];     
static real_t rem_numerator [MAXSTATES];
/*
 *  At step n of the orthogonalization the comparitive value
 *  (numerator_i / denominator_i):= <b, o_n>^2 / ||o_n|| ,
 *  is computed for every domain i,
 *  where o_n := s_i - \sum(k = 0, ... , n-1) {(<s_i, o_k> / ||o_k||^2) o_k}
 *  To avoid computing the same values over and over again,
 *  the constant (remaining) parts of every domain are
 *  stored in 'rem_numerator' and 'rem_denominator' separately
 */
static bool_t used [MAXSTATES];    
/*
 *  Shows whether a domain image was already used in a
 *  linear combination (YES) or not (NO)
 */

/*****************************************************************************

				private code
  
*****************************************************************************/

static void 
matching_pursuit (mp_t *mp, bool_t full_search, real_t price,
		  unsigned max_edges, int y_state, const range_t *range,
		  const domain_pool_t *domain_pool, const coeff_t *coeff,
		  const wfa_t *wfa, const coding_t *c)
/*
 *  Find an approximation of the current 'range' with a linear
 *  combination of vectors of the 'domain_pool'. The linear
 *  combination is generated step by step with the matching pursuit
 *  algorithm.  If flag 'full_search' is set then compute complete set
 *  of linear combinations with n = {0, ..., 'max_edges'} vectors and
 *  return the best one. Otherwise abort the computation as soon as
 *  costs (LC (n + 1)) exceed costs ( LC (n)) and return the
 *  sub-optimal solution.  'price' is the langrange multiplier
 *  weighting rate and distortion.  'band' is the current color band
 *  and 'y_state' the corresponding state in the Y component at same
 *  pixel position.  'domain_pool' gives the set of available vectors,
 *  and 'coeff' the model for the linear factors. The number of
 *  elements in the linear combination is limited by 'max_edges'. In
 *  'mp', vectors may be specified which should be excluded during the
 *  approximation.
 *  
 *  No return value.
 *
 *  Side effects:
 *	vectors, factors, rate, distortion and costs are stored in 'mp'
 */
{
   unsigned	 n;			/* current vector of the OB */
   int		 index;			/* best fitting domain image */
   unsigned	 domain;		/* counter */
   real_t	 norm;			/* norm of range image */
   real_t	 additional_bits;	/* bits for mc, nd, and tree */
   word_t	*domain_blocks;		/* current set of domain images */
   const real_t  min_norm = 2e-3;	/* lower bound of norm */
   unsigned 	 best_n   = 0;
   unsigned	 size 	  = size_of_level (range->level);
 
   /*
    *  Initialize domain pool and inner product arrays
    */
   domain_blocks = domain_pool->generate (range->level, y_state, wfa,
					  domain_pool->model);
   for (domain = 0; domain_blocks [domain] >= 0; domain++)
   {
      used [domain] = NO;
      rem_denominator [domain]		/* norm of domain */
	 = get_ip_state_state (domain_blocks [domain], domain_blocks [domain],
			       range->level, c);
      if (rem_denominator [domain] / size < min_norm)
	 used [domain] = YES;		/* don't use domains with small norm */
      else
	 rem_numerator [domain]		/* inner product <s_domain, b> */
	    = get_ip_image_state (range->image, range->address,
				  range->level, domain_blocks [domain], c);
      if (!used [domain] && fabs (rem_numerator [domain]) < min_norm)
	 used [domain] = YES;
   }

   /*
    *  Exclude all domain blocks given in array 'mp->exclude'
    */
   for (n = 0; isdomain (mp->exclude [n]); n++)
      used [mp->exclude [n]] = YES;

   /*
    *  Compute the approximation costs if 'range' is approximated with
    *  no linear combination, i.e. the error is equal to the square
    *  of the image norm and the size of the automaton is determined by
    *  storing only zero elements in the current matrix row
    */
   for (norm = 0, n = 0; n < size; n++)
      norm += square (c->pixels [range->address * size + n]);

   additional_bits = range->tree_bits + range->mv_tree_bits
		     + range->mv_coord_bits + range->nd_tree_bits
		     + range->nd_weights_bits;

   mp->err          = norm;
   mp->weights_bits = 0;
   mp->matrix_bits  = domain_pool->bits (domain_blocks, NULL, range->level,
					 y_state, wfa, domain_pool->model);
   mp->costs        = (mp->matrix_bits + mp->weights_bits
		       + additional_bits) * price + mp->err;

   n = 0;
   do 
   {
      /*
       *  Current approximation is: b = d_0 o_0 + ... + d_(n-1) o_(n-1)
       *  with corresponding costs 'range->err + range->bits * p'.
       *  For all remaining state images s_i (used[s_i] == NO) set
       *  o_n :	= s_i - \sum(k = 0, ... , n-1) {(<s_i, o_k> / ||o_k||^2) o_k}
       *  and try to beat current costs.
       *  Choose that vector for the next orthogonalization step,
       *  which has minimal costs: s_index.
       *  (No progress is indicated by index == -1)
       */
      
      real_t min_matrix_bits  = 0;
      real_t min_weights_bits = 0;
      real_t min_error 	      = 0;
      real_t min_weight [MAXEDGES];
      real_t min_costs = full_search ? MAXCOSTS : mp->costs;
      
      for (index = -1, domain = 0; domain_blocks [domain] >= 0; domain++) 
	 if (!used [domain]) 
	 {
	    real_t    matrix_bits, weights_bits;
	    /*
	     *  To speed up the search through the domain images,
	     *  the costs of using domain image 'domain' as next vector
	     *  can be approximated in a first step:
	     *  improvement of image quality
	     *	  <= square (rem_numerator[domain]) / rem_denominator[domain]
	     */
	    {
		  word_t   vectors [MAXEDGES + 1];
		  word_t   states [MAXEDGES + 1];
		  real_t   weights [MAXEDGES + 1];
		  unsigned i, k;
		  
		  for (i = 0, k = 0; k < n; k++)
		     if (mp->weight [k] != 0)
		     {
			vectors [i] = mp->indices [k];
			states [i]  = domain_blocks [vectors [i]];
			weights [i] = mp->weight [k];
			i++;
		     }
		  vectors [i] 	  = domain;
		  states [i]  	  = domain_blocks [domain];
		  weights [i] 	  = 0.5;
		  vectors [i + 1] = -1;
		  states [i + 1]  = -1;

		  weights_bits = coeff->bits (weights, states, range->level,
					      coeff);
		  matrix_bits = domain_pool->bits (domain_blocks, vectors,
						   range->level, y_state,
						   wfa, domain_pool->model);
	    }
	    if (((matrix_bits + weights_bits + additional_bits) * price +
		 mp->err -
		 square (rem_numerator [domain]) / rem_denominator [domain])
		< min_costs)
	    {
	       /*
		*  1.) Compute the weights (linear factors) c_i of the
		*  linear combination
		*  b = c_0 v_0 + ... + c_(n-1) v_(n-1) + c_n v_'domain'
		*  Use backward substitution to obtain c_i from the linear
		*  factors of the lin. comb. b = d_0 o_0 + ... + d_n o_n
		*  of the corresponding orthogonal vectors {o_0, ..., o_n}.
		*  Vector o_n of the orthogonal basis is obtained by using
		*  vector 'v_domain' in step n of the Gram Schmidt
		*  orthogonalization (see above for definition of o_n).
		*  Recursive formula for the coefficients c_i:
		*  c_n := <b, o_n> / ||o_n||^2
		*  for i = n - 1, ... , 0:
		*  c_i := <b, o_i> / ||o_i||^2 +
		*          \sum (k = i + 1, ... , n){ c_k <v_k, o_i>
		*					/ ||o_i||^2 }
		*  2.) Because linear factors are stored with reduced precision
		*  factor c_i is rounded with the given precision in step i
		*  of the recursive formula. 
		*/

	       unsigned k;		/* counter */
	       int    	l;		/* counter */
	       real_t 	m_bits;		/* number of matrix bits to store */
	       real_t 	w_bits;		/* number of weights bits to store */
	       real_t 	r [MAXEDGES];	/* rounded linear factors */
	       real_t 	f [MAXEDGES];	/* linear factors */
	       int    	v [MAXEDGES];	/* mapping of domains to vectors */
	       real_t 	costs;		/* current approximation costs */
	       real_t 	m_err;		/* current approximation error */

	       f [n] = rem_numerator [domain] / rem_denominator [domain];
	       v [n] = domain;		/* corresponding mapping */
	       for (k = 0; k < n; k++)
	       {
		  f [k] = ip_image_ortho_vector [k] / norm_ortho_vector [k];
		  v [k] = mp->indices [k];
	       }
	    
	       for (l = n; l >= 0; l--) 
	       {
		  rpf_t *rpf = domain_blocks [v [l]]
			       ? coeff->rpf : coeff->dc_rpf;

		  r [l] = f [l] = btor (rtob (f [l], rpf), rpf);
		     
		  for (k = 0; k < (unsigned) l; k++)
		     f [k] -= f [l] * ip_domain_ortho_vector [v [l]][k]
			      / norm_ortho_vector [k] ;
	       } 

	       /*
		*  Compute the number of output bits of the linear combination
		*  and store the weights with reduced precision. The
		*  resulting linear combination is
		*  b = r_0 v_0 + ... + r_(n-1) v_(n-1) + r_n v_'domain'
		*/
	       {
		  word_t vectors [MAXEDGES + 1];
		  word_t states [MAXEDGES + 1];
		  real_t weights [MAXEDGES + 1];
		  int	 i;
		  
		  for (i = 0, k = 0; k <= n; k++)
		     if (f [k] != 0)
		     {
			vectors [i] = v [k];
			states [i]  = domain_blocks [v [k]];
			weights [i] = f [k];
			i++;
		     }
		  vectors [i] = -1;
		  states [i]  = -1;

		  w_bits = coeff->bits (weights, states, range->level, coeff);
		  m_bits = domain_pool->bits (domain_blocks, vectors,
					      range->level, y_state,
					      wfa, domain_pool->model);
	       }
	       
	       /*
		*  To compute the approximation error, the corresponding
		*  linear factors of the linear combination 
		*  b = r_0 o_0 + ... + r_(n-1) o_(n-1) + r_n o_'domain'
		*  with orthogonal vectors must be computed with following
		*  formula:
		*  r_i := r_i +
		*          \sum (k = i + 1, ... , n) { r_k <v_k, o_i>
		*					/ ||o_i||^2 }
		*/
	       for (l = 0; (unsigned) l <= n; l++)
	       {
		  /*
		   *  compute <v_n, o_n>
		   */
		  real_t a;

		  a = get_ip_state_state (domain_blocks [v [l]],
					  domain_blocks [domain],
					  range->level, c);
		  for (k = 0; k < n; k++) 
		     a -= ip_domain_ortho_vector [v [l]][k]
			  / norm_ortho_vector [k]
			  * ip_domain_ortho_vector [domain][k];
		  ip_domain_ortho_vector [v [l]][n] = a;
	       }
	       norm_ortho_vector [n]     = rem_denominator [domain];
	       ip_image_ortho_vector [n] = rem_numerator [domain];
 	    
	       for (k = 0; k <= n; k++)
		  for (l = k + 1; (unsigned) l <= n; l++)
		     r [k] += ip_domain_ortho_vector [v [l]][k] * r [l]
			      / norm_ortho_vector [k];
	       /*
		*  Compute approximation error:
		*  error := ||b||^2 +
		*  \sum (k = 0, ... , n){r_k^2 ||o_k||^2 - 2 r_k <b, o_k>}
		*/
	       m_err = norm;
	       for (k = 0; k <= n; k++)
		  m_err += square (r [k]) * norm_ortho_vector [k]
			 - 2 * r [k] * ip_image_ortho_vector [k];
	       if (m_err < 0)		/* TODO: return MAXCOSTS */
		  warning ("Negative image norm: %f"
			   " (current domain: %d, level = %d)",
			   (double) m_err, domain, range->level);

	       costs = (m_bits + w_bits + additional_bits) * price + m_err;
	       if (costs < min_costs)	/* found a better approximation */
	       {
		  index            = domain;
		  min_costs        = costs;
		  min_matrix_bits  = m_bits;
		  min_weights_bits = w_bits;
		  min_error        = m_err;
		  for (k = 0; k <= n; k++)
		     min_weight [k] = f [k];
	       }
	    }
	 }
      
      if (index >= 0)			/* found a better approximation */
      {
	 if (min_costs < mp->costs)
	 {
	    unsigned k;
	    
	    mp->costs        = min_costs;
	    mp->err          = min_error;
	    mp->matrix_bits  = min_matrix_bits;
	    mp->weights_bits = min_weights_bits;
	    
	    for (k = 0; k <= n; k++)
	       mp->weight [k] = min_weight [k];

	    best_n = n + 1;
	 }
	 
	 mp->indices [n] = index;
	 mp->into [n]    = domain_blocks [index];

	 used [index] = YES;

	 /* 
	  *  Gram-Schmidt orthogonalization step n 
	  */
	 orthogonalize (index, n, range->level, min_norm, domain_blocks, c);
	 n++;
      }	
   } 
   while (n < max_edges && index >= 0);

   mp->indices [best_n] = NO_EDGE;
   
   mp->costs = (mp->matrix_bits + mp->weights_bits + additional_bits) * price
	       + mp->err;

   Free (domain_blocks);
}

static void
orthogonalize (unsigned index, unsigned n, unsigned level, real_t min_norm,
	       const word_t *domain_blocks, const coding_t *c)
/*
 *  Step 'n' of the Gram-Schmidt orthogonalization procedure:
 *  vector 'index' is orthogonalized with respect to the set
 *  {u_[0], ... , u_['n' - 1]}. The size of the image blocks is given by
 *  'level'. If the denominator gets smaller than 'min_norm' then
 *  the corresponding domain is excluded from the list of available
 *  domain blocks.
 *
 *  No return value.
 *
 *  Side effects:
 *	The remainder values (numerator and denominator) of
 *	all 'domain_blocks' are updated. 
 */
{
   unsigned domain;
   
   ip_image_ortho_vector [n] = rem_numerator [index];
   norm_ortho_vector [n]     = rem_denominator [index];

   /*
    *  Compute inner products between all domain images and 
    *  vector n of the orthogonal basis:
    *  for (i = 0, ... , wfa->states)  
    *  <s_i, o_n> := <s_i, v_n> -
    *      \sum (k = 0, ... , n - 1){ <v_n, o_k> <s_i, o_k> / ||o_k||^2}
    *  Moreover the denominator and numerator parts of the comparitive
    *  value are updated.
    */
   for (domain = 0; domain_blocks [domain] >= 0; domain++) 
      if (!used [domain]) 
      {
	 unsigned k;
	 real_t   tmp = get_ip_state_state (domain_blocks [index],
					    domain_blocks [domain], level, c);
	 
	 for (k = 0; k < n; k++) 
	    tmp -= ip_domain_ortho_vector [domain][k] / norm_ortho_vector [k]
		   * ip_domain_ortho_vector [index][k];
	 ip_domain_ortho_vector [domain][n] = tmp;
	 rem_denominator [domain] -= square (tmp) / norm_ortho_vector [n];
	 rem_numerator [domain]   -= ip_image_ortho_vector [n]
				     / norm_ortho_vector [n]
				     * ip_domain_ortho_vector [domain][n] ;

	 /*
	  *  Exclude vectors with small denominator
	  */
	 if (!used [domain]) 
	    if (rem_denominator [domain] / size_of_level (level) < min_norm) 
	       used [domain] = YES;
      }
}



