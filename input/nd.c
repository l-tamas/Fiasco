/*
 *  nd.c:		Input of prediction tree	
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
#include "list.h"
#include "wfalib.h"

#include "nd.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
decode_nd_coefficients (unsigned total, wfa_t *wfa, bitfile_t *input);
static unsigned
decode_nd_tree (wfa_t *wfa, bitfile_t *input);

/*****************************************************************************

				public code
  
*****************************************************************************/

void
read_nd (wfa_t *wfa, bitfile_t *input)
/*
 *  Read transitions of the nondetermistic 'wfa' part from 'input' stream.
 *  ND is used only at levels {'wfa->p_min_level', ... , 'wfa->p_max_level'}.
 *
 *  Side effects:
 *	'wfa->into' and 'wfa->weights' are filled with the decoded values
 */
{
   unsigned total = decode_nd_tree (wfa, input);
   
   if (total > 0)
      decode_nd_coefficients (total, wfa, input);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static unsigned
decode_nd_tree (wfa_t *wfa, bitfile_t *input)
/*
 *  Read 'wfa' prediction tree of given 'input' stream.
 *
 *  No return value.
 *
 *  Side effects:
 *	'wfa->into' is filled with the decoded values
 */
{
   lqueue_t *queue;			/* queue of states */
   int       next, state;		/* state and its current child */
   unsigned  total = 0;			/* total number of predicted states */
   u_word_t  sum0, sum1;		/* Probability model */
   u_word_t  code;			/* The present input code value */
   u_word_t  low;			/* Start of the current code range */
   u_word_t  high;			/* End of the current code range */

   /*
    *  Initialize arithmetic decoder
    */
   code = get_bits (input, 16);
   low  = 0;
   high = 0xffff;
   sum0 = 1;
   sum1 = 11;

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
	       unsigned count;		/* Current interval count */
	       unsigned range;		/* Current interval range */
	       
	       count = (((code - low) + 1) * sum1 - 1) / ((high - low) + 1);
	       if (count < sum0)
	       {
		  /*
		   *  Decode a '0' symbol
		   *  First, the range is expanded to account for the
		   *  symbol removal.
		   */
		  range = (high - low) + 1;
		  high = low + (u_word_t) ((range * sum0) / sum1 - 1 );
		  RESCALE_INPUT_INTERVAL;
		  /*
		   *  Update the frequency counts
		   */
		  sum0++;
		  sum1++;
		  if (sum1 > 50) /* scale the symbol frequencies */
		  {
		     sum0 >>= 1;
		     sum1 >>= 1;
		     if (!sum0)
			sum0 = 1;
		     if (sum0 >= sum1)
			sum1 = sum0 + 1;
		  }
		  if (wfa->level_of_state [state] > wfa->wfainfo->p_min_level)
		     queue_append (queue, &state);
	       }
	       else
	       {
		  /*
		   *  Decode a '1' symbol
		   *  First, the range is expanded to account for the
		   *  symbol removal.
		   */
		  range = (high - low) + 1;
		  high = low + (u_word_t) ((range * sum1) / sum1 - 1);
		  low  = low + (u_word_t) ((range * sum0) / sum1);
		  RESCALE_INPUT_INTERVAL;
		  /*
		   *  Update the frequency counts
		   */
		  sum1++;
		  if (sum1 > 50) /* scale the symbol frequencies */
		  {
		     sum0 >>= 1;
		     sum1 >>= 1;
		     if (!sum0)
			sum0 = 1;
		     if (sum0 >= sum1)
			sum1 = sum0 + 1;
		  }
		  append_edge (next, 0, -1, label, wfa);
		  total++;
	       }
	    }
      }
   }
   free_queue (queue);

   INPUT_BYTE_ALIGN (input);

   return total;
}

static void
decode_nd_coefficients (unsigned total, wfa_t *wfa, bitfile_t *input)
/*
 *  Read #'total' weights of nondeterministic part of 'wfa'  
 *  of given 'input' stream.
 *  'frame' gives the current frame number.
 *
 *  No return value.
 *
 *  Side effects:
 *	'wfa->weights' is filled with the decoded values.
 */
{
   unsigned *coefficients;		/* array of factors to encode */
   unsigned *ptr;			/* pointer to current factor */
   
   /*
    *  Decode array of coefficients stored with arithmetic coding
    */
   {
      const int	scaling  = 50;		/* scaling factor of prob. model */
      unsigned  c_symbols = 1 << (wfa->wfainfo->dc_rpf->mantissa_bits + 1);
      
      ptr = coefficients = decode_array (input, NULL, &c_symbols, 1,
					 total, scaling);
   }
   
   /*
    *  Fill 'wfa->weights' with decoded coefficients
    */
   {
      unsigned state, label;
      
      for (state = wfa->basis_states; state < wfa->states; state++)
	 for (label = 0; label < MAXLABELS; label++)
	    if (ischild (wfa->tree [state][label])
		&& isedge (wfa->into [state][label][0]))
	    {
	       wfa->weight [state][label][0] = btor (*ptr++,
						     wfa->wfainfo->dc_rpf);
	       wfa->int_weight [state][label][0]
		  = wfa->weight [state][label][0] * 512 + 0.5;
	    }
   }
   Free (coefficients);
}
