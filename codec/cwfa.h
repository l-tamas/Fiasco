/*
 *  cwfa.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/10/28 17:39:30 $
 *  $Author: hafner $
 *  $Revision: 5.3 $
 *  $State: Exp $
 */

#ifndef _CWFA_H
#define _CWFA_H

#include "wfa.h"
#include "types.h"
#include "tiling.h"
#include "rpf.h"
#include "domain-pool.h"
#include "bintree.h"
#include "coeff.h"
#include "list.h"
#include "wfalib.h"
#include "options.h"

extern const real_t MAXCOSTS;

typedef struct motion
{
   image_t	 *original;		/* Current image */
   image_t	 *past;			/* Preceeding image */
   image_t	 *future;		/* Succeeding image */
   frame_type_e	  frame_type;		/* frame type: B_, P_ I_FRAME */
   unsigned	  number;		/* display number of frame */
   real_t        *xbits;		/* # bits for mv x-component */
   real_t        *ybits;		/* # bits for mv y-component */
   real_t       **mc_forward_norms; 	/* norms of mcpe */
   real_t       **mc_backward_norms; 	/* norms of mcpe */
} motion_t;

typedef struct range
/*
 *  Information about current range in the original image.
 *  Approximation data (error, encoding bits, approximation type and factors
 *  of the linear combination) are also saved.
 */ 
{
   unsigned global_address;             /* We need absolute image addresses
				           for distance calculations. */
   unsigned x, y;			/* Coordinates of upper left corner */
   unsigned image;			/* Position in the tree */
   unsigned address;			/* Address of the pixel data */
   unsigned level;			/* Level of the range */
   real_t   weight [MAXEDGES + 1];	/* coeff. of the approximation */
   word_t   into [MAXEDGES + 1];	/* used domains of the approximation */
   int	    tree;			/* == domain : range is approximated
					   with new state 'domain'
					   == RANGE  :
					   with a linear comb. */
   real_t   err;			/* approximation error */
   real_t   tree_bits;			/* # bits to encode tree */
   real_t   matrix_bits;		/* # bits to encode matrices */
   real_t   weights_bits;		/* # bits to encode weights */
   mv_t	    mv;				/* motion vector */
   real_t   mv_tree_bits;		/* # bits to encode mv tree */
   real_t   mv_coord_bits;		/* # bits to encode mv coordinates */
   real_t   nd_tree_bits;		/* # bits to encode nd tree */
   real_t   nd_weights_bits;		/* # bits to encode nd factors */
   bool_t   prediction;			/* range is predicted? */
} range_t;

typedef struct coding
/*
 *  All parameters and variables that must be accessible through the coding
 *  process.
 */
{
   real_t     	   price;		/* determines quality of approx. */
   real_t   	 **images_of_state;	/* image of state i at level
					   0, ... , imageslevel */
   real_t   *(*ip_states_state)[MAXLEVEL]; /* inner products between state i
					      and states 0, ... , i
					      at all image levels */
   real_t   	 **ip_images_state;	/* inner products between all
					   ranges and state i */
   real_t    	  *pixels;		/* current image pixels stored in tree
					   order (only leaves are stored) */
   unsigned   	   products_level;	/* inner products are stored up to
					   this level */
   tiling_t   	  *tiling;		/* tiling of the entire image */
   tree_t     	   tree;		/* probability model */
   tree_t     	   p_tree;		/* prediction probability model */
   motion_t   	  *mt;			/* motion compensation information */
   coeff_t   	  *coeff;
   coeff_t   	  *d_coeff;
   domain_pool_t  *domain_pool;
   domain_pool_t  *d_domain_pool;
   c_options_t     options;		/* global options */
} coding_t;

#endif /* not _CWFA_H */

