/*
 *  tree.c:		Output of bintree partitioning
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
#include "bit-io.h"
#include "arith.h"
#include "misc.h"

#include "tree.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
encode_tree (bitfile_t *output, const byte_t *data, unsigned n_data,
	     unsigned scaling, u_word_t sum0, u_word_t sum1);

/*****************************************************************************

				public code
  
*****************************************************************************/

void
write_tree (const wfa_t *wfa, bitfile_t *output)
/*
 *  Write bintree to stream 'output'.
 *  Traverse tree in breadth first order and save a '1' for each child
 *  and a '0' for each range image.
 *
 *  No return value.
 */
{
   unsigned  queue [MAXSTATES];		/* state numbers in BFO */
   unsigned  current;			/* current node to process */
   unsigned  last;			/* last node (update every new node) */
   unsigned  label;			/* current label */
   int	     into;			/* next child */
   byte_t   *tree_string;		/* bitstring to encode */
   unsigned  total = 0;			/* number of ranges */
   unsigned  bits  = bits_processed (output); /* number of bits */

   /*
    *  Traverse tree in breadth first order. Use a queue to store
    *  the childs of each node ('last' is the next free queue element).
    *  The first element ('current') of this queue will get the new parent
    *  node. 
    */
   tree_string = Calloc (MAXSTATES * MAXLABELS, sizeof (byte_t));
   queue [0] = wfa->root_state;
   for (last = 1, current = 0; current < last; current++)
      for (label = 0; label < MAXLABELS; label++)
	 if (!isrange (into = wfa->tree [queue[current]][label])) /* child ? */
	 {
	    queue [last++]        = into;
	    tree_string [total++] = 1;
	 }
	 else				/* or range ? */
	    tree_string [total++] = 0;

   if (total != (wfa->states - wfa->basis_states) * MAXLABELS)
      error ("total [%d] != (states - basis_states) * 2 [%d]", total,
	     (wfa->states - wfa->basis_states) * MAXLABELS);
   
   {
      unsigned scale = total / 20 ;

      encode_tree (output, tree_string, total, scale, 1, 11);
   }

   Free (tree_string);
   
   debug_message ("tree:         %5d bits. (%5d symbols => %5.2f bps)",
		  bits_processed (output) - bits, total,
		  total > 0 ? ((bits_processed (output) - bits)
			       / (double) total) : 0);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
encode_tree (bitfile_t *output, const byte_t *data, unsigned n_data,
	     unsigned scaling, u_word_t sum0, u_word_t sum1)
/*
 *  Encode bintree data with adaptive binary arithmetic coding.
 *  Write 'n_data' output symbols stored in 'data' to stream 'output'.
 *  Rescale probability model after every 'scaling' symbols.
 *  Initial counts are given by 'sum0' and 'sum1'.
 *
 *  No return value.
 */
{
   u_word_t low;			/* Start of the current code range */
   u_word_t high;			/* End of the current code range */
   u_word_t underflow;			/* Number of underflow bits pending */
   unsigned n;				/* Data counter */

   low       = 0;
   high      = 0xffff;
   underflow = 0;

   for (n = n_data; n; n--)
   {
      unsigned range;			/* Current interval range */
      
      if (!*data++)
      {
	 /*
	  *  encode a '0'
	  */
	 range =  (high - low) + 1;
	 high  = low + (u_word_t) ((range * sum0) / sum1 - 1);

	 RESCALE_OUTPUT_INTERVAL;

	 sum0++;
      }
      else
      {
	 /*
	  *  encode a '1'
	  */
	 range =  (high - low) + 1;
	 low   = low + (u_word_t) ((range * sum0) / sum1);

	 RESCALE_OUTPUT_INTERVAL;
      }
      /*
       *  Update the frequency counts
       */
      sum1++;
      if (sum1 > scaling) /* Scale the symbol frequencies */
      {
	 sum0 >>= 1;
	 sum1 >>= 1;
	 if (!sum0)
	    sum0 = 1;
	 if (sum0 >= sum1)
	    sum1 = sum0 + 1;
      }
   }
   /*
    *  Flush the quasi-arithmetic encoder
    */
   low = high;

   RESCALE_OUTPUT_INTERVAL;

   OUTPUT_BYTE_ALIGN (output);
}
