/*
 *  mc.c:		Output of motion compensation	
 *
 *  Written by:		Michael Unger
 *			Ullrich Hafner
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

#include "mc.h"

int mv_code_table [33][2];		/* VLC table for coordinates, mwfa.c */

/*****************************************************************************

			     local variables
  
*****************************************************************************/

static unsigned p_frame_codes [4][2] =
/*
 *  Code values and bits for P-frame prediction
 *  NONE,  FORWARD
 */
{
   {1, 1}, {0, 1}, {0, 0}, {0, 0} 
};

static unsigned b_frame_codes [4][2] =
/*
 *  Code values and bits for B-frame prediction
 *  NONE,  FORWARD,  BACKWARD, INTERPOLATED
 */
{
   {1, 1}, {000, 3}, {001, 3}, {01, 2} 
};

enum vlc_e {CODE = 0, BITS = 1};

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
encode_mc_tree (unsigned max_state, frame_type_e frame_type, const wfa_t *wfa,
	       bitfile_t *output);
static void
encode_mc_coords (unsigned max_state, const wfa_t *wfa, bitfile_t *output);

/*****************************************************************************

				public code
  
*****************************************************************************/

void
write_mc (frame_type_e frame_type, const wfa_t *wfa, bitfile_t *output)
{
   unsigned max_state = wfa->wfainfo->color
			? wfa->tree[wfa->tree[wfa->root_state][0]][0]
			: wfa->states;

   encode_mc_tree (max_state, frame_type, wfa, output);
   encode_mc_coords (max_state, wfa, output);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
encode_mc_tree (unsigned max_state, frame_type_e frame_type, const wfa_t *wfa,
		bitfile_t *output)
/*
 *  Write tree of motion compensation decisions to the 'output' stream.
 *  Depending on 'frame_type' different decoding methods are used.
 *  'max_state' is the last state with motion compensation infos.
 *
 *  No return value.
 */
{
   unsigned  label;			/* current label */
   unsigned  state;			/* current state */
   unsigned  total = 0;			/* number of motion tree decisions */
   unsigned  queue [MAXSTATES];		/* state numbers in BFO */
   unsigned  current;			/* current node to process */
   unsigned  last;			/* last node (update every new node) */
   mc_type_e type;			/* type of motion compensation */
   unsigned	     (*mc_tree_codes)[2]; /* pointer to VLC table */
   unsigned  bits  = bits_processed (output); /* number of bits used */
   
   if (frame_type == P_FRAME)
      mc_tree_codes = p_frame_codes;	/* binary code */
   else 
      mc_tree_codes = b_frame_codes;	/* variable length code */
   
   /*
    *  Traverse tree in breadth first order (starting at
    *  level 'wfa->p_max_level'). Use a queue to store the childs
    *  of each node ('last' is the next free queue element).  
    */

   for (last = 0, state = wfa->basis_states; state < max_state; state++)
      if (wfa->level_of_state [state] - 1 == (int) wfa->wfainfo->p_max_level)
	 queue [last++] = state;	/* init level = 'mc_max_level' */
   
   for (current = 0; current < last; current++)
      for (label = 0; label < MAXLABELS; label++)
      {
	 state = queue [current];
	 type  = wfa->mv_tree [state][label].type;
	 if (wfa->x [state][label]
	     + width_of_level (wfa->level_of_state [state] - 1)
	     <= wfa->wfainfo->width
	     &&
	     wfa->y [state][label]
	     + height_of_level (wfa->level_of_state [state] - 1)
	     <= wfa->wfainfo->height)
	 {
	    put_bits (output, mc_tree_codes [type][CODE],
		      mc_tree_codes [type][BITS]);
	    total++;
	 }
	 if (type == NONE && !isrange (wfa->tree [state][label]) &&
	     wfa->level_of_state [state] - 1 >=
	     (int) wfa->wfainfo->p_min_level)
	    queue [last++] = wfa->tree [state][label]; /* append child */
	 
      }

   OUTPUT_BYTE_ALIGN (output);
   debug_message ("mc-tree:      %5d bits. (%5d symbols => %5.2f bps)",
		  bits_processed (output) - bits, total,
		  total > 0 ? ((bits_processed (output) - bits) /
			       (double) total) : 0);
}

static void
encode_mc_coords (unsigned max_state, const wfa_t *wfa, bitfile_t *output)
/*
 *  Write motion vector coordinates to the 'output' stream. They are stored
 *  with the static Huffman code of the MPEG and H.263 standards.
 *  'max_state' is the last state with motion compensation infos.
 *
 *  No return value.
 */
{
   unsigned  state;			/* current state */
   unsigned  label;			/* current label */
   unsigned  level_count [MAXLEVEL];	/* number of mv per level */
   unsigned  level;			/* counter */
   unsigned  ftotal = 0;		/* #forward motion tree decisions */
   unsigned  btotal = 0;		/* #backward decisions */
   unsigned  itotal = 0;		/* #interpolated decisions */
   unsigned  bits   = bits_processed (output); /* number of bits used */
   unsigned  sr     = wfa->wfainfo->search_range; /* search range */
   
   for (level = wfa->wfainfo->p_max_level;
	level >= wfa->wfainfo->p_min_level; level--)
      level_count [level] = 0;
   
   for (state = wfa->basis_states; state < max_state; state++)
      for (label = 0; label < MAXLABELS; label++)
      {
	 mv_t *mv = &wfa->mv_tree[state][label]; /* motion vector info */
	 
	 if (mv->type != NONE)
	 {
	    level_count [wfa->level_of_state [state] - 1]++;
	    switch (mv->type)
	    {
	       case FORWARD:
		  put_bits (output,
			    mv_code_table[(mv->fx + sr)][CODE],
			    mv_code_table[(mv->fx + sr)][BITS]);
		  put_bits (output,
			    mv_code_table[(mv->fy + sr)][CODE],
			    mv_code_table[(mv->fy + sr)][BITS]);
		  ftotal++;
		  break;
	       case BACKWARD:
		  put_bits (output,
			    mv_code_table[(mv->bx + sr)][CODE],
			    mv_code_table[(mv->bx + sr)][BITS]);
		  put_bits (output,
			    mv_code_table[(mv->by + sr)][CODE],
			    mv_code_table[(mv->by + sr)][BITS]);
		  btotal++;
		  break;
	       case INTERPOLATED:
		  put_bits (output,
			    mv_code_table[(mv->fx + sr)][CODE],
			    mv_code_table[(mv->fx + sr)][BITS]);
		  put_bits (output,
			    mv_code_table[(mv->fy + sr)][CODE],
			    mv_code_table[(mv->fy + sr)][BITS]);
		  put_bits (output,
			    mv_code_table[(mv->bx + sr)][CODE],
			    mv_code_table[(mv->bx + sr)][BITS]);
		  put_bits (output,
			    mv_code_table[(mv->by + sr)][CODE],
			    mv_code_table[(mv->by + sr)][BITS]);
		  itotal++;
		  break;
	       default:
		  break;
	    }
	 }
      }

   OUTPUT_BYTE_ALIGN (output);
   
   debug_message ("Motion compensation: %d forward, %d backward, "
		  "%d interpolated", ftotal, btotal, itotal);

   for (level = wfa->wfainfo->p_max_level;
	level >= wfa->wfainfo->p_min_level; level--)
      debug_message ("Level %d: %d motion vectors", level, level_count[level]);
   
   {
      unsigned  total = ftotal * 2 + btotal * 2 + itotal * 4;

      debug_message ("mv-coord:     %5d bits. (%5d symbols => %5.2f bps)",
		     bits_processed (output) - bits, total,
		     total > 0 ? ((bits_processed (output) - bits) /
				  (double) total) : 0);
   }

   return;
}
