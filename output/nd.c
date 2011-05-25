/*
 *  nd.c:		Output of prediction tree	
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
#include "arith.h"
#include "misc.h"
#include "bit-io.h"
#include "rpf.h"
#include "list.h"

#include "nd.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static unsigned
encode_nd_tree (const wfa_t *wfa, bitfile_t *output);
static void
encode_nd_coefficients (unsigned total, const wfa_t *wfa, bitfile_t *output);

/*****************************************************************************

				public code
  
*****************************************************************************/

void
write_nd (const wfa_t *wfa, bitfile_t *output)
/*
 *  Write prediction information of 'wfa' to given stream 'output'.
 *  Coefficients are quantized with model 'p_rpf'.
 *
 *  No return value.
 */
{
   unsigned total = encode_nd_tree (wfa, output);
   
   if (total > 0)
      encode_nd_coefficients (total, wfa, output);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static unsigned
encode_nd_tree (const wfa_t *wfa, bitfile_t *output)
/*
 *  Write prediction tree of 'wfa' to given stream 'output'. 
 *
 *  No return value.
 */
{
   lqueue_t *queue;			/* queue of states */
   int	     state, next;		/* state and its current child */
   unsigned  used, not_used;		/* counter ND used/not used */
   u_word_t  low;			/* Start of the current code range */
   u_word_t  high;			/* End of the current code range */
   u_word_t  underflow;			/* Number of underflow bits pending */
   u_word_t  sum0, sum1;		/* Probability model */
   unsigned  bits = bits_processed (output);

   used = not_used = 0;
   
   /*
    *  Initialize arithmetic coder
    */
   low       = 0;
   high      = 0xffff;
   underflow = 0;
   sum0      = 1;
   sum1      = 11;
   
   queue = alloc_queue (sizeof (int));
   state = wfa->root_state;
   queue_append (queue, &state);
   
   /*
    *  Traverse the WFA tree in breadth first order (using a queue).
    */
   while (queue_remove (queue, &next))
   {
      unsigned label;
      
      if (wfa->level_of_state [next] > wfa->wfainfo->p_max_level + 1)
      {
	 /*
	  *  Nondetermismn is not allowed at levels larger than
	  *  'wfa->wfainfo->p_max_level'.
	  */
	 for (label = 0; label < MAXLABELS; label++)
	    if (ischild (state = wfa->tree [next][label]))
	       queue_append (queue, &state); /* continue with childs */
      }
      else if (wfa->level_of_state [next] > wfa->wfainfo->p_min_level)
      {
	 for (label = 0; label < MAXLABELS; label++)
	    if (ischild (state = wfa->tree [next][label]))
	    {
	       unsigned range;		/* Current interval range */

	       if (isedge (wfa->into [next][label][0])) /* prediction used */
	       {
		  used++;
		  
		  /*
		   *  Encode a '1' symbol
		   */
		  range =  (high - low) + 1;
		  low   = low + (u_word_t) ((range * sum0) / sum1);
		  RESCALE_OUTPUT_INTERVAL;
	       }
	       else			/* no predict., continue with childs */
	       {
		  not_used++;
		  if (wfa->level_of_state [state] > wfa->wfainfo->p_min_level)
		     queue_append (queue, &state);
		  
		  /*
		   *  Encode a '0' symbol
		   */
		  range =  (high - low) + 1;
		  high  = low + (u_word_t) ((range * sum0) / sum1 - 1);
		  RESCALE_OUTPUT_INTERVAL;
		  sum0++;
	       }
	       /*
		*  Update the frequency counts
		*/
	       sum1++;
	       if (sum1 > 50)		/* Scale the symbol frequencies */
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
   }
   free_queue (queue);

   /*
    *  Flush the quasi-arithmetic encoder
    */
   low = high;
   RESCALE_OUTPUT_INTERVAL;
   OUTPUT_BYTE_ALIGN (output);

   debug_message ("%d nd fields: %d used nd, %d used not nd", used + not_used,
		  used, not_used);
   {
      unsigned total = used + not_used;
      
      debug_message ("nd-tree:      %5d bits. (%5d symbols => %5.2f bps)",
		     bits_processed (output) - bits, total,
		     total > 0 ? ((bits_processed (output) - bits) /
				  (double) total) : 0);
   }

   return used;
}

static void
encode_nd_coefficients (unsigned total, const wfa_t *wfa, bitfile_t *output)
/*
 *  Write #'total' weights of nondeterministic part of 'wfa' to given 'output'
 *  stream. Coefficients are stored with arithmetic coding (the model is
 *  given by 'p_rpf').
 *
 *  No return value.
 */
{
   unsigned bits = bits_processed (output);

   {
      unsigned *coefficients;		/* array of factors to encode */
      unsigned *ptr;			/* pointer to current factor */
      unsigned	state, label, edge;
      word_t	domain;
      
      ptr = coefficients  = Calloc (total, sizeof (unsigned));

      for (state = wfa->basis_states; state < wfa->states; state++)
	 for (label = 0; label < MAXLABELS; label++)
	    if (ischild (wfa->tree [state][label])
		&& isedge (wfa->into [state][label][0]))
	       for (edge = 0; isedge (domain = wfa->into [state][label][edge]);
		    edge++)
	       {
		  if (ptr - coefficients >= (int) total)
		     error ("Can't write more than %d coefficients.", total);

		  *ptr++ = rtob (wfa->weight [state][label][edge],
				 wfa->wfainfo->dc_rpf);
	       }

      /*
       *  Encode array of coefficients with arithmetic coding
       */
      {
	 const int scaling = 50;	/* scaling factor of prob. model */
	 unsigned  c_symbols = 1 << (wfa->wfainfo->dc_rpf->mantissa_bits + 1);

	 encode_array (output, coefficients, NULL, &c_symbols, 1,
		       total, scaling);
      }
      
      debug_message ("nd-factors:   %5d bits. (%5d symbols => %5.2f bps)",
		     bits_processed (output) - bits, total,
		     total ? ((bits_processed (output) - bits)
			      / (double) total) : 0);
      Free (coefficients);
   }
}


