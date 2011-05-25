/*
 *  weights.c:		Output of weights
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:50:31 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#include "config.h"

#include "types.h"
#include "macros.h"
#include "error.h"

#include "wfa.h"
#include "misc.h"
#include "bit-io.h"
#include "arith.h"
#include "wfalib.h"

#include "weights.h"

/*****************************************************************************

				public code
  
*****************************************************************************/

void
write_weights (unsigned total, const wfa_t *wfa, bitfile_t *output)
/*
 *  Traverse the transition matrices of the 'wfa' and write #'total'
 *  weights != 0 to stream 'output'.
 *
 *  No return value.
 */
{
   unsigned  state, label;		/* current label */
   unsigned  offset1, offset2;		/* model offsets. */
   unsigned  offset3, offset4;		/* model offsets. */
   unsigned *weights_array;		/* array of weights to encode */
   unsigned *wptr;			/* pointer to current weight */
   unsigned *level_array;		/* array of corresponding levels */
   unsigned *lptr;			/* pointer to current corr. level */
   int	     min_level, max_level;	/* min and max range level */
   int	     d_min_level, d_max_level; 	/* min and max delta range level */
   bool_t    dc, d_dc;			/* true if dc or delta dc are used */
   bool_t    delta_approx = NO;		/* true if delta has been used */
   unsigned  delta_count  = 0;		/* number of delta ranges */
   unsigned  bits 	  = bits_processed (output);
   
   /*
    *  Check whether delta approximation has been used
    */
   for (state = wfa->basis_states; state < wfa->states; state++)
      if (wfa->delta_state [state])
      {
	 delta_approx = YES;
	 break;
      }
   
   /*
    *  Generate array of corresponding levels (context of probability model)
    */
   min_level = d_min_level = MAXLEVEL;
   max_level = d_max_level = 0;
   dc 	     = d_dc	   = NO;
   
   for (state = wfa->basis_states; state < wfa->states; state++)
      for (label = 0; label < MAXLABELS; label++)
         if (isrange (wfa->tree [state][label]))
	 {
	    if (delta_approx && wfa->delta_state [state]) /* delta approx. */
	    {
	       d_min_level = min (d_min_level,
				  wfa->level_of_state [state] - 1);
	       d_max_level = max (d_max_level,
				  wfa->level_of_state [state] - 1);
	       if (wfa->into [state][label][0] == 0)
		  d_dc = YES;
	    }
	    else
	    {
	       min_level = min (min_level, wfa->level_of_state [state] - 1);
	       max_level = max (max_level, wfa->level_of_state [state] - 1);
	       if (wfa->into [state][label][0] == 0)
		  dc = YES;
	    }
	 }
   if (min_level > max_level)		/* no lc found */
      max_level = min_level - 1;
   if (d_min_level > d_max_level)
      d_max_level = d_min_level - 1;

   /*
    *  Context model:
    *		0		DC weight
    *		1		Delta DC weight
    *		2-k		normal weights per level
    *		k+1 - m		Delta weights per level
    */

   offset1 = dc ? 1 : 0;
   offset2 = offset1 + (d_dc ? 1 : 0);
   offset3 = offset2 + (max_level - min_level + 1);
   offset4 = offset3 + (d_max_level - d_min_level + 1);
   
   /*
    *  Weights are encoded as follows:
    *  all weights of state n
    *     sorted by label
    *        sorted by domain number
    */

   wptr = weights_array = Calloc (total, sizeof (unsigned));
   lptr = level_array   = Calloc (total, sizeof (unsigned));

   for (state = wfa->basis_states; state < wfa->states; state++)
      for (label = 0; label < MAXLABELS; label++)
         if (isrange (wfa->tree [state][label]))
	 {
	    int	edge;			/* current edge */
	    int	domain;			/* current domain (context of model) */
	    
            for (edge = 0; isedge (domain = wfa->into [state][label][edge]);
		 edge++)
            {
	       if (wptr - weights_array >= (int) total)
		  error ("Can't write more than %d weights.", total);
	       if (domain)		/* not DC component */
	       {
		  if (delta_approx && wfa->delta_state [state]) /* delta */
		  {
		     *wptr++ = rtob (wfa->weight [state][label][edge],
				     wfa->wfainfo->d_rpf);
		     *lptr++ = offset3
			       + wfa->level_of_state [state] - 1 - d_min_level;
		     delta_count++;
		  }
		  else
		  {
		     *wptr++ = rtob (wfa->weight [state][label][edge],
				     wfa->wfainfo->rpf);
		     *lptr++ = offset2
			       + wfa->level_of_state [state] - 1 - min_level;
		  }
	       }
	       else			/* DC component */
	       {
		  if (delta_approx && wfa->delta_state [state]) /* delta */
		  {
		     *wptr++ = rtob (wfa->weight [state][label][edge],
				     wfa->wfainfo->d_dc_rpf);
		     *lptr++ = offset1;
		  }
		  else
		  {
		     *wptr++ = rtob (wfa->weight [state][label][edge],
				     wfa->wfainfo->dc_rpf);
		     *lptr++ = 0;
		  }
	       }
            }
	 }

   {
      unsigned	 i;
      unsigned	*c_symbols = Calloc (offset4, sizeof (int));
      const int	 scale 	   = 500;	/* scaling of probability model */

      c_symbols [0] = 1 << (wfa->wfainfo->dc_rpf->mantissa_bits + 1);
      if (offset1 != offset2)
	 c_symbols [offset1] = 1 << (wfa->wfainfo->d_dc_rpf->mantissa_bits
				     + 1);
      for (i = offset2; i < offset3; i++)
	 c_symbols [i] = 1 << (wfa->wfainfo->rpf->mantissa_bits + 1);
      for (; i < offset4; i++)
	 c_symbols [i] = 1 << (wfa->wfainfo->d_rpf->mantissa_bits + 1);
      
      encode_array (output, weights_array, level_array, c_symbols, offset4,
		    total, scale);
      Free (c_symbols);
   }
   
   debug_message ("%d delta weights out of %d.", delta_count, total);
   debug_message ("weights:      %5d bits. (%5d symbols => %5.2f bps)",
		  bits_processed (output) - bits, total,
		  (bits_processed (output) - bits) / (double) total);

   Free (weights_array);
   Free (level_array);
}
