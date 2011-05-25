/*
 *  tree.c:		Input of bintree partitioning
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:50:13 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#include "config.h"

#include "types.h"
#include "macros.h"
#include "error.h"

#include "bit-io.h"
#include "arith.h"
#include "misc.h"
#include "wfalib.h"
#include "tiling.h"

#include "tree.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static unsigned
restore_depth_first_order (unsigned src_state, unsigned level, unsigned x,
			   unsigned y, unsigned *dst_state,
			   word_t (*bfo_tree)[MAXLABELS],
			   wfa_t *wfa, tiling_t *tiling);
static void 
decode_tree (bitfile_t *input, byte_t *data, unsigned n_data, unsigned scaling,
	     u_word_t sum0, u_word_t sum1);

/*****************************************************************************

				public code
  
*****************************************************************************/

void
read_tree (wfa_t *wfa, tiling_t *tiling, bitfile_t *input)
/*
 *  Read bintree partitioning of WFA from the 'input' stream.
 *  'tiling' provides the information about image tiling, if applied.
 *
 *  No return value.
 *
 *  Side effects:
 *	'wfa->tree', 'wfa->x', 'wfa->y', 'wfa->level_of_state'
 *      are filled with decoded values.
 */
{
   byte_t *bitstring;			/* the encoded data */
   word_t (*bfo_tree)[MAXLABELS];	/* node numbers in BFO */
      
   /*
    *  Read WFA tree stored in breadth first order
    */
   {
      unsigned total = (wfa->states - wfa->basis_states) * MAXLABELS;
      unsigned scale = total / 20;

      bitstring = Calloc (total, sizeof (byte_t));
      decode_tree (input, bitstring, total, scale, 1, 11);
   }
   
   /*
    *  Generate tree using a breadth first traversal
    */
   {
      unsigned 	next;			/* next free node number of the tree */
      unsigned 	state;
      unsigned 	label;
      byte_t   *buffer = bitstring;	/* pointer to decoded data */
      
      bfo_tree = Calloc (wfa->states * MAXLABELS, sizeof (word_t));
      for (state = 0, next = 1; state < next; state++)
	 for (label = 0; label < MAXLABELS; label++)
	    bfo_tree [state][label] = *buffer++ ? next++ : RANGE;
   }

   /*
    *  Traverse tree and restore depth first order
    */
   {
      unsigned dst_state = wfa->basis_states;

      wfa->root_state
	 = restore_depth_first_order (0, (wfa->wfainfo->level
					  + (wfa->wfainfo->color ? 2 : 0)),
				      0, 0, &dst_state, bfo_tree, wfa, tiling);
   }

   Free (bitstring);
   Free (bfo_tree);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static unsigned
restore_depth_first_order (unsigned src_state, unsigned level, unsigned x,
			   unsigned y, unsigned *dst_state,
			   word_t (*bfo_tree)[MAXLABELS],
			   wfa_t *wfa, tiling_t *tiling)
/*
 *  Map state 'src_state' (breadth first order) 
 *  to state '*dst_state' (depth first order)
 *  Add a tree edge 'state' --> 'child' with label and weight 1.0
 *  if required.
 *  'x', 'y' give the coordinates of the current state in the 'color' image
 *  of size 'image_level'. 'tiling' defines the image partitioning. 
 *  
 *  Return value:
 *	new node number in depth first order
 *
 *  Side effects:
 *	'wfa->tree', 'wfa->x', 'wfa->y', 'wfa->level_of_state'
 *      are filled with decoded values.
 */
{
   unsigned newx [MAXLABELS];		/* x coordinate of childs */
   unsigned newy [MAXLABELS];		/* y coordinate of childs */
   unsigned x0, y0;			/* NW corner of image tile */
   unsigned width, height;		/* size of image tile */

   /*
    *  If tiling is performed then replace current coordinates
    */
   if (tiling->exponent && level == wfa->wfainfo->level - tiling->exponent)
   {
      unsigned tile;
      
      for (tile = 0; tile < 1U << tiling->exponent; tile++)
      {
	 locate_subimage (wfa->wfainfo->level, level, tile,
			  &x0, &y0, &width, &height);
	 if (x0 == x && y0 == y) /* matched ! */
	 {
	    locate_subimage (wfa->wfainfo->level, level, tiling->vorder[tile],
			     &x, &y, &width, &height);
	    break;
	 }
      }
   }
   /*
    *  Coordinates of childs 0 and 1
    */
   if (wfa->wfainfo->color && level == wfa->wfainfo->level + 1)
      newx[0] = newy[0] = newx[1] = newy[1] = 0;
   else
   {
      newx[0] = x;
      newy[0] = y;
      newx[1] = level & 1 ? x : x + width_of_level (level - 1);
      newy[1] = level & 1 ? y + height_of_level (level - 1) : y;
   }
   
   /*
    *  Remap node numbers
    */
   {
      int      child [MAXLABELS];	/* childs of current node (state) */
      int      domain;			/* current domain */
      unsigned label;

      for (label = 0; label < MAXLABELS; label++)
	 if (!isrange (domain = bfo_tree [src_state][label]))
	    child [label] = restore_depth_first_order (domain, level - 1,
						       newx [label],
						       newy [label], dst_state,
						       bfo_tree, wfa, tiling);
	 else
	    child [label] = RANGE;

      for (label = 0; label < MAXLABELS; label++)
      {
	 wfa->tree [*dst_state][label] = child [label];
	 wfa->x [*dst_state][label]    = newx [label];
	 wfa->y [*dst_state][label]    = newy [label];
      }
      wfa->level_of_state [*dst_state] = level;
   }
   
   return (*dst_state)++;
}	

/****************************************************************************

                 Binary adaptive arithmetic compression
 
****************************************************************************/

static void 
decode_tree (bitfile_t *input, byte_t *data, unsigned n_data, unsigned scaling,
	     u_word_t sum0, u_word_t sum1)
/*
 *  Decode bintree partitioning using adaptive binary arithmetic decoding.
 *  'input'	input stream,
 *  'data'	buffer for decoded szmbols,
 *  'n_data'	number of symbols to decode,
 *  'scaling'	rescale probability models if range > 'scaling'
 *  'sum0'	initial totals of symbol '0'
 *  'sum1'	initial totals of symbol '1'
 *
 *  No return value.
 *
 *  Side effects:
 *	'data []' is filled with the decoded bitstring
 */
{
   u_word_t code;			/* The present input code value */
   u_word_t low;			/* Start of the current code range */
   u_word_t high;			/* End of the current code range */
   unsigned n;				/* Data counter */

   assert (data);

   code = get_bits (input, 16);
   low  = 0;
   high = 0xffff;

   for (n = n_data; n; n--) 
   {
      unsigned count;			/* Current interval count */
      unsigned range;			/* Current interval range */
      
      count = (((code - low) + 1) * sum1 - 1) / ((high - low) + 1);
      if (count < sum0)
      {
	 /*
	  *  Decode a '0' symbol
	  *  First, the range is expanded to account for the symbol removal.
	  */
	 range = (high - low) + 1;
	 high = low + (u_word_t) ((range * sum0) / sum1 - 1 );

	 RESCALE_INPUT_INTERVAL;

	 *data++ = 0;
	 /*
	  *  Update the frequency counts
	  */
	 sum0++;
	 sum1++;
	 if (sum1 > scaling) /* scale the symbol frequencies */
	 {
	    sum0 >>= 1;
	    sum1 >>= 1;
	    if (!sum0)
	       sum0 = 1;
	    if (sum0 >= sum1)
	       sum1 = sum0 + 1;
	 }

      }
      else
      {
	 /*
	  *  Decode a '1' symbol
	  *  First, the range is expanded to account for the symbol removal.
	  */
	 range = (high - low) + 1;
	 high = low + (u_word_t) ((range * sum1) / sum1 - 1);
	 low  = low + (u_word_t) ((range * sum0) / sum1);

	 RESCALE_INPUT_INTERVAL;

	 *data++ = 1;
	 /*
	  *  Update the frequency counts
	  */
	 sum1++;
	 if (sum1 > scaling) /* scale the symbol frequencies */
	 {
	    sum0 >>= 1;
	    sum1 >>= 1;
	    if (!sum0)
	       sum0 = 1;
	    if (sum0 >= sum1)
	       sum1 = sum0 + 1;
	 }
      }
   }
   INPUT_BYTE_ALIGN (input);
}


