/*
 *  mc.c:	Input of motion compensation	
 *
 *  written by: Michael Unger
 *		Ullrich Hafner
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

#include "wfa.h"
#include "bit-io.h"
#include "misc.h"

#include "mc.h"

static int mv_code_table [33][2] =
/*
 *  MPEG's huffman code for vector components. Format: code_value, length
 */
{
   {0x19, 11}, {0x1b, 11}, {0x1d, 11}, {0x1f, 11}, {0x21, 11}, {0x23, 11},
   {0x13, 10}, {0x15, 10}, {0x17, 10}, {0x7, 8}, {0x9, 8}, {0xb, 8}, {0x7, 7},
   {0x3, 5}, {0x3, 4}, {0x3, 3}, {0x1, 1}, {0x2, 3}, {0x2, 4}, {0x2, 5},
   {0x6, 7}, {0xa, 8}, {0x8, 8}, {0x6, 8}, {0x16, 10}, {0x14, 10}, {0x12, 10}, 
   {0x22, 11}, {0x20, 11}, {0x1e, 11}, {0x1c, 11}, {0x1a, 11}, {0x18, 11}
};

/*****************************************************************************

			     local variables
  
*****************************************************************************/

typedef struct huff_node 
{
   int		     code_index;	/* leaf if index >= 0 */
   struct huff_node *left;		/* follow if '0' bit read */
   struct huff_node *right;		/* follow if '1' bit read */
   int		     index_set [34];
} huff_node_t;

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
decode_mc_tree (frame_type_e frame_type, unsigned max_state,
		wfa_t *wfa, bitfile_t *input);
static void
decode_mc_coords (unsigned max_state, wfa_t *wfa, bitfile_t *input);
static int
get_mv (int f_code, huff_node_t *hn, bitfile_t *input);
static huff_node_t *
create_huff_tree (void);
static void
create_huff_node (huff_node_t *hn, int bits_processed);

/*****************************************************************************

				public code
  
*****************************************************************************/

void
read_mc (frame_type_e frame_type, wfa_t *wfa, bitfile_t *input)
/*
 *  Read motion compensation information of the 'input' stream.
 *  Depending on 'frame_type' different decoding methods are used.
 *
 *  No return value.
 *
 *  Side effects:
 *	'wfa->mv_tree' is filled with the decoded values.
 */
{
   unsigned max_state = wfa->wfainfo->color
			? wfa->tree [wfa->tree [wfa->root_state][0]][0]
			: wfa->states;

   decode_mc_tree (frame_type, max_state, wfa, input);
   decode_mc_coords (max_state, wfa, input);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
decode_mc_tree (frame_type_e frame_type, unsigned max_state,
		wfa_t *wfa, bitfile_t *input)
/*
 *  Read tree of motion compensation decisions of the 'input' stream.
 *  Depending on 'frame_type' different decoding methods are used.
 *  'max_state' is the last state with motion compensation infos.
 *
 *  No return value.
 *
 *  Side effects:
 *	'wfa->mv_tree' is filled with decoded values.
 */
{
   unsigned  state;			/* current state */
   unsigned *queue;			/* states in breadth first order */
   unsigned  last;			/* last node
					   (update for each new node) */

   /*
    *  Traverse tree in breadth first order (starting at level
    *  'wfa->wfainfo->p_max_level'). Use a queue to store the childs
    *  of each node ('last' is the next free queue element).  
    */
   queue = Calloc (MAXSTATES, sizeof (unsigned));
   for (last = 0, state = wfa->basis_states; state < max_state; state++)
      if (wfa->level_of_state [state] - 1 == (int) wfa->wfainfo->p_max_level)
	 queue [last++] = state;	/* init level 'p_max_level' */

   if (frame_type == P_FRAME)
   {
      unsigned label;			/* current label */
      unsigned current;			/* current node to process */
      
      for (current = 0; current < last; current++)
	 for (label = 0; label < MAXLABELS; label++)
	 {
	    state = queue[current];
	    if (wfa->x [state][label]	/* process visible states only */
		+  width_of_level (wfa->level_of_state [state] - 1)
		<= wfa->wfainfo->width
		&&
		wfa->y [state][label]
		+  height_of_level (wfa->level_of_state [state] - 1)
		<= wfa->wfainfo->height)
	    {
	       wfa->mv_tree [state][label].type
		  = get_bit (input) ? NONE : FORWARD;
	    }
	    else
	       wfa->mv_tree [state][label].type = NONE;
	    if (wfa->mv_tree [state][label].type == NONE &&
		!isrange (wfa->tree [state][label]) &&
		wfa->level_of_state [state] - 1 >=
		(int) wfa->wfainfo->p_min_level) 
	       queue [last++] = wfa->tree [state][label]; /* append child  */
	 }
   }
   else
   {
      unsigned label;			/* current label */
      unsigned current;			/* current node to process */
      
      for (current = 0; current < last; current++)
	 for (label = 0; label < MAXLABELS; label++)
	 {
	    state = queue[current];
	    if (wfa->x [state][label]	/* process visible states only */
		+ width_of_level (wfa->level_of_state [state] - 1)
		> wfa->wfainfo->width
		||
		wfa->y [state][label]
		+ height_of_level (wfa->level_of_state [state] - 1)
		> wfa->wfainfo->height)
	       wfa->mv_tree[state][label].type = NONE;
	    else if (get_bit (input))	/* 1   */
	       wfa->mv_tree[state][label].type = NONE;
	    else if (get_bit (input))	/* 01  */
	       wfa->mv_tree[state][label].type = INTERPOLATED;
	    else if (get_bit (input))	/* 001 */ 
	       wfa->mv_tree[state][label].type = BACKWARD;
	    else			/* 000 */ 
	       wfa->mv_tree[state][label].type = FORWARD;
	    if (wfa->mv_tree[state][label].type == NONE &&
		!isrange (wfa->tree[state][label]) &&
		wfa->level_of_state[state] - 1
		>= (int) wfa->wfainfo->p_min_level) 
	       queue[last++] = wfa->tree[state][label]; /* append child  */
	 }
   }
   
   INPUT_BYTE_ALIGN (input);
   Free (queue);
}

static void
decode_mc_coords (unsigned max_state, wfa_t *wfa, bitfile_t *input)
/*
 *  Read motion vector coordinates of the 'input' stream. They are stored
 *  with the static Huffman code of the MPEG and H.263 standards.
 *  'max_state' is the last state with motion compensation infos.
 *
 *  No return value.
 *
 *  Side effects:
 *	'wfa->mv_tree' is filled with decoded values.
 */
{
   unsigned	       label;		/* current label */
   unsigned	       state;		/* current state */
   mv_t		      *mv;		/* current motion vector */
   static huff_node_t *huff_mv_root = NULL; /* root of huffman tree */
 
   if (huff_mv_root == NULL)
      huff_mv_root = create_huff_tree ();
   
   for (state = wfa->basis_states; state < max_state; state++)
      for (label = 0; label < MAXLABELS; label++)
      {
	 mv = &wfa->mv_tree[state][label];
	 switch (mv->type)
	 {
	    case NONE:
	       break;
	    case FORWARD:
	       mv->fx = get_mv (1, huff_mv_root, input);
	       mv->fy = get_mv (1, huff_mv_root, input);
	       break;	    
	    case BACKWARD:	    
	       mv->bx = get_mv (1, huff_mv_root, input);
	       mv->by = get_mv (1, huff_mv_root, input);
	       break;	    
	    case INTERPOLATED:   
	       mv->fx = get_mv (1, huff_mv_root, input);
	       mv->fy = get_mv (1, huff_mv_root, input);
	       mv->bx = get_mv (1, huff_mv_root, input);
	       mv->by = get_mv (1, huff_mv_root, input);
	       break;
	 }
      }

   INPUT_BYTE_ALIGN (input);
}
 
static int
get_mv (int f_code, huff_node_t *hn, bitfile_t *input)
/* 
 *  Decode next motion vector component in bitstream 
 *  by traversing the huffman tree.
 */
{
   int vlc_code, vlc_code_magnitude, residual, diffvec;

   while (hn->code_index < 0)
   {
      if (hn->code_index == -2)
	 error ("wrong huffman code !");
      if (get_bit (input))
	 hn = hn->right;
      else
	 hn = hn->left;
   }
   vlc_code = hn->code_index - 16;
   if (vlc_code == 0 || f_code == 1) 
      return vlc_code;

   vlc_code_magnitude = abs (vlc_code) - 1;
   if (f_code <= 1)
      residual = 0;
   else
      residual = get_bits (input, f_code - 1);
   diffvec = (vlc_code_magnitude << (f_code - 1)) + residual + 1;
   
   return vlc_code > 0 ? diffvec : - diffvec;
}

static huff_node_t *
create_huff_tree (void)
/*
 *  Construct huffman tree from code table
 */
{
   unsigned	i;
   huff_node_t *huff_root = Calloc (1, sizeof (huff_node_t));
   
   /*
    *  The nodes' index set contains indices of all codewords that are
    *  still decodable by traversing further down from the node.
    *  (The root node has the full index set.)
    */

   for (i = 0; i < 33; i++)
      huff_root->index_set [i] = i;
   huff_root->index_set [i] = -1;	/* end marker */

   create_huff_node (huff_root, 0);

   return huff_root;
}

static void
create_huff_node (huff_node_t *hn, int bits_processed)
/*
 *  Create one node in the huffman tree
 */
{
   int lind = 0;			/* next index of left huff_node */
   int rind = 0;			/* next index of right huff_node */
   int code_len, i, ind;

   hn->code_index = -1;
   if (hn->index_set [0] < 0)		/* empty index set ? */
   {
      hn->code_index = -2;		/* error */
      return;
   }
   hn->left  = Calloc (1, sizeof (huff_node_t));
   hn->right = Calloc (1, sizeof (huff_node_t));

   for (i = 0; (ind = hn->index_set[i]) >= 0; i++)
   {
      code_len = mv_code_table[ind][1];
      if (code_len == bits_processed)	/* generate leaf */
      {
	 hn->code_index = ind;
	 Free (hn->left); 
	 Free (hn->right);
	 return;
      }
      if (mv_code_table[ind][0] & (1 << (code_len - 1 - bits_processed)))
	 hn->right->index_set[rind++] = ind;
      else
	 hn->left->index_set[lind++] = ind;
   }
   hn->right->index_set[rind] = -1;	/* set end markers */
   hn->left->index_set[lind]  = -1;
   create_huff_node (hn->left, bits_processed + 1);
   create_huff_node (hn->right, bits_processed + 1);
}
