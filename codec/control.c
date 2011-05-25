/*
 *  control.c:		Control unit of WFA structure
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

#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */
 
#include "types.h"
#include "macros.h"
#include "error.h"

#include "cwfa.h"
#include "ip.h"
#include "misc.h"
#include "wfalib.h"
#include "control.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void 
clear_or_alloc (real_t **ptr, size_t size);
static void 
compute_images (unsigned from, unsigned to, const wfa_t *wfa, coding_t *c);

/*****************************************************************************

				public code
  
*****************************************************************************/

void    
append_state (bool_t auxiliary_state, real_t final, unsigned level_of_state,
	      wfa_t *wfa, coding_t *c)
/*
 *  Append a 'wfa' state. If 'auxiliary_state' == YES then
 *  allocate memory for inner products and state images.  'final' is
 *  the final distribution of the new state. 'level_of_state' is the
 *  level of the subimage which is represented by this state
 *
 *  No return value.
 *
 *  Side effects:
 *	The WFA information are updated in structure 'wfa'
 *	State images are computed and inner products are cleared (in 'c')
 */
{
   wfa->final_distribution [wfa->states] = final;
   wfa->level_of_state [wfa->states]     = level_of_state;
   
   if (!auxiliary_state)
   {
      unsigned level;

      wfa->domain_type [wfa->states] = USE_DOMAIN_MASK;

      /*
       *  Allocate memory for inner products and for state images
       */
      clear_or_alloc (&c->images_of_state [wfa->states],
		      size_of_tree (c->options.images_level));
	 
      for (level = c->options.images_level + 1;
	   level <= c->options.lc_max_level; level++)
	 clear_or_alloc (&c->ip_states_state [wfa->states][level],
			 wfa->states + 1);

      clear_or_alloc (&c->ip_images_state [wfa->states],
		      size_of_tree (c->products_level));

      /*
       *  Compute the images of the current state at level 0,..,'imageslevel'
       */
      
      c->images_of_state [wfa->states][0] = final;
      compute_images (wfa->states, wfa->states, wfa, c);  

      /*
       *  Compute the inner products between the current state and the
       *  old states 0,...,'states'-1
       */ 
      
      compute_ip_states_state (wfa->states, wfa->states, wfa, c);
   }
   else
   {
      unsigned level;
      
      wfa->domain_type [wfa->states] = 0;
	    
      /*
       *  Free the allocated memory
       */
      if (c->images_of_state [wfa->states] != NULL)
      {
	 Free (c->images_of_state [wfa->states]);
	 c->images_of_state [wfa->states] = NULL;
      }
      for (level = 0; level <= c->options.lc_max_level; level++)
	 if (c->ip_states_state [wfa->states][level])
	 {
	    Free (c->ip_states_state [wfa->states][level]);
	    c->ip_states_state [wfa->states][level] = NULL;
	 }
      if (c->ip_images_state [wfa->states])
      {
	 Free (c->ip_images_state [wfa->states]);
	 c->ip_images_state [wfa->states] = NULL;
      }
   }
   
   wfa->states++;
   if (wfa->states >= MAXSTATES) 
      error ("Maximum number of states reached!");
}	
 
void 
append_basis_states (unsigned basis_states, wfa_t *wfa, coding_t *c)
/*
 *  Append the WFA basis states 0, ... , ('basis_states' - 1).
 *
 *  No return value.
 *
 *  Side effects:
 *	The WFA information are updated in structure 'wfa'
 *	State images and inner products are computed (in 'c')
 */
{
   unsigned level, state;

   /*
    *  Allocate memory
    */

   for (state = 0; state < basis_states; state++)
   {
      clear_or_alloc (&c->images_of_state [state],
		      size_of_tree (c->options.images_level));

      for (level = c->options.images_level + 1;
	   level <= c->options.lc_max_level; level++)
	 clear_or_alloc (&c->ip_states_state [state][level], state + 1);

      clear_or_alloc (&c->ip_images_state [state],
		      size_of_tree (c->products_level));

      c->images_of_state [state][0] = wfa->final_distribution [state];
      wfa->level_of_state [state]   = -1;
   }
   
   compute_images (0, basis_states - 1, wfa, c);  
   compute_ip_states_state (0, basis_states - 1, wfa, c);
   wfa->states = basis_states;
   
   if (wfa->states >= MAXSTATES) 
      error ("Maximum number of states reached!");
}	
 
void 
append_transitions (unsigned state, unsigned label, const real_t *weight,
		    const word_t *into, wfa_t *wfa)
/*
 *  Append the 'wfa' transitions (given by the arrays 'weight' and 'into')
 *  of the range ('state','label').
 *
 *  No return value.
 *
 *  Side effects:
 *	new 'wfa' edges are appended
 */
{
   unsigned edge;

   wfa->y_column [state][label] = 0;
   for (edge = 0; isedge (into [edge]); edge++)
   {
      append_edge (state, into [edge], weight [edge], label, wfa);
      if (into [edge] == wfa->y_state [state][label])
	 wfa->y_column [state][label] = 1;
   }
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void 
compute_images (unsigned from, unsigned to, const wfa_t *wfa, coding_t *c)
/*
 *  Computes the images of the given states 'from', ... , 'to' 
 *  at level 0,...,'c->imagelevel'.
 *  Uses the fact that each state image is a linear combination of state
 *  images, i.e. s_i := c_0 s_0 + ... + c_i s_i.
 */
{
   unsigned label, level, state;
   
   /*
    *  Compute the images Phi(state)
    *  #		level = 0
    *  ##		level = 1
    *  ####		level = 2
    *  ########    	level = 3
    *  ...
    *  ########...##    level = imageslevel
    */
   
   for (level = 1; level <= c->options.images_level; level++)
      for (state = from; state <= to; state++)
	 for (label = 0; label < MAXLABELS; label++)
	 {
	    real_t   *dst, *src;
	    unsigned  edge;
	    int	      domain;		/* current domain */
	    
	    if (ischild (domain = wfa->tree[state][label]))
	    {
	       dst = c->images_of_state [state] + address_of_level (level) +
		     label * size_of_level (level - 1);
	       src = c->images_of_state [domain]
		     + address_of_level (level - 1);
	       memcpy (dst, src, size_of_level (level - 1) * sizeof (real_t));
	    }
	    for (edge = 0; isedge (domain = wfa->into[state][label][edge]);
		 edge++)
	    {
	       unsigned n;
	       real_t 	weight = wfa->weight [state][label][edge];
	       
	       dst = c->images_of_state [state] + address_of_level (level) +
		     label * size_of_level (level - 1);
	       src = c->images_of_state [domain]
		     + address_of_level (level - 1);
		  
	       for (n = size_of_level (level - 1); n; n--)
		  *dst++ += *src++ * weight;
	    }
	 }

}

static void 
clear_or_alloc (real_t **ptr, size_t size)
/*
 *  if *ptr == NULL 	allocate memory with Calloc 
 *  otherwise 		fill the real_t-array ptr[] with 0
 */
{
   if (*ptr == NULL) 
      *ptr = Calloc (size, sizeof (real_t));
   else 
      memset (*ptr, 0, size * sizeof (real_t));
    
}
