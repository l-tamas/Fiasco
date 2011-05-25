/*
 *  prediction.c:	Range image prediction with MC or ND	
 *
 *  Written by:		Ullrich Hafner
 *			Michael Unger
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
#include "control.h"
#include "misc.h"
#include "subdivide.h"
#include "bintree.h"
#include "domain-pool.h"
#include "approx.h"
#include "wfalib.h"
#include "mwfa.h"
#include "prediction.h"

#include "decoder.h"


/*****************************************************************************

			     local variables
  
*****************************************************************************/

typedef struct state_data
{
   real_t final_distribution;
   byte_t level_of_state;
   byte_t domain_type;

   real_t *images_of_state;
   real_t *inner_products;
   real_t *ip_states_state [MAXLEVEL];

   word_t tree [MAXLABELS];
   mv_t	  mv_tree [MAXLABELS];
   word_t y_state [MAXLABELS];
   byte_t y_column [MAXLABELS];
   byte_t prediction [MAXLABELS];

   u_word_t x [MAXLABELS];
   u_word_t y [MAXLABELS];

   real_t weight [MAXLABELS][MAXEDGES + 1];
   word_t int_weight [MAXLABELS][MAXEDGES + 1];
   word_t into [MAXLABELS][MAXEDGES + 1];
} state_data_t;

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static real_t
nd_prediction (real_t max_costs, real_t price, unsigned band, int y_state,
	       range_t *range, wfa_t *wfa, coding_t *c);
static real_t
mc_prediction (real_t max_costs, real_t price, unsigned band, int y_state,
	       range_t *range, wfa_t *wfa, coding_t *c);
static state_data_t *
store_state_data (unsigned from, unsigned to, unsigned max_level,
		  wfa_t *wfa, coding_t *c);
static void
restore_state_data (unsigned from, unsigned to, unsigned max_level,
		    state_data_t *data, wfa_t *wfa, coding_t *c);

/*****************************************************************************

				public code
  
*****************************************************************************/
 
real_t
predict_range (real_t max_costs, real_t price, range_t *range, wfa_t *wfa,
	       coding_t *c, unsigned band, int y_state, unsigned states,
	       const tree_t *tree_model, const tree_t *p_tree_model,
	       const void *domain_model, const void *d_domain_model,
	       const void *coeff_model, const void *d_coeff_model)
{
   unsigned	 state;		     	/* counter */
   void		*rec_domain_model;   	/* domain model after recursion */
   void		*rec_d_domain_model; 	/* p domain model after recursion */
   void		*rec_coeff_model;    	/* coeff model after recursion */
   void		*rec_d_coeff_model;  	/* d coeff model after recursion */
   tree_t	 rec_tree_model;	/* tree_model after '' */
   tree_t	 rec_p_tree_model;    	/* p_tree_model after '' */
   unsigned	 rec_states;	     	/* wfa->states after '' */
   real_t	*rec_pixels;	     	/* c->pixels after '' */
   state_data_t	*rec_state_data;     	/* state_data struct after '' */
   real_t	 costs;		     	/* current approximation costs */
   unsigned	 level;		     	/* counter */
   state_data_t	*sd;		     	/* pointer to state_data field */

   /*
    *  Store WFA data from state 'lc_states' to 'wfa->states' - 1 and
    *  current state of probability models.
    */
   rec_domain_model   = c->domain_pool->model;
   rec_d_domain_model = c->d_domain_pool->model;
   rec_coeff_model    = c->coeff->model;
   rec_d_coeff_model  = c->d_coeff->model;
   rec_tree_model     = c->tree;
   rec_p_tree_model   = c->p_tree;
   rec_states         = wfa->states;	
   rec_pixels         = c->pixels;
   rec_state_data     = store_state_data (states, rec_states - 1,
					  c->options.lc_max_level, wfa, c);
   
   /*
    *  Restore probability models to the state before the recursive subdivision
    *  has been started.
    */
   wfa->states             = states;
   c->tree                 = *tree_model;
   c->p_tree               = *p_tree_model;
   c->domain_pool->model   = c->domain_pool->model_duplicate (domain_model);
   c->d_domain_pool->model = c->d_domain_pool->model_duplicate (d_domain_model);
   c->coeff->model   	   = c->coeff->model_duplicate (c->coeff, coeff_model);
   c->d_coeff->model   	   = c->d_coeff->model_duplicate (c->d_coeff,
							  d_coeff_model);
   
   if (c->mt->frame_type == I_FRAME)
      costs = nd_prediction (max_costs, price, band, y_state, range, wfa, c); 
   else
      costs = mc_prediction (max_costs, price, band, y_state, range, wfa, c);
   
   c->pixels = rec_pixels;
   
   if (costs < MAXCOSTS)
   {
      /*
       *  Free the memory used by the state_data struct
       */
      for (state = states; state < rec_states; state++)
      {
	 sd = &rec_state_data [state - states];
	 for (level = c->options.images_level + 1;
	      level <= c->options.lc_max_level; level++)
	    if (sd->ip_states_state [level] != NULL)
	       Free (sd->ip_states_state [level]);
	 if (sd->images_of_state != NULL)
	    Free (sd->images_of_state);
	 if (sd->inner_products != NULL)
	    Free (sd->inner_products);
      }
      if (states < rec_states)
	 Free (rec_state_data);
      c->domain_pool->model_free (rec_domain_model);
      c->d_domain_pool->model_free (rec_d_domain_model);
      c->coeff->model_free (rec_coeff_model);
      c->d_coeff->model_free (rec_d_coeff_model);

      costs = (range->tree_bits + range->matrix_bits + range->weights_bits
	       + range->mv_tree_bits + range->mv_coord_bits
	       + range->nd_tree_bits + range->nd_weights_bits) * price
	      + range->err;
   }
   else
   {
      /*
       *  Restore WFA to state before function was called
       */
      c->domain_pool->model_free (c->domain_pool->model);
      c->d_domain_pool->model_free (c->d_domain_pool->model);
      c->coeff->model_free (c->coeff->model);
      c->d_coeff->model_free (c->d_coeff->model);
      
      c->domain_pool->model   = rec_domain_model;
      c->d_domain_pool->model = rec_d_domain_model;
      c->coeff->model         = rec_coeff_model;
      c->d_coeff->model       = rec_d_coeff_model;
      c->tree                 = rec_tree_model;
      c->p_tree               = rec_p_tree_model;
      
      range->prediction = NO;
      
      if (wfa->states != states)
	 remove_states (states, wfa);
      restore_state_data (states, rec_states - 1, c->options.lc_max_level,
			  rec_state_data, wfa, c);
      costs = MAXCOSTS;
   }
 
   return costs;
} 

void
clear_norms_table (unsigned level, const wfa_info_t *wi, motion_t *mt)
/*
 *  Clear norms arrays.
 *
 *  No return value.
 */
{
   unsigned  range_size = wi->half_pixel
			  ? square (wi->search_range)
			  : square (2 * wi->search_range);

   if (level > wi->p_min_level)
   {
      memset (mt->mc_forward_norms [level], 0, range_size * sizeof(real_t));
      memset (mt->mc_backward_norms [level], 0, range_size * sizeof(real_t));
   }
}

void
update_norms_table (unsigned level, const wfa_info_t *wi, motion_t *mt)
/*
 *  Norms table of levels larger than the bottom level are computed
 *  by summing up previously calculated displacement costs of lower levels.
 *
 *  No return value.
 */
{
   unsigned  range_size = wi->half_pixel
			  ? square (wi->search_range)
			  : square (2 * wi->search_range);
   
   if (level > wi->p_min_level)
   {
      unsigned index;			/* index of motion vector */
      
      for (index = 0; index < range_size; index++)
	 mt->mc_forward_norms [level][index]
	    += mt->mc_forward_norms [level - 1][index];
      if (mt->frame_type == B_FRAME)
	 for (index = 0; index < range_size; index++)
	    mt->mc_backward_norms [level][index]
	       += mt->mc_backward_norms [level - 1][index];
   }
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static real_t
mc_prediction (real_t max_costs, real_t price, unsigned band, int y_state,
	       range_t *range, wfa_t *wfa, coding_t *c)
{
   real_t    costs;		   	/* current approximation costs */
   range_t   prange = *range;
   unsigned  width  = width_of_level (range->level);
   unsigned  height = height_of_level (range->level);
   word_t   *mcpe   = Calloc (width * height, sizeof (word_t));

   /*
    *  If we are at the bottom level of the mc tree:
    *  Fill in the norms table
    */
   if (prange.level == wfa->wfainfo->p_min_level) 
      fill_norms_table (prange.x, prange.y, prange.level, wfa->wfainfo, c->mt);
   /*
    *  Predict 'range' with motion compensation according to frame type.
    *  MCPE is returned in 'c->mcpe'
    */
   if (c->mt->frame_type == P_FRAME)
      find_P_frame_mc (mcpe, price, &prange, wfa->wfainfo, c->mt);
   else
      find_B_frame_mc (mcpe, price, &prange, wfa->wfainfo, c->mt);
   
   costs = (prange.mv_tree_bits + prange.mv_coord_bits) * price;
   
   if (costs < max_costs)		/* motion vector not too expensive */
   {
      unsigned  last_state;		/* last WFA state before recursion */
      real_t   *ipi [MAXSTATES];	/* inner products pointers */
      unsigned  state;
      real_t  	mvt, mvc;
      
      c->pixels = Calloc (width * height, sizeof (real_t));
      cut_to_bintree (c->pixels, mcpe, width, height, 0, 0, width, height);
   
      /*
       *  Approximate MCPE recursively.
       */
      last_state = wfa->states - 1;
      for (state = 0; state <= last_state; state++)
	 if (need_image (state, wfa))
	 {
	    ipi [state] = c->ip_images_state[state];
	    c->ip_images_state[state]
	       = Calloc (size_of_tree (c->products_level), sizeof (real_t));
	 }

      mvc = prange.mv_coord_bits;
      mvt = prange.mv_tree_bits;
      
      prange.image           = 0;
      prange.address         = 0;
      prange.tree_bits       = 0;
      prange.matrix_bits     = 0;
      prange.weights_bits    = 0;
      prange.mv_coord_bits   = 0;
      prange.mv_tree_bits    = 0;
      prange.nd_weights_bits = 0;
      prange.nd_tree_bits    = 0;

      compute_ip_images_state (prange.image, prange.address, prange.level,
			       1, 0, wfa, c);
      costs += subdivide (max_costs - costs, band, y_state, &prange,
			  wfa, c, NO, YES);

      if (costs < max_costs)		/* use motion compensation */
      {
	 unsigned img, adr;		/* temp. values */
	 
	 img                  = range->image;
	 adr                  = range->address;
	 *range               = prange;
	 range->image         = img;
	 range->address       = adr;
	 range->mv_coord_bits = mvc;
	 range->mv_tree_bits  = mvt;
	 range->prediction    = YES;

	 for (state = last_state + 1; state < wfa->states; state++)
	    if (need_image (state, wfa))
	       memset (c->ip_images_state [state], 0,
		       size_of_tree (c->products_level) * sizeof (real_t));

	 costs = (range->tree_bits + range->matrix_bits + range->weights_bits
		  + range->mv_tree_bits + range->mv_coord_bits
		  + range->nd_tree_bits + range->nd_weights_bits) * price
		 + range->err;
      }
      else
	 costs = MAXCOSTS;

      for (state = 0; state <= last_state; state++)
	 if (need_image (state, wfa))
	 {
	    Free (c->ip_images_state[state]);
	    c->ip_images_state[state] = ipi [state];
	 }
      Free (c->pixels);
   }
   else
      costs = MAXCOSTS;
   
   Free (mcpe);

   return costs;
}

static real_t
nd_prediction (real_t max_costs, real_t price, unsigned band, int y_state,
	       range_t *range, wfa_t *wfa, coding_t *c)
{
   real_t  costs;			/* current approximation costs */
   range_t lrange = *range;
   
   /*
    *  Predict 'range' with DC component approximation
    */
   {
      real_t x = get_ip_image_state (range->image, range->address,
				     range->level, 0, c);
      real_t y = get_ip_state_state (0, 0, range->level, c);
      real_t w = btor (rtob (x / y, c->coeff->dc_rpf), c->coeff->dc_rpf);
      word_t s [2] = {0, -1};

      lrange.into [0] 	     = 0;
      lrange.into [1] 	     = NO_EDGE;
      lrange.weight [0]      = w;
      lrange.mv_coord_bits   = 0;
      lrange.mv_tree_bits    = 0;
      lrange.nd_tree_bits    = tree_bits (LEAF, lrange.level, &c->p_tree);
      lrange.nd_weights_bits = 0;
      lrange.tree_bits       = 0;
      lrange.matrix_bits     = 0;
      lrange.weights_bits    = c->coeff->bits (&w, s, range->level, c->coeff);
   }
   costs = price * (lrange.weights_bits + lrange.nd_tree_bits);
   
   /*
    *  Recursive aproximation of difference image
    */
   if (costs < max_costs)		
   {
      unsigned  state;
      range_t  	rrange;			/* range: recursive subdivision */
      unsigned  last_state;		/* last WFA state before recursion */
      real_t   *ipi [MAXSTATES];	/* inner products pointers */
      unsigned 	width  = width_of_level (range->level);
      unsigned  height = height_of_level (range->level);
      real_t   *pixels;

      /*
       *  Generate difference image original - approximation
       */
      {
	 unsigned  n;
	 real_t *src, *dst;		/* pointers to image data */
	 real_t w = - lrange.weight [0] * c->images_of_state [0][0];
		     
	 src = c->pixels + range->address * size_of_level (range->level); 
	 dst = c->pixels = pixels = Calloc (width * height, sizeof (real_t));

	 for (n = width * height; n; n--)
	    *dst++ = *src++ + w;
      }
      
      /*
       *  Approximate difference recursively.
       */
      rrange                 = *range;
      rrange.tree_bits       = 0;
      rrange.matrix_bits     = 0;
      rrange.weights_bits    = 0;
      rrange.mv_coord_bits   = 0;
      rrange.mv_tree_bits    = 0;
      rrange.nd_tree_bits    = 0;
      rrange.nd_weights_bits = 0;
      rrange.image           = 0;
      rrange.address         = 0;

      last_state = wfa->states - 1;
      for (state = 0; state <= last_state; state++)
	 if (need_image (state, wfa))
	 {
	    ipi [state] = c->ip_images_state[state];
	    c->ip_images_state[state]
	       = Calloc (size_of_tree (c->products_level), sizeof (real_t));
	 }
      
      compute_ip_images_state (rrange.image, rrange.address, rrange.level,
			       1, 0, wfa, c);
      
      costs += subdivide (max_costs - costs, band, y_state, &rrange, wfa, c,
			  NO, YES);
      
      Free (pixels);

      if (costs < max_costs && ischild (rrange.tree)) /* use prediction */
      {
	 unsigned img, adr;
	 unsigned edge;

	 img                     = range->image;
	 adr                     = range->address;
	 *range                  = rrange;
	 range->image            = img;
	 range->address          = adr;
	 range->nd_tree_bits    += lrange.nd_tree_bits;
	 range->nd_weights_bits += lrange.weights_bits;
	 
	 for (edge = 0; isedge (lrange.into [edge]); edge++)
	 {
	    range->into [edge]   = lrange.into [edge];
	    range->weight [edge] = lrange.weight [edge];
	 }
	 range->into [edge] = NO_EDGE;
	 range->prediction  = edge;

	 for (state = last_state + 1; state < wfa->states; state++)
	    if (need_image (state, wfa))
	       memset (c->ip_images_state [state], 0,
		       size_of_tree (c->products_level) * sizeof (real_t));
      }
      else
	 costs = MAXCOSTS;
      
      for (state = 0; state <= last_state; state++)
	 if (need_image (state, wfa))
	 {
	    Free (c->ip_images_state [state]);
	    c->ip_images_state [state] = ipi [state];
	 }
   }
   else
      costs = MAXCOSTS;

   return costs;
}

static state_data_t *
store_state_data (unsigned from, unsigned to, unsigned max_level,
		  wfa_t *wfa, coding_t *c)
/*
 *  Save and remove all states starting from state 'from'.
 *
 *  Return value:
 *	pointer to array of state_data structs
 */
{
   state_data_t *data;			/* array of savestates */
   state_data_t *sd;			/* pointer to current savestates */
   unsigned	 state, label, level;

   if (to < from)
      return NULL;			/* nothing to do */
   
   data = Calloc (to - from + 1, sizeof (state_data_t));
   
   for (state = from; state <= to; state++)
   {
      sd = &data [state - from];

      sd->final_distribution = wfa->final_distribution [state];
      sd->level_of_state     = wfa->level_of_state [state];
      sd->domain_type        = wfa->domain_type [state];
      sd->images_of_state    = c->images_of_state [state];
      sd->inner_products     = c->ip_images_state [state];
      
      wfa->domain_type [state]   = 0;
      c->images_of_state [state] = NULL;
      c->ip_images_state [state] = NULL;
				   
      for (label = 0; label < MAXLABELS; label++) 
      {
	 sd->tree [label]     	= wfa->tree [state][label];
	 sd->y_state [label]  	= wfa->y_state [state][label];
	 sd->y_column [label] 	= wfa->y_column [state][label];
	 sd->mv_tree [label]  	= wfa->mv_tree [state][label];
	 sd->x [label]        	= wfa->x [state][label];
	 sd->y [label]        	= wfa->y [state][label];
	 sd->prediction [label] = wfa->prediction [state][label];

	 memcpy (sd->weight [label], wfa->weight [state][label], 
		 sizeof (real_t) * (MAXEDGES + 1));
	 memcpy (sd->int_weight [label], wfa->int_weight [state][label], 
		 sizeof (word_t) * (MAXEDGES + 1));
	 memcpy (sd->into [label], wfa->into [state][label], 
		 sizeof (word_t) * (MAXEDGES + 1));

	 wfa->into [state][label][0] = NO_EDGE;
	 wfa->tree [state][label]    = RANGE;
	 wfa->y_state [state][label] = RANGE;
      }
      for (level = c->options.images_level + 1; level <= max_level;
	   level++)
      {
	 sd->ip_states_state [level]       = c->ip_states_state [state][level];
	 c->ip_states_state [state][level] = NULL;
      }
   }

   return data;
}

static void
restore_state_data (unsigned from, unsigned to, unsigned max_level,
		    state_data_t *data, wfa_t *wfa, coding_t *c)
/*
 *  Restore all state data starting from state 'from'.
 *  
 *  No return value.
 */
{
   state_data_t *sd;			/* pointer to state_data item */
   unsigned	 state, label, level;

   if (to < from)
      return;				/* nothing to do */
   
   for (state = from; state <= to; state++)
   {
      sd = &data [state - from];
      
      wfa->final_distribution [state] = sd->final_distribution;
      wfa->level_of_state [state]     = sd->level_of_state;
      wfa->domain_type [state]        = sd->domain_type;
      
      if (c->images_of_state [state] != NULL)
	 Free (c->images_of_state [state]);
      c->images_of_state [state] = sd->images_of_state;
      if (c->ip_images_state [state] != NULL)
	 Free (c->ip_images_state [state]);
      c->ip_images_state [state] = sd->inner_products;

      for (label = 0; label < MAXLABELS; label++)
      {
	 wfa->tree [state][label]     	= sd->tree [label];
	 wfa->y_state [state][label]  	= sd->y_state [label];
	 wfa->y_column [state][label] 	= sd->y_column [label];
	 wfa->mv_tree [state][label]  	= sd->mv_tree [label];
	 wfa->x [state][label]        	= sd->x [label];
	 wfa->y [state][label]        	= sd->y [label];
	 wfa->prediction [state][label] = sd->prediction [label];
	 
	 memcpy (wfa->weight [state][label], sd->weight [label], 
		 sizeof(real_t) * (MAXEDGES + 1));
	 memcpy (wfa->int_weight [state][label], sd->int_weight [label], 
		 sizeof(word_t) * (MAXEDGES + 1));
	 memcpy (wfa->into [state][label], sd->into [label],  
		 sizeof(word_t) * (MAXEDGES + 1));
      }	 
      for (level = c->options.images_level + 1; level <= max_level;
	   level++)
      {
	 if (c->ip_states_state [state][level] != NULL)
	    Free (c->ip_states_state [state][level]);
	 c->ip_states_state [state][level] = sd->ip_states_state [level];
      }
   }

   Free (data);
   wfa->states = to + 1;
}
