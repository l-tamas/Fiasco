/*
 *  matrices.c:		Input of transition matrices
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

#include "matrices.h"

#if STDC_HEADERS
#	include <stdlib.h>
#endif /* not STDC_HEADERS */

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static unsigned
delta_decoding (wfa_t *wfa, unsigned last_domain, bitfile_t *input);
static unsigned
column_0_decoding (wfa_t *wfa, unsigned last_row, bitfile_t *input);
static unsigned
chroma_decoding (wfa_t *wfa, bitfile_t *input);
static void
compute_y_state (int state, int y_state, wfa_t *wfa);

/*****************************************************************************

				public code
  
*****************************************************************************/

unsigned
read_matrices (wfa_t *wfa, bitfile_t *input)
/* 
 *  Read transitions of WFA given from the stream 'input'.
 *
 *  Return value:
 *	number of edges
 *
 *  Side effects:
 *	'wfa->into' is filled with decoded values 
 */
{
   unsigned total;			/* total number of edges in the WFA */
   unsigned root_state = wfa->wfainfo->color
			 ? wfa->tree [wfa->tree [wfa->root_state][0]][0]
			 : wfa->root_state;

   total  = column_0_decoding (wfa, root_state, input);
   total += delta_decoding (wfa, root_state, input);
   if (wfa->wfainfo->color)
      total += chroma_decoding (wfa, input);
       
   return total;
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static unsigned
delta_decoding (wfa_t *wfa, unsigned last_domain, bitfile_t *input)
/*
 *  Read transition matrices which are encoded with delta coding
 *  from stream 'input'.
 *  'last_domain' is the maximum state number used as domain image.
 *
 *  Return value:
 *	number of non-zero matrix elements (WFA edges)
 *
 *  Side effects:
 *	'wfa->into' is filled with decoded values 
 */
{
   range_sort_t	 rs;			/* ranges are sorted as in the coder */
   unsigned	 max_domain;		/* dummy used for recursion */
   unsigned	 range;
   unsigned	 count [MAXEDGES + 1];
   unsigned 	 state, label;
   unsigned	*n_edges;		/* number of elements per row */
   unsigned	 total = 0;		/* total number of decoded edges */

   /*
    *  Generate a list of range blocks.
    *  The order is the same as in the coder.
    */
   rs.range_state      = Calloc ((last_domain + 1) * MAXLABELS,
				 sizeof (u_word_t));
   rs.range_label      = Calloc ((last_domain + 1) * MAXLABELS,
				 sizeof (byte_t));
   rs.range_max_domain = Calloc ((last_domain + 1) * MAXLABELS,
				 sizeof (u_word_t));
   rs.range_subdivided = Calloc ((last_domain + 1) * MAXLABELS,
				 sizeof (bool_t));
   rs.range_no	       = 0;
   max_domain 	       = wfa->basis_states - 1;
   sort_ranges (last_domain, &max_domain, &rs, wfa);

   /*
    *  Get row statistics
    */
   {
      arith_t  *decoder;
      model_t  *elements;
      unsigned 	max_edges = read_rice_code (3, input);
      
      /*
       *  Get the probability array of the number of edges distribution
       *  and allocate the corresponding model.
       */
      {
	 unsigned edge;
	 
	 for (edge = 0; edge <= max_edges; edge++)
	    count [edge] = read_rice_code ((int) log2 (last_domain) - 2,
					   input);
	 elements = alloc_model (max_edges + 1, 0, 0, count);
      }
      
      /*
       *  Get number of elements per matrix row
       */
      {
	 unsigned row;
      
	 n_edges = Calloc (wfa->states, sizeof (unsigned));
	 decoder = alloc_decoder (input);
	 for (row = range = 0; range < rs.range_no; range++)
	    if (!rs.range_subdivided [range])
	    {
	       state = rs.range_state [range];
	       label = rs.range_label [range];
	       
	       n_edges [row++]
		  = decode_symbol (decoder, elements)
		  - (isedge (wfa->into [state][label][0]) ? 1 : 0);
	    }
	 
	 free_decoder (decoder);
	 free_model (elements);
      }
   }
   
   /*
    *  Get matrix elements
    */
   {
      unsigned row;
      u_word_t *mapping1           = Calloc (wfa->states, sizeof (word_t));
      u_word_t *mapping_coder1     = Calloc (wfa->states, sizeof (word_t));
      u_word_t *mapping2           = Calloc (wfa->states, sizeof (word_t));
      u_word_t *mapping_coder2     = Calloc (wfa->states, sizeof (word_t));
      bool_t	use_normal_domains = get_bit (input);
      bool_t	use_delta_domains  = get_bit (input);
	  
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
	    mapping1 [n1] = state;
	    mapping_coder1 [state] = n1;
	    if (usedomain (state, wfa)
		&& (state < wfa->basis_states
		    || use_delta_domains || !wfa->delta_state [state]))
	       n1++;
	    
	    mapping2 [n2] = state;
	    mapping_coder2 [state] = n2;
	    if (usedomain (state, wfa)
		&& (state < wfa->basis_states || use_normal_domains
		    || wfa->delta_state [state]))
	       n2++;
	 }
      }
	 
      for (row = 0, range = 0; range < rs.range_no; range++)
	 if (!rs.range_subdivided [range])
	 {
	    u_word_t *mapping;
	    u_word_t *mapping_coder;
	    unsigned  max_value;
	    unsigned  edge;
	    unsigned  state = rs.range_state [range];
	    unsigned  label = rs.range_label [range];
	    unsigned  last  = 1;

	    if (wfa->delta_state [state] ||
		wfa->mv_tree [state][label].type != NONE)
	    {
	       mapping 	     = mapping2;
	       mapping_coder = mapping_coder2;
	    }
	    else
	    {
	       mapping 	     = mapping1;
	       mapping_coder = mapping_coder1;
	    }
	    max_value = mapping_coder [rs.range_max_domain [range]];
	    for (edge = n_edges [row]; edge; edge--)
	    {
	       unsigned domain;

	       if (max_value - last)
		  domain = read_bin_code (max_value - last, input) + last;
	       else
		  domain = max_value;
	       append_edge (state, mapping [domain], -1, label, wfa);
	       last = domain + 1;
	       total++;
	    }
	    row++;
	 }
      Free (mapping1);
      Free (mapping_coder1);
      Free (mapping2);
      Free (mapping_coder2);
   }
      
   Free (n_edges);
   Free (rs.range_state);
   Free (rs.range_label);
   Free (rs.range_max_domain);
   Free (rs.range_subdivided);

   return total;
}

static unsigned
column_0_decoding (wfa_t *wfa, unsigned last_row, bitfile_t *input)
/*
 *  Read column 0 of the transition matrices of the 'wfa' which are coded
 *  with quasi arithmetic coding from stream 'input'.
 *  All rows from 'wfa->basis_states' up to 'last_row' are decoded.
 * 
 *  Return value:
 *	number of non-zero matrix elements (WFA edges)
 *
 *  Side effects:
 *	'wfa->into' is filled with decoded values 
 */
{
   unsigned  row;			/* current matrix row */
   unsigned  total = 0;			/* total number of edges in col 0 */
   unsigned *prob_ptr;			/* pointer to current probability */
   unsigned *last;			/* pointer to minimum probability */
   unsigned *first;			/* pointer to maximum probability */
   unsigned *new_prob_ptr;		/* ptr to probability of last domain */
   unsigned *prob;			/* probability array */
   u_word_t  high;			/* Start of the current code range */
   u_word_t  low;			/* End of the current code range */
   u_word_t  code;			/* The present input code value */
   word_t   *is_leaf;			/* pointer to the tree structure */

   /*
    *  Compute the asymmetric probability array
    *  prob[] = { 1/2, 1/2, 1/4, 1/4, 1/4, 1/4,
    *             1/8, ... , 1/16, ..., 1/(MAXPROB+1)}
    */
   {
      unsigned n;
      unsigned index;			/* probability index */
      unsigned exp;			/* current exponent */
      
      prob = Calloc (1 << (MAX_PROB + 1), sizeof (unsigned));
   
      for (index = 0, n = MIN_PROB; n <= MAX_PROB; n++)
	 for (exp = 0; exp < 1U << n; exp++, index++)
	    prob [index] = n;
   }

   first = prob_ptr = new_prob_ptr = prob;
   last  = first + 1020;
   
   is_leaf = wfa->tree [wfa->basis_states]; /* use pointer arithmetics ... */

   high = HIGH;				/* 1.0 */
   low  = LOW;				/* 0.0 */
   code = get_bits (input, 16);		

   /*
    *  Decode column 0 with a quasi arithmetic coder (QAC).
    *  Advantage of this QAC with respect to a binary AC:
    *  Instead of using time consuming multiplications and divisions
    *  to compute the probability of the most probable symbol (MPS) and
    *  the range of the interval, a table look up procedure linked
    *  with a shift operation is used for both computations.
    *
    *  Loops and array accesses have been removed
    *  to make real time decoding possible.
    */
   for (row = wfa->basis_states; row <= last_row; row++)
   {
      unsigned count;			/* value in the current interval */
      
      /*
       *  Read label 0 element
       */
      if (isrange (*is_leaf++))		/* valid matrix index */
      {
	 count = high - ((high - low) >> *prob_ptr);
	 if (code < count)
	 {
	    if (prob_ptr < last)	/* model update */
	       prob_ptr++;
	    /*
	     *  Decode the MPS '0'
	     */
	    high = count - 1;

	    RESCALE_INPUT_INTERVAL;
	 }
	 else
	 {
	    prob_ptr = ((prob_ptr - first) >> 1) + first; /* model update */
	    /*
	     *  Decode the LPS '1'
	     */
	    low = count;

	    RESCALE_INPUT_INTERVAL;
	    /*
	     *  Restore the transition (weight = -1)
	     */
	    append_edge (row, 0, -1, 0, wfa);
	    total++;
	 }
      }
      /*
       *  Read label 1 element
       */
      if (isrange (*is_leaf++)) /* valid matrix index */
      {
	 count = high - ((high - low) >> *prob_ptr);
	 if (code < count)
	 {
	    if (prob_ptr < last)
	       prob_ptr++;		/* model update */
	    /*
	     *  Decode the MPS '0'
	     */
	    high = count - 1;

	    RESCALE_INPUT_INTERVAL;
	 }
	 else
	 {
	    prob_ptr = ((prob_ptr - first) >> 1) + first; /* model update */
	    /*
	     *  Decode the LPS '1'
	     */
	    low = count;

	    RESCALE_INPUT_INTERVAL;
	    /*
	     *  Restore the transition (weight = -1)
	     */
	    append_edge (row, 0, -1, 1, wfa);
	    total++;
	 }
      }
   }

   INPUT_BYTE_ALIGN (input);

   Free (prob);
   
   return total;
}

static unsigned
chroma_decoding (wfa_t *wfa, bitfile_t *input)
/*
 *  Read transition matrices of 'wfa' states which are part of the
 *  chroma channels Cb and Cr from stream 'input'.
 *
 *  Return value:
 *	number of non-zero matrix elements (WFA edges)
 *
 *  Side effects:
 *	'wfa->into' is filled with decoded values 
 */
{
   unsigned  domain;			/* current domain, counter */
   unsigned  total = 0;			/* total number of chroma edges */
   unsigned *prob_ptr;			/* pointer to current probability */
   unsigned *last;			/* pointer to minimum probability */
   unsigned *first;			/* pointer to maximum probability */
   unsigned *new_prob_ptr;		/* ptr to probability of last domain */
   unsigned *prob;			/* probability array */
   u_word_t  high;			/* Start of the current code range */
   u_word_t  low;			/* End of the current code range */
   u_word_t  code;			/* The present input code value */
   word_t   *y_domains;			/* domain images corresponding to Y */
   int	     save_index;		/* YES: store current probabilty */

   /*
    *  Compute the asymmetric probability array
    *  prob[] = { 1/2, 1/2, 1/4, 1/4, 1/4, 1/4,
    *                     1/8, ... , 1/16, ..., 1/(MAXPROB+1)}
    */
   {
      unsigned n;
      unsigned index;			/* probability index */
      unsigned exp;			/* current exponent */
      
      prob = Calloc (1 << (MAX_PROB + 1), sizeof (unsigned));
   
      for (index = 0, n = MIN_PROB; n <= MAX_PROB; n++)
	 for (exp = 0; exp < 1U << n; exp++, index++)
	    prob [index] = n;
   }

   high = HIGH;				/* 1.0 */
   low  = LOW;				/* 0.0 */
   code = get_bits (input, 16);

   /*
    *  Compute list of admitted domains
    */
   y_domains = compute_hits (wfa->basis_states,
			     wfa->tree [wfa->tree [wfa->root_state][0]][0],
			     wfa->wfainfo->chroma_max_states, wfa);
   
   first = prob_ptr = new_prob_ptr = prob;
   last  = first + 1020;

   /*
    *  First of all, read all matrix columns given in the list 'y_domains'
    *  which note all admitted domains.
    *  These matrix elements are stored with QAC (see column_0_decoding ()).
    */
   for (domain = 0; y_domains [domain] != -1; domain++)
   {
      unsigned 	row	= wfa->tree [wfa->tree [wfa->root_state][0]][0] + 1;
      word_t   *is_leaf = wfa->tree [row];

      prob_ptr   = new_prob_ptr;
      save_index = YES;

      for (; row < wfa->states; row++)
      {
	 unsigned count;		/* value in the current interval */
	 /*
	  *  Read label 0 element
	  */
	 if (isrange (*is_leaf++)) 	/* valid matrix index */
	 {
	    count = high - ((high - low) >> *prob_ptr);
	    if (code < count)
	    {
	       if (prob_ptr < last)
		  prob_ptr++;
	       /*
		*  Decode the MPS '0'
		*/
	       high = count - 1;

	       RESCALE_INPUT_INTERVAL;
	    }
	    else
	    {
	       prob_ptr = ((prob_ptr - first) >> 1) + first;
	       /*
		*  Decode the LPS '1'
		*/
	       low = count;

	       RESCALE_INPUT_INTERVAL;
	       /*
		*  Restore the transition (weight = -1)
		*/
	       append_edge (row, y_domains [domain], -1, 0, wfa);
	       total++;
	    }
	 }
	 /*
	  *  Read label 1 element
	  */
	 if (isrange (*is_leaf++)) /* valid matrix index */
	 {
	    count = high - ((high - low) >> *prob_ptr);
	    if (code < count)
	    {
	       if (prob_ptr < last)
		  prob_ptr++;
	       /*
		*  Decode the MPS '0'
		*/
	       high = count - 1;

	       RESCALE_INPUT_INTERVAL;
	    }
	    else
	    {
	       prob_ptr = ((prob_ptr - first) >> 1) + first;
	       /*
		*  Decode the LPS '1'
		*/
	       low = count;

	       RESCALE_INPUT_INTERVAL;
	       /*
		*  Restore the transition (weight = -1)
		*/
	       append_edge (row, y_domains [domain], -1, 1, wfa);
	       total++;
	    }
	 }
	 if (save_index)
	 {
	    save_index 	 = NO;
	    new_prob_ptr = prob_ptr;
	 }
      }
   }

   Free (y_domains);

   compute_y_state (wfa->tree [wfa->tree [wfa->root_state][0]][1],
		    wfa->tree [wfa->tree [wfa->root_state][0]][0], wfa);
   compute_y_state (wfa->tree [wfa->tree [wfa->root_state][1]][0],
		    wfa->tree [wfa->tree [wfa->root_state][0]][0], wfa);
   
   first = prob_ptr = new_prob_ptr = prob;

   /*
    *  Decode the additional column which indicates whether there
    *  are transitions to a state with same spatial coordinates
    *  in the Y component.
    *
    *  Again, quasi arithmetic decoding is used for this task.
    */
   {
      unsigned 	row;
      
      for (row = wfa->tree [wfa->tree [wfa->root_state][0]][0] + 1;
	   row < wfa->states; row++)
      {
	 int label;			/* current label */

	 for (label = 0; label < MAXLABELS; label++)
	 {
	    u_word_t count = high - ((high - low) >> *prob_ptr);

	    if (code < count)
	    {
	       if (prob_ptr < last)
		  prob_ptr++;
	       /*
		*  Decode the MPS '0'
		*/
	       high = count - 1;

	       RESCALE_INPUT_INTERVAL;
	    }
	    else
	    {
	       prob_ptr = ((prob_ptr - first) >> 1) + first;
	       /*
		*  Decode the LPS '1'
		*/
	       low = count;

	       RESCALE_INPUT_INTERVAL;
	       /*
		*  Restore the transition (weight = -1)
		*/
	       append_edge (row, wfa->y_state [row][label], -1, label, wfa);
	       total++;
	    }
	 }
      }
   }

   INPUT_BYTE_ALIGN (input);

   Free (prob);

   return total;
}

static void
compute_y_state (int state, int y_state, wfa_t *wfa)
/*
 *  Compute the 'wfa->y_state' array which denotes those states of
 *  the Y band that have the same spatial coordinates as the corresponding
 *  states of the Cb and Cr bands.
 *  The current root of the Y tree is given by 'y_state'.
 *  The current root of the tree of the chroma channel is given by 'state'.
 *
 *  No return value.
 *
 *  Side effects:
 *	'wfa->y_state' is filled with the generated tree structure.
 */
{
   unsigned label;
   
   for (label = 0; label < MAXLABELS; label++)
      if (isrange (y_state))
	 wfa->y_state [state][label] = RANGE;
      else
      {
	 wfa->y_state [state][label] = wfa->tree [y_state][label];
	 if (!isrange (wfa->tree [state][label]))
	    compute_y_state (wfa->tree [state][label],
			     wfa->y_state [state][label], wfa);
      }
      
}
