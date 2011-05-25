/*
 *  subdivide.c:	Recursive subdivision of range images
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/15 17:59:31 $
 *  $Author: hafner $
 *  $Revision: 5.4 $
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

#include "image.h"
#include "cwfa.h"
#include "approx.h"
#include "ip.h"
#include "bintree.h"
#include "control.h"
#include "prediction.h"
#include "domain-pool.h"
#include "mwfa.h"
#include "misc.h"
#include "subdivide.h"
#include "list.h"
#include "coeff.h"
#include "wfalib.h"

/*****************************************************************************

				prototypes

*****************************************************************************/

static void
init_new_state (bool_t auxiliary_state, bool_t delta, range_t *range,
		const range_t *child, const int *y_state,
		wfa_t *wfa, coding_t *c);
static void
init_range (range_t *range, const image_t *image, unsigned band,
	    const wfa_t *wfa, coding_t *c);

/*****************************************************************************

				public code
  
*****************************************************************************/

real_t 
subdivide (real_t max_costs, unsigned band, int y_state, range_t *range,
	   wfa_t *wfa, coding_t *c, bool_t prediction, bool_t delta)
/*
 *  Subdivide the current 'range' recursively and decide whether
 *  a linear combination, a recursive subdivision, or a prediction is
 *  the best choice of approximation.
 *  'band' is the current color band, 'y_state' is the corresponding
 *  state of the Y color component (color image compression only).
 *  If 'prediction' is TRUE then also test motion compensation or
 *  nondeterministic approximation.
 *  If 'delta' is TRUE then current range is already predicted.
 *  
 *  Return value:
 *	costs of the best approximation or MAXCOSTS if costs exceed 'max_costs'
 *
 *  Side effects:
 *	'range'	factors and costs of linear combination are modified
 *      'wfa'	new transitions and prediction coefficients are added
 *	'c'	pixels and inner products are updated
 */
{
   real_t    subdivide_costs;        /* Costs arising from approx. the current
				       range with two childs */
   real_t    lincomb_costs;          /* Costs arising from approx. the current
				       range with a linear combination */
   int	     new_y_state [MAXLABELS];	/* Corresponding state of Y */
   real_t    price;			/* Approximation costs multiplier */
   bool_t    try_mc;			/* YES: try MC prediction */
   bool_t    try_nd;			/* YES: try ND prediction */
   unsigned  states;			/* Number of states before the
					   recursive subdivision starts */
   void     *domain_model;		/* copy of domain pool model */      
   void     *d_domain_model;		/* copy of delta domain pool model */
   void     *lc_domain_model;		/* copy of domain pool model */
   void     *lc_d_domain_model;		/* copy of delta domain pool model */
   void	    *coeff_model;	        /* copy of coefficients model */
   void	    *d_coeff_model;		/* copy of delta coefficients model */
   void	    *lc_coeff_model;	        /* copy of coefficients model */
   void	    *lc_d_coeff_model;		/* copy of delta coefficients model */
   tree_t    tree_model;		/* copy of tree model */
   tree_t    p_tree_model;		/* copy of pred. tree model */
   range_t   lrange;			/* range of lin. comb. approx. */
   range_t   rrange;			/* range of recursive approx. */
   range_t   child [MAXLABELS];		/* new childs of the current range */
   static unsigned percent = 0;		/* status of progress meter */

   if (wfa->wfainfo->level == range->level)
      percent = 0;
   
   range->into [0] = NO_EDGE;		/* default approximation: empty */
   range->tree     = RANGE;

   if (range->level < 3)		/* Don't process small ranges */
      return MAXCOSTS;	

   /*
    *  If image permutation (tiling) is performed and the tiling level
    *  is reached then get coordinates of the new block.
    */
   if (c->tiling->exponent
       && range->level == wfa->wfainfo->level - c->tiling->exponent)
   {
      unsigned width, height;		/* size of range (dummies)*/
      
      if (c->tiling->vorder [range->global_address] < 0)
	 return 0;			/* nothing to do */
      else
	 locate_subimage (wfa->wfainfo->level, range->level,
			  c->tiling->vorder [range->global_address],
			  &range->x, &range->y, &width, &height);
   }

   if (range->x >= c->mt->original->width ||
       range->y >= c->mt->original->height)
      return 0;				/* range is not visible */

   /*
    *  Check whether prediction is allowed or not
    *  mc == motion compensation, nd == nondeterminism
    */
   try_mc = (prediction && c->mt->frame_type != I_FRAME			
	     && range->level >= wfa->wfainfo->p_min_level
	     && range->level <= wfa->wfainfo->p_max_level
	     && (range->x + width_of_level (range->level)
		 <= c->mt->original->width)
	     && (range->y + height_of_level (range->level)
		 <= c->mt->original->height));

   try_nd = (prediction && c->mt->frame_type == I_FRAME
	     && range->level >= wfa->wfainfo->p_min_level
	     && range->level <= wfa->wfainfo->p_max_level);

   if (try_mc)
      clear_norms_table (range->level, wfa->wfainfo, c->mt);

   
   /*
    *  Check if current range must be initialized. I.e. range pixels must
    *  be copied from entire image to bintree pixel buffer. Moreover,
    *  all inner products tables must be initialized.
    */
   if (range->level == c->options.lc_max_level)	
      init_range (range, c->mt->original, band, wfa, c);
   
   price = c->price;
   if (band != Y)			
      price *= c->options.chroma_decrease; /* less quality for chroma bands */

   /*
    *  Compute childs of corresponding state in Y band
    */
   if (band != Y)			/* Cb and Cr bands only */
   {
      unsigned label;

      for (label = 0; label < MAXLABELS; label++)
	 if (ischild (y_state))
	    new_y_state [label] = wfa->tree [y_state][label];
	 else
	    new_y_state [label] = RANGE;
   }
   else
      new_y_state [0] = new_y_state [1] = RANGE;
   
   /*
    *  Store contents of all models that may get modified during recursion
    */
   domain_model   = c->domain_pool->model_duplicate (c->domain_pool->model);
   d_domain_model = c->d_domain_pool->model_duplicate (c->d_domain_pool->model);
   coeff_model    = c->coeff->model_duplicate (c->coeff, c->coeff->model);
   d_coeff_model  = c->d_coeff->model_duplicate (c->d_coeff, c->d_coeff->model);
   tree_model     = c->tree;
   p_tree_model   = c->p_tree;
   states         = wfa->states;	
   
   /*
    *  First alternative of range approximation:
    *  Compute costs of linear combination.
    */
   if (range->level <= c->options.lc_max_level) /* range is small enough */
   {
      lrange                 = *range;
      lrange.tree            = RANGE;
      lrange.tree_bits       = tree_bits (LEAF, lrange.level, &c->tree);
      lrange.matrix_bits     = 0;
      lrange.weights_bits    = 0;
      lrange.mv_tree_bits    = try_mc ? 1 : 0; /* mc allowed but not used */
      lrange.mv_coord_bits   = 0;
      lrange.nd_tree_bits    = 0;	
      lrange.nd_weights_bits = 0;	
      lrange.prediction	     = NO;
      
      lincomb_costs
	 = approximate_range (max_costs, price, c->options.max_elements,
			      y_state, &lrange,
			      (delta ? c->d_domain_pool : c->domain_pool),
			      (delta ? c->d_coeff : c->coeff), wfa, c);
   }
   else
      lincomb_costs = MAXCOSTS;		

   /*
    *  Store contents of models that have been modified
    *  by approximate_range () above ...
    */
   lc_domain_model   = c->domain_pool->model;
   lc_d_domain_model = c->d_domain_pool->model;
   lc_coeff_model    = c->coeff->model;
   lc_d_coeff_model  = c->d_coeff->model;
   /*
    *  ... and restore them with values before lc
    */
   c->domain_pool->model   = c->domain_pool->model_duplicate (domain_model);
   c->d_domain_pool->model = c->d_domain_pool->model_duplicate (d_domain_model);
   c->coeff->model         = c->coeff->model_duplicate (c->coeff, coeff_model);
   c->d_coeff->model       = c->d_coeff->model_duplicate (c->d_coeff,
							  d_coeff_model);
   
   /*
    *  Second alternative of range approximation:
    *  Compute costs of recursive subdivision.
    */
   if (range->level > c->options.lc_min_level) /* range is large enough */
   {
      unsigned label;
      
      memset (&child [0], 0, 2 * sizeof (range_t)); /* initialize childs */

      /*
       *  Initialize a new range for recursive approximation
       */
      rrange                 = *range;
      rrange.tree_bits       = tree_bits (CHILD, rrange.level, &c->tree);
      rrange.matrix_bits     = 0;
      rrange.weights_bits    = 0;
      rrange.err             = 0;
      rrange.mv_tree_bits    = try_mc ? 1 : 0;	/* mc allowed but not used */
      rrange.mv_coord_bits   = 0;
      rrange.nd_tree_bits    = try_nd ?
			       tree_bits (CHILD, lrange.level, &c->p_tree): 0;
      rrange.nd_weights_bits = 0;	
      rrange.prediction	     = NO;

      /*
       *  Initialize the cost function and subdivide the current range.
       *  Every child is approximated by a recursive call of subdivide()
       */
      subdivide_costs = (rrange.tree_bits + rrange.weights_bits
			 + rrange.matrix_bits + rrange.mv_tree_bits
			 + rrange.mv_coord_bits + rrange.nd_tree_bits
			 + rrange.nd_weights_bits) * price;
      
      for (label = 0; label < MAXLABELS; label++) 
      {
	 real_t remaining_costs;	/* upper limit for next recursion */
	 
	 child[label].image          = rrange.image * MAXLABELS + label + 1;
	 child[label].address        = rrange.address * MAXLABELS + label;
	 child[label].global_address = rrange.global_address * MAXLABELS
				       + label;
	 child[label].level          = rrange.level - 1;
	 child[label].x	= rrange.level & 1
			  ? rrange.x
			  : (rrange.x
			     + label * width_of_level (rrange.level - 1));
	 child[label].y = rrange.level & 1
			  ? (rrange.y
			     + label * height_of_level (rrange.level - 1))
			  : rrange.y;
	 
	 /* 
	  *  If neccessary compute the inner products of the new states
	  *  (generated during the recursive approximation of child [0])
	  */
	 if (label && rrange.level <= c->options.lc_max_level)
	    compute_ip_images_state (child[label].image, child[label].address,
				     child[label].level, 1, states, wfa, c);
	 /*
	  *  Call subdivide() for both childs. 
	  *  Abort the recursion if 'subdivide_costs' exceed 'lincomb_costs'
	  *  or 'max_costs'.
	  */
	 remaining_costs = min (lincomb_costs, max_costs) - subdivide_costs;

	 if (remaining_costs > 0)	/* still a way for improvement */
	 {
	    subdivide_costs += subdivide (remaining_costs, band,
					  new_y_state [label], &child [label],
					  wfa, c, prediction, delta);
	 }
	 else if (try_mc && child[label].level >= wfa->wfainfo->p_min_level)
	 {
	    fill_norms_table (child[label].x, child[label].y,
			      child[label].level, wfa->wfainfo, c->mt);
	 }
	 
	 if (try_mc)
	    update_norms_table (rrange.level, wfa->wfainfo, c->mt);
	 
	 /*
	  *  Update of progress meter
	  */
	 if (c->options.progress_meter != FIASCO_PROGRESS_NONE)
	 {
	    if (c->options.progress_meter == FIASCO_PROGRESS_PERCENT)
	    {
	       unsigned	new_percent; 	/* new status of progress meter */
	 
	       new_percent = (child[label].global_address + 1) * 100.0
			     / (1 << (wfa->wfainfo->level - child[label].level));
	       if (new_percent > percent)
	       {
		  percent = new_percent;
		  info ("%3d%%  \r", percent);
	       }
	    }
	    else if (c->options.progress_meter == FIASCO_PROGRESS_BAR)
	    {
	       unsigned	new_percent;	/* new status of progress meter */
	 
	       new_percent = (child[label].global_address + 1) * 50.0
			     / (1 << (wfa->wfainfo->level
				      - child[label].level));
	       for (; new_percent > percent; percent++)
	       {
		  info ("#");
	       }
	    }
	 }
   
	 /*
	  *  If costs of subdivision exceed costs of linear combination 
	  *  then abort recursion.
	  */
	 if (subdivide_costs >= min (lincomb_costs, max_costs)) 
	 {
	    subdivide_costs = MAXCOSTS;
	    break; 
	 }
	 rrange.err             += child [label].err;
	 rrange.tree_bits       += child [label].tree_bits;
	 rrange.matrix_bits     += child [label].matrix_bits;
	 rrange.weights_bits    += child [label].weights_bits;
	 rrange.mv_tree_bits    += child [label].mv_tree_bits;
	 rrange.mv_coord_bits   += child [label].mv_coord_bits;
	 rrange.nd_weights_bits += child [label].nd_weights_bits;
	 rrange.nd_tree_bits    += child [label].nd_tree_bits;

	 tree_update (ischild (child [label].tree) ? CHILD : LEAF,
		      child [label].level, &c->tree);
	 tree_update (child [label].prediction ? LEAF : CHILD,
		      child [label].level, &c->p_tree);
      }
   }
   else
      subdivide_costs = MAXCOSTS;

   /*
    *  Third alternative of range approximation:
    *  Predict range via motion compensation or nondeterminism and 
    *  approximate delta image. 
    */
   if (try_mc || try_nd)		/* try prediction */
   {
      real_t prediction_costs;	/* Costs arising from approx. the current
				   range with prediction */

      prediction_costs
	 = predict_range (min (min (lincomb_costs, subdivide_costs),
			       max_costs),
			  price, range, wfa, c, band, y_state, states,
			  &tree_model, &p_tree_model, domain_model,
			  d_domain_model, coeff_model, d_coeff_model);
      if (prediction_costs < MAXCOSTS)	/* prediction has smallest costs */
      {
	 c->domain_pool->model_free (domain_model);
	 c->d_domain_pool->model_free (d_domain_model);
	 c->domain_pool->model_free (lc_domain_model);
	 c->d_domain_pool->model_free (lc_d_domain_model);
	 c->coeff->model_free (coeff_model);
	 c->d_coeff->model_free (d_coeff_model);
	 c->coeff->model_free (lc_coeff_model);
	 c->d_coeff->model_free (lc_d_coeff_model);
	 
	 return prediction_costs;
      }
   }

   if (lincomb_costs >= MAXCOSTS && subdivide_costs >= MAXCOSTS)
   {
      /*
       *  Return MAXCOSTS if neither a linear combination nor a recursive
       *  subdivision yield costs less than 'max_costs'
       */
      c->domain_pool->model_free (c->domain_pool->model);
      c->d_domain_pool->model_free (c->d_domain_pool->model);
      c->domain_pool->model_free (lc_domain_model);
      c->d_domain_pool->model_free (lc_d_domain_model);

      c->coeff->model_free (c->coeff->model);
      c->d_coeff->model_free (c->d_coeff->model);
      c->coeff->model_free (lc_coeff_model);
      c->d_coeff->model_free (lc_d_coeff_model);
	 
      c->domain_pool->model   = domain_model;
      c->d_domain_pool->model = d_domain_model;
      c->coeff->model	      = coeff_model;
      c->d_coeff->model	      = d_coeff_model;
      c->tree                 = tree_model;
      c->p_tree               = p_tree_model;
      
      if (wfa->states != states)
	 remove_states (states, wfa);

      return MAXCOSTS;
   }
   else if (lincomb_costs < subdivide_costs) 
   {
      /*
       *  Use the linear combination: The factors of the linear combination
       *  are stored already in 'range', so revert the probability models
       *  only. 
       */
      c->domain_pool->model_free (c->domain_pool->model);
      c->d_domain_pool->model_free (c->d_domain_pool->model);
      c->domain_pool->model_free (domain_model);
      c->d_domain_pool->model_free (d_domain_model);

      c->coeff->model_free (c->coeff->model);
      c->d_coeff->model_free (c->d_coeff->model);
      c->coeff->model_free (coeff_model);
      c->d_coeff->model_free (d_coeff_model);
      
      c->domain_pool->model   = lc_domain_model;
      c->d_domain_pool->model = lc_d_domain_model;
      c->coeff->model	      = lc_coeff_model;
      c->d_coeff->model	      = lc_d_coeff_model;
      c->tree                 = tree_model;
      c->p_tree               = p_tree_model;

      *range = lrange;
      
      if (wfa->states != states)
	 remove_states (states, wfa);

      return lincomb_costs;
   }
   else
   {
      /*
       *  Use the recursive subdivision: Generate a new state with transitions
       *  given in child[].
       *  Don't use state in linear combinations in any of the following cases:
       *  - if color component is Cb or Cr
       *  - if level of state > tiling level 
       *  - if state is (partially) outside image geometry 
       */
      if (band > Y
	  || (c->tiling->exponent
	      && rrange.level > wfa->wfainfo->level - c->tiling->exponent)
	  || (range->x + width_of_level (range->level)
	      > c->mt->original->width)
	  || (range->y + height_of_level (range->level)
	      > c->mt->original->height))
	 init_new_state (YES, delta, &rrange, child, new_y_state, wfa, c);
      else
	 init_new_state (NO, delta, &rrange, child, new_y_state, wfa, c);

      *range = rrange;

      c->domain_pool->model_free (domain_model);
      c->d_domain_pool->model_free (d_domain_model);
      c->domain_pool->model_free (lc_domain_model);
      c->d_domain_pool->model_free (lc_d_domain_model);
      c->coeff->model_free (coeff_model);
      c->d_coeff->model_free (d_coeff_model);
      c->coeff->model_free (lc_coeff_model);
      c->d_coeff->model_free (lc_d_coeff_model);

      return subdivide_costs;
   }
}

void
cut_to_bintree (real_t *dst, const word_t *src,
		unsigned src_width, unsigned src_height,
		unsigned x0, unsigned y0, unsigned width, unsigned height)
/*
 *  Cut region ('x0', 'y0', 'width', 'height') of the pixel array 'src'.
 *  Size of image is given by 'src_width' x 'src_height'.
 *  'dst' pixels are converted to bintree order and real format.
 *
 *  No return value.
 *
 *  Side effects:
 *	'dst []' is filled with corresponding region.
 */
{
   const unsigned mask01      = 0x555555; /* binary ...010101010101 */
   const unsigned mask10      = 0xaaaaaa; /* binary ...101010101010 */
   const unsigned mask01plus1 = mask01 + 1; /* binary ...010101010110 */
   const unsigned mask10plus1 = mask10 + 1; /* binary ...101010101011 */
   unsigned  	  x, y;			/* pixel coordinates */
   unsigned  	  xmask, ymask;		/* address conversion */

   if (width != height && width != (height >> 1))
      error ("Bintree cutting requires special type of images.");

   ymask = 0;
   for (y = y0; y < y0 + height; y++, ymask = (ymask + mask10plus1) & mask01)
   {
      xmask = 0;
      for (x = x0; x < x0 + width; x++, xmask = (xmask + mask01plus1) & mask10)
      {
	 if (y >= src_height || x >= src_width)
	    dst [xmask | ymask] = 0;
	 else
	    dst [xmask | ymask] = src [y * src_width + x] / 16;
      }
   }
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
init_new_state (bool_t auxiliary_state, bool_t delta, range_t *range,
		const range_t *child, const int *y_state,
		wfa_t *wfa, coding_t *c)
/*
 *  Initializes a new state with all parameters needed for the encoding step.
 *  If flag 'auxiliary_state' is set then don't insert state into domain pools.
 *  If flag 'delta' is set then state represents a delta image (prediction via
 *  nondeterminism or motion compensation).
 *  'range' the current range image,
 *   'child []' the left and right childs of 'range'.
 *
 *  No return value.
 *
 *  Side effects:
 *	New state is appended to 'wfa' (and also its inner products and images
 *      are computed and stored in 'c')
 */
{
   unsigned label;
   bool_t   state_is_domain = NO;

   if (!auxiliary_state)
   {
      if (!delta || c->options.delta_domains)
	 state_is_domain = c->domain_pool->append (wfa->states, range->level,
						   wfa, c->domain_pool->model);
      if (delta || c->options.normal_domains)
	 state_is_domain = c->d_domain_pool->append (wfa->states, range->level,
						     wfa,
						     c->d_domain_pool->model)
			   || state_is_domain;
   }
   else
      state_is_domain = NO;
   
   range->into [0] = NO_EDGE;
   range->tree     = wfa->states;
   
   for (label = 0; label < MAXLABELS; label++) 
   {
      wfa->tree [wfa->states][label]       = child [label].tree;
      wfa->y_state [wfa->states][label]    = y_state [label];
      wfa->mv_tree [wfa->states][label]    = child [label].mv;
      wfa->x [wfa->states][label]          = child [label].x;
      wfa->y [wfa->states][label]          = child [label].y;
      wfa->prediction [wfa->states][label] = child [label].prediction;

      append_transitions (wfa->states, label, child [label].weight,
			  child [label].into, wfa);
   }
   wfa->delta_state [wfa->states] = delta;

   if (range->err < 0)
      warning ("Negative image norm: %f, %f", child [0].err, child [1].err);

/*    state_is_domain = YES; */
   
   append_state (!state_is_domain,
		 compute_final_distribution (wfa->states, wfa),
		 range->level, wfa, c);
}

static void
init_range (range_t *range, const image_t *image, unsigned band,
	    const wfa_t *wfa, coding_t *c)
/*
 *  Read a new 'range' of the image 'image_name' (current color component
 *  is 'band') and compute the new inner product arrays.
 *
 *  No return value.
 *
 *  Side effects:
 *	'c->pixels' are filled with pixel values of image block 
 *	'c->ip_images_state' are computed with respect to new image block 
 *	'range->address' and 'range->image' are initialized with zero
 */
{
   unsigned state;
   
   /*
    *  Clear already computed products
    */
   for (state = 0; state < wfa->states; state++)
      if (need_image (state, wfa))
	 memset (c->ip_images_state[state], 0,
		 size_of_tree (c->products_level) * sizeof(real_t));

   cut_to_bintree (c->pixels, image->pixels [band],
		   image->width, image->height,
		   range->x, range->y, width_of_level (range->level),
		   height_of_level (range->level));
   
   range->address = range->image = 0;
   compute_ip_images_state (0, 0, range->level, 1, 0, wfa, c);
}


