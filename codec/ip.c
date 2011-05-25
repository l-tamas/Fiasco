/*
 *  ip.c:		Computation of inner products
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

#include "types.h"
#include "macros.h"
#include "error.h"

#include "cwfa.h"
#include "control.h"
#include "ip.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static real_t 
standard_ip_image_state (unsigned address, unsigned level, unsigned domain,
			 const coding_t *c);
static real_t 
standard_ip_state_state (unsigned domain1, unsigned domain2, unsigned level,
			 const coding_t *c);

/*****************************************************************************

				public code
  
*****************************************************************************/

real_t 
get_ip_image_state (unsigned image, unsigned address, unsigned level,
		    unsigned domain, const coding_t *c)
/*
 *  Return value:
 *	Inner product between 'image' ('address') and
 *      'domain' at given 'level' 
 */
{
   if (level <= c->options.images_level)
   {
      /*
       *  Compute the inner product in the standard way by multiplying 
       *  the pixel-values of the given domain and range image.
       */ 
      return standard_ip_image_state (address, level, domain, c);
   }
   else 
   {
      /*
       *  Use the already computed inner products stored in 'ip_images_states'
       */
      return c->ip_images_state [domain][image];
   }
}

void 
compute_ip_images_state (unsigned image, unsigned address, unsigned level,
			 unsigned n, unsigned from,
			 const wfa_t *wfa, coding_t *c)
/*
 *  Compute the inner products between all states
 *  'from', ... , 'wfa->max_states' and the range images 'image'
 *  (and childs) up to given level.
 *
 *  No return value.
 *
 *  Side effects:
 *	inner product tables 'c->ip_images_states' are updated
 */ 
{
   if (level > c->options.images_level) 
   {
      unsigned state, label;

      if (level > c->options.images_level + 1)	/* recursive computation */
	 compute_ip_images_state (MAXLABELS * image + 1, address * MAXLABELS,
				  level - 1, MAXLABELS * n, from, wfa, c);
      
      /*
       *  Compute inner product <f, Phi_i>
       */
      for (label = 0; label < MAXLABELS; label++)
	 for (state = from; state < wfa->states; state++)
	    if (need_image (state, wfa))
	    {
	       unsigned  edge, count;
	       int     	 domain;
	       real_t 	*dst, *src;
	       
	       if (ischild (domain = wfa->tree [state][label]))
	       {
		  if (level > c->options.images_level + 1)
		  {
		     dst = c->ip_images_state [state] + image;
		     src = c->ip_images_state [domain]
			   + image * MAXLABELS + label + 1;
		     for (count = n; count; count--, src += MAXLABELS)
			*dst++ += *src;
		  }
		  else
		  {
		     unsigned newadr = address * MAXLABELS + label;
		     
		     dst = c->ip_images_state [state] + image;
		     
		     for (count = n; count; count--, newadr += MAXLABELS)
			*dst++ += standard_ip_image_state (newadr, level - 1,
							   domain, c);
		  }
	       }
	       for (edge = 0; isedge (domain = wfa->into [state][label][edge]);
		    edge++)
	       {
		  real_t weight = wfa->weight [state][label][edge];
		  
		  if (level > c->options.images_level + 1)
		  {
		     dst = c->ip_images_state [state] + image;
		     src = c->ip_images_state [domain]
			   + image * MAXLABELS + label + 1;
		     for (count = n; count; count--, src += MAXLABELS)
			*dst++ += *src * weight;
		  }
		  else
		  {
		     unsigned newadr = address * MAXLABELS + label;

		     dst = c->ip_images_state [state] + image;
		     
		     for (count = n; count; count--, newadr += MAXLABELS)
			*dst++ += weight *
				  standard_ip_image_state (newadr, level - 1,
							   domain, c);
		  }
	       }
	    }
   }
}

real_t 
get_ip_state_state (unsigned domain1, unsigned domain2, unsigned level,
		    const coding_t *c)
/*
 *  Return value:
 *	Inner product between 'domain1' and 'domain2' at given 'level'.
 */
{
   if (level <= c->options.images_level)
   {
      /*
       *  Compute the inner product in the standard way by multiplying 
       *  the pixel-values of both state-images
       */ 
      return standard_ip_state_state (domain1, domain2, level, c);
   }
   else 
   {
      /*
       *  Use already computed inner products stored in 'ip_images_states'
       */
      if (domain2 < domain1)
	 return c->ip_states_state [domain1][level][domain2];
      else
	 return c->ip_states_state [domain2][level][domain1];
   }
}

void 
compute_ip_states_state (unsigned from, unsigned to,
			 const wfa_t *wfa, coding_t *c)
/*
 *  Computes the inner products between the current state 'state1' and the
 *  old states 0,...,'state1'-1
 *
 *  No return value.
 *
 *  Side effects:
 *	inner product tables 'c->ip_states_state' are computed.
 */ 
{
   unsigned level;
   unsigned state1, state2;

   /*
    *  Compute inner product <Phi_state1, Phi_state2>
    */

   for (level = c->options.images_level + 1;
	level <= c->options.lc_max_level; level++)
      for (state1 = from; state1 <= to; state1++)
	 for (state2 = 0; state2 <= state1; state2++) 
	    if (need_image (state2, wfa))
	    {
	       unsigned	label;
	       real_t	ip = 0;
	       
	       for (label = 0; label < MAXLABELS; label++)
	       {
		  int	   domain1, domain2;
		  unsigned edge1, edge2;
		  real_t   sum, weight2;
		  
		  if (ischild (domain1 = wfa->tree [state1][label]))
		  {
		     sum = 0;
		     if (ischild (domain2 = wfa->tree [state2][label]))
			sum = get_ip_state_state (domain1, domain2,
						  level - 1, c);
		     
		     for (edge2 = 0;
			  isedge (domain2 = wfa->into [state2][label][edge2]);
			  edge2++)
		     {
			weight2 = wfa->weight [state2][label][edge2];
			sum += weight2 * get_ip_state_state (domain1, domain2,
							     level - 1, c);
		     }
		     ip += sum;
		  }
		  for (edge1 = 0;
		       isedge (domain1 = wfa->into [state1][label][edge1]);
		       edge1++)
		  {
		     real_t weight1 = wfa->weight [state1][label][edge1];
		     
		     sum = 0;
		     if (ischild (domain2 = wfa->tree [state2][label]))
			sum = get_ip_state_state (domain1, domain2,
						  level - 1, c);
		     
		     for (edge2 = 0;
			  isedge (domain2 = wfa->into [state2][label][edge2]);
			  edge2++)
		     {
			weight2 = wfa->weight [state2][label][edge2];
			sum += weight2 * get_ip_state_state (domain1, domain2,
							     level - 1, c);
		     }
		     ip += weight1 * sum;
		  }
	       }
	       c->ip_states_state [state1][level][state2] = ip;
	    }
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static real_t 
standard_ip_image_state (unsigned address, unsigned level, unsigned domain,
			 const coding_t *c)
/*
 *  Returns the inner product between the subimage 'address' and the
 *  state image 'domain' at given 'level'.  The stored state images
 *  and the image tree are used to compute the inner product in the
 *  standard way by multiplying the corresponding pixel values.
 *
 *  Return value:
 *	computed inner product
 */
{
   unsigned i;
   real_t   ip = 0, *imageptr, *stateptr;

   if (level > c->options.images_level)
      error ("Level %d not supported.", level);
   
   imageptr = &c->pixels [address * size_of_level (level)];

   stateptr = c->images_of_state [domain] + address_of_level (level);
   
   for (i = size_of_level (level); i; i--)
      ip += *imageptr++ * *stateptr++;

   return ip;
}

static real_t 
standard_ip_state_state (unsigned domain1, unsigned domain2, unsigned level,
			 const coding_t *c)
/*
 *  Returns the inner product between the subimage 'address' and the
 *  state image 'state' at given 'level'.  The stored state images are
 *  used to compute the inner product in the standard way by
 *  multiplying the corresponding pixel values.
 *
 *  Return value:
 *	computed inner product
 */
{
   unsigned i;
   real_t   ip = 0, *state1ptr, *state2ptr;

   if (level > c->options.images_level)
      error ("Level %d not supported.", level);

   state1ptr = c->images_of_state [domain1] + address_of_level (level);
   state2ptr = c->images_of_state [domain2] + address_of_level (level);
   
   for (i = size_of_level (level); i; i--)
      ip += *state1ptr++ * *state2ptr++;

   return ip;
}

