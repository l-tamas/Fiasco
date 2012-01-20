/*
 *  matrices.c:		Output of transitions matrices
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:50:31 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#include "config.h"

#if STDC_HEADERS
#	include <stdlib.h>
#endif /* not STDC_HEADERS */

#include "types.h"
#include "macros.h"
#include "error.h"

#include "wfa.h"
#include "bit-io.h"
#include "arith.h"
#include "misc.h"
#include "wfalib.h"

#include "matrices.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static unsigned
delta_encoding (bool_t use_normal_domains, bool_t use_delta_domains,
		const wfa_t *wfa, unsigned last_domain, bitfile_t *output);
static unsigned
column_0_encoding (const wfa_t *wfa, unsigned last_row, bitfile_t *output);
static unsigned
chroma_encoding (const wfa_t *wfa, bitfile_t *output);

/*****************************************************************************

				public code
  
*****************************************************************************/

unsigned
write_matrices (bool_t use_normal_domains, bool_t use_delta_domains,
		const wfa_t *wfa, bitfile_t *output)
/*
 *  Write transition matrices of 'wfa' to stream 'output'.
 *
 *  Return value:
 *	number of transitions encoded
 */
{
   unsigned root_state;			/* root of luminance */
   unsigned total = 0;			/* number of transitions */
   
   root_state = wfa->wfainfo->color
		? wfa->tree [wfa->tree [wfa->root_state][0]][0]
		: wfa->root_state;
   
   total  = column_0_encoding (wfa, root_state, output);

   total += delta_encoding (use_normal_domains, use_delta_domains,
			    wfa, root_state, output);
   
   if (wfa->wfainfo->color)
      total += chroma_encoding (wfa, output);
   
   return total;
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static unsigned
delta_encoding (bool_t use_normal_domains, bool_t use_delta_domains,
		const wfa_t *wfa, unsigned last_domain, bitfile_t *output)
/*
 *  Write transition matrices with delta coding to stream 'input'.
 *  'last_domain' is the maximum state number used as domain image.
 *
 *  Return value:
 *	number of non-zero matrix elements (WFA edges)
 */
{
   range_sort_t	rs;			/* ranges are sorted as in the coder */
   unsigned	max_domain;		/* dummy used for recursion */
   unsigned	total = 0;
      
   /*
    *  Generate a list of range blocks.
    *  The order is the same as in the coder.
    */
   rs.range_state      = fiasco_calloc ((last_domain + 1) * MAXLABELS,
				 sizeof (u_word_t));
   rs.range_label      = fiasco_calloc ((last_domain + 1) * MAXLABELS,
				 sizeof (byte_t));
   rs.range_max_domain = fiasco_calloc ((last_domain + 1) * MAXLABELS,
				 sizeof (u_word_t));
   rs.range_subdivided = fiasco_calloc ((last_domain + 1) * MAXLABELS,
				 sizeof (bool_t));
   rs.range_no	       = 0;
   max_domain 	       = wfa->basis_states - 1;
   sort_ranges (last_domain, &max_domain, &rs, wfa);
   
   /*
    *  Compute and write distribution of #edges
    */
   {
      unsigned state, label;
      unsigned edge;
      unsigned count [MAXEDGES + 1];
      unsigned n;
      unsigned edges = 0;
      unsigned M     = 0;
      unsigned bits  = bits_processed (output);
      
      for (n = 0; n < MAXEDGES + 1; n++)
	 count [n] = 0;
      
      for (state = wfa->basis_states; state <= last_domain; state++)
	 for (label = 0; label < MAXLABELS; label++)
	    if (isrange (wfa->tree [state][label]))
	    {
	       for (edge = 0; isedge (wfa->into [state][label][edge]); edge++)
		  ;
	       count [edge]++;
	       edges++;
	       M = max (edge, M);
	    }
      write_rice_code (M, 3, output);
      for (n = 0; n <= M; n++)
	 write_rice_code (count [n], (int) log2 (last_domain) - 2, output);

      /*
       *  Arithmetic coding of values
       */
      {
	 unsigned  range;
	 model_t  *elements = alloc_model (M + 1, 0, 0, count);
	 arith_t  *encoder  = alloc_encoder (output);
	       
	 for (range = 0; range < rs.range_no; range++)
	    if (!rs.range_subdivided [range])
	    {
	       state = rs.range_state [range];
	       label = rs.range_label [range];
	       for (edge = 0; isedge (wfa->into [state][label][edge]); edge++)
		  ;
	       
	       encode_symbol (edge, encoder, elements);
	    }
	 free_encoder (encoder);
	 free_model (elements);
      }
      debug_message ("delta-#edges: %5d bits. (%5d symbols => %5.2f bps)",
		     bits_processed (output) - bits, edges,
		     edges > 0 ? ((bits_processed (output) - bits) /
				  (double) edges) : 0);
   }

   /*
    *  Write matrix elements
    */
   {
      unsigned	bits  	 = bits_processed (output);
      u_word_t *mapping1 = fiasco_calloc (wfa->states, sizeof (u_word_t));
      u_word_t *mapping2 = fiasco_calloc (wfa->states, sizeof (u_word_t));
      unsigned	range;

      put_bit (output, use_normal_domains);
      put_bit (output, use_delta_domains);
      
      /*
       *  Generate array of states which are admitted domains.
       *  When coding intra frames 'mapping1' == 'mapping2' otherwise
       *  'mapping1' is a list of 'normal' domains which are admitted for 
       *             coding intra blocks
       *  'mapping2' is a list of 'delta' domains which are admitted for
       *             coding the motion compensated prediction error 
       */
      {
	 unsigned n1, n2, state;
	 
	 for (n1 = n2 = state = 0; state < wfa->states; state++)
	 {
	    mapping1 [state] = n1;
	    if (usedomain (state, wfa)
		&& (state < wfa->basis_states || use_delta_domains
		    || !wfa->delta_state [state]))
	       n1++;
	    
	    mapping2 [state] = n2;
	    if (usedomain (state, wfa)
		&& (state < wfa->basis_states || use_normal_domains
		    || wfa->delta_state [state]))
	       n2++;
	 }
	 debug_message ("# normal states = %d, # delta states = %d,"
			" # WFA states = %d", n1, n2, wfa->states);
      }
      
      for (range = 0; range < rs.range_no; range++)
	 if (!rs.range_subdivided [range])
	 {
	    unsigned  state = rs.range_state [range];
	    unsigned  label = rs.range_label [range];
	    unsigned  last  = 1;
	    u_word_t *mapping;
	    unsigned  max_value;
	    unsigned  edge;
	    word_t    domain;

	    if (wfa->delta_state [state] ||
		wfa->mv_tree [state][label].type != NONE)
	       mapping = mapping2;
	    else
	       mapping = mapping1;
	    
	    max_value = mapping [rs.range_max_domain [range]];
	    
	    for (edge = 0; isedge (domain = wfa->into [state][label][edge]);
		 edge++)
	       if (domain > 0)
	       {
		  total++;
		  if (max_value - last)
		  {
		     write_bin_code (mapping [domain] - last,
				     max_value - last, output);
		     last = mapping [domain] + 1;
		  }
	       }
	 }

      debug_message ("delta-index:  %5d bits. (%5d symbols => %5.2f bps)",
		     bits_processed (output) - bits, total,
		     total > 0 ? ((bits_processed (output) - bits) /
				  (double) total) : 0);
      fiasco_free (mapping1);
      fiasco_free (mapping2);
   }
   
   fiasco_free (rs.range_state);
   fiasco_free (rs.range_label);
   fiasco_free (rs.range_max_domain);
   fiasco_free (rs.range_subdivided);

   return total;
}

static unsigned
column_0_encoding (const wfa_t *wfa, unsigned last_row, bitfile_t *output)
/*
 *  Write column 0 of the transition matrices of the 'wfa' to stream 'output'
 *  with quasi arithmetic coding.
 *  All rows from 'wfa->basis_states' up to 'last_row' are decoded.
 *
 *  Return value:
 *	number of non-zero matrix elements (WFA edges)
 */
{
   u_word_t  high;			/* Start of the current code range */
   u_word_t  low;			/* End of the current code range */
   unsigned *prob;			/* probability array */
   unsigned  row;			/* current matrix row */
   unsigned  label;			/* current matrix label */
   unsigned  underflow;			/* Underflow bits */
   unsigned  index;			/* probability index */
   unsigned  total = 0;			/* Number of '1' elements */
   unsigned  bits  = bits_processed (output);

   /*
    *  Compute the probability array:
    *  prob[] = { 1/2, 1/2, 1/4, 1/4, 1/4, 1/4,
    *             1/8, ... , 1/16, ..., 1/(MAXPROB+1)}
    */
   {
      unsigned n;
      unsigned exp;			/* current exponent */
      
      prob = fiasco_calloc (1 << (MAX_PROB + 1), sizeof (unsigned));
   
      for (index = 0, n = MIN_PROB; n <= MAX_PROB; n++)
	 for (exp = 0; exp < 1U << n; exp++, index++)
	    prob [index] = n;
   }
   
   high      = HIGH;			/* 1.0 */
   low       = LOW;			/* 0.0 */
   underflow = 0;			/* no underflow bits */

   index = 0;

   /*
    *  Encode column 0 with a quasi arithmetic coder (QAC).
    *  Advantage of this QAC with respect to a binary AC:
    *  Instead of using time consuming multiplications and divisions
    *  to compute the probability of the most probable symbol (MPS) and
    *  the range of the interval, a table look up procedure linked
    *  with a shift operation is used for both computations.
    */
   for (row = wfa->basis_states; row <= last_row; row++)
      for (label = 0; label < MAXLABELS; label++)
	 if (isrange (wfa->tree [row][label]))
	 {
	    if (wfa->into [row][label][0] != 0)
	    {
	       /*
		*  encode the MPS '0'
		*/
	       high = high - ((high - low) >> prob [index]) - 1;
	       RESCALE_OUTPUT_INTERVAL;
	       
	       if (index < 1020)
		  index++;
	    }
	    else
	    {
	       /*
		*  encode the LPS '1'
		*/
	       low = high - ((high - low) >> prob [index]);

	       RESCALE_OUTPUT_INTERVAL;
	       
	       total++;
	       index >>= 1;
	    }
	 }
   /*
    *  Flush the quasi-arithmetic encoder
    */
   low = high;

   RESCALE_OUTPUT_INTERVAL;
   
   OUTPUT_BYTE_ALIGN (output);

   fiasco_free (prob);

   debug_message ("delta-state0: %5d bits. (%5d symbols => %5.2f bps)",
		  bits_processed (output) - bits, total,
		  total > 0 ? ((bits_processed (output) - bits) /
			       (double) total) : 0);

   return total;
}   

static unsigned
chroma_encoding (const wfa_t *wfa, bitfile_t *output)
/*
 *  Write transition matrices of 'wfa' states which are part of the
 *  chroma channels Cb and Cr to stream 'output'.
 *
 *  Return value:
 *	number of non-zero matrix elements (WFA edges)
 */
{

   unsigned  domain;			/* current domain, counter */
   unsigned  label;			/* current label */
   unsigned  total = 0;			/* number of '1' elements */
   u_word_t  high;			/* Start of the current code range */
   u_word_t  low;			/* End of the current code range */
   unsigned  underflow;			/* underflow bits */
   unsigned *prob;			/* probability array */
   unsigned  index;			/* probability index, counter */
   unsigned  next_index;		/* probability of last domain */
   unsigned  row;			/* current matrix row */
   word_t   *y_domains;
   unsigned  count = 0;			/* number of transitions for part 1 */
   unsigned  bits  = bits_processed (output);
   
   /*
    *  Compute the asymmetric probability array
    *  prob[] = { 1/2, 1/2, 1/4, 1/4, 1/4, 1/4,
    *                     1/8, ... , 1/16, ..., 1/(MAXPROB+1)}
    */
   {
      unsigned n;
      unsigned exp;			/* current exponent */
      
      prob = fiasco_calloc (1 << (MAX_PROB + 1), sizeof (unsigned));
   
      for (index = 0, n = MIN_PROB; n <= MAX_PROB; n++)
	 for (exp = 0; exp < 1U << n; exp++, index++)
	    prob [index] = n;
   }
   
   high      = HIGH;			/* 1.0 */
   low       = LOW;			/* 0.0 */
   underflow = 0;			/* no underflow bits */

   next_index = index = 0;

   y_domains = compute_hits (wfa->basis_states,
			     wfa->tree [wfa->tree [wfa->root_state][0]][0],
			     wfa->wfainfo->chroma_max_states, wfa);

   /*
    *  First of all, read all matrix columns given in the list 'y_domains'
    *  which note all admitted domains.
    *  These matrix elements are stored with QAC (see column_0_encoding ()).
    */
   for (domain = 0; y_domains [domain] != -1; domain++)
   {
      bool_t save_index = YES;		/* YES: store current prob. index */
      
      row   = wfa->tree [wfa->tree [wfa->root_state][0]][0] + 1;
      index = next_index;
	 
      for (; row < wfa->states; row++)
      {
	 for (label = 0; label < MAXLABELS; label++)
	    if (isrange (wfa->tree [row][label]))
	    {
	       unsigned	edge;
	       int	into;
	       bool_t    match;		/* approx with current domain found */
	       
	       for (match = NO, edge = 0;
		    isedge (into = wfa->into [row][label][edge])
			    && (unsigned) into < row;
		    edge++)
		  if (into == y_domains [domain]
		      && into != wfa->y_state [row][label])
		     match = YES;
	       if (!match)
	       {
		  /*
		   *  encode the MPS '0'
		   */
		  high = high - ((high - low) >> prob [index]) - 1;

		  RESCALE_OUTPUT_INTERVAL;
		     
		  if (index < 1020)
		     index++;
	       }
	       else
	       {
		  /*
		   *  encode the LPS '1'
		   */
		  low = high - ((high - low) >> prob [index]);

		  RESCALE_OUTPUT_INTERVAL;
		     
		  total++;
		  index >>= 1;
	       }
	    }
	 if (save_index)
	 {
	    next_index = index;
	    save_index = NO;
	 }
      }
   }

   debug_message ("CbCr_matrix:  %5d bits. (%5d symbols => %5.2f bps)",
		  bits_processed (output) - bits, total,
		  total > 0 ? ((bits_processed (output) - bits) /
			       (double) total) : 0);
   count = total;
   bits  = bits_processed (output);
   
   /*
    *  Encode the additional column which indicates whether there
    *  are transitions to a state with same spatial coordinates
    *  in the Y component.
    *
    *  Again, quasi arithmetic coding is used for this task.
    */

   next_index = index = 0;

   for (row = wfa->tree [wfa->tree [wfa->root_state][0]][0] + 1;
	row < wfa->states; row++)
      for (label = 0; label < MAXLABELS; label++)
	 if (!wfa->y_column [row][label])
	 {
	    /*
	     *  encode the MPS '0'
	     */
	    high = high - ((high - low) >> prob [index]) - 1;

	    RESCALE_OUTPUT_INTERVAL;
	    
	    if (index < 1020)
	       index++;
	 }
	 else
	 {
	    /*
	     *  encode the LPS '1'
	     */
	    low = high - ((high - low) >> prob [index]);

	    RESCALE_OUTPUT_INTERVAL;

	    index >>= 1;
	    total++;
	 }

   /*
    *  Flush the quasi-arithmetic encoder
    */
   low = high;

   RESCALE_OUTPUT_INTERVAL;
   OUTPUT_BYTE_ALIGN (output);

   debug_message ("Yreferences:  %5d bits. (%5d symbols => %5.2f bps)",
		  bits_processed (output) - bits, total - count,
		  total - count > 0 ? ((bits_processed (output) - bits) /
				       (double) (total - count)) : 0);

   fiasco_free (prob);
   fiasco_free (y_domains);
   
   return total;
}
