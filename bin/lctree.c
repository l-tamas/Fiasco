/*      
 *  lctree.c:		Tree implementation for not restricted LC-lists
 *
 *  Written by:         Martin Grimm
 *
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */
 
/*
 *  $Date: 2000/07/15 17:20:59 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#include "config.h"

#include "types.h"
#include "macros.h"
#include "error.h"

#include <math.h>

#include "misc.h"

#include "twfa.h"
#include "tlist.h"
#include "fig.h"
#include "lctree.h"

void
Init_LCTree (const wfa_t *wfa, LCTree_t *LCTree, toptions_t *options,
	     int *lrw_to_lwr)
/*
 *  Initialize LCTree-struct, allocate memory, set all values to default
 *
 *  wfa        - pointer on wfa-tree containing the data
 *  LCTree     - pointer on LCTree-struct to initialize
 *  options    - pointer on options-struct for settings
 *  lrw_to_lwr - pointer on field for transformation from lrw to lwr
 *		 ordered state numbers
 */
{
   int i, j;

   /* initialize LCTree */
   LCTree->root_state   = lrw_to_lwr[options->root_state];
   LCTree->nr_of_states = wfa->root_state;
   LCTree->basis_states = wfa->basis_states;
   LCTree->root_origin  = NULL;

   /* get origin of root */
   GetOrigin (wfa, &(LCTree->root_origin), options->root_state);
   
   /* initalize LCTree->states */
   LCTree->states = Calloc (wfa->root_state + 1, sizeof (state_t));
   for (i = 0; i <= wfa->root_state; i++)
   {
      LCTree->states[i].state_nr   = RANGE;
      LCTree->states[i].x          = 0;
      LCTree->states[i].y          = 0;
      LCTree->states[i].multistate = RANGE;
      
      for(j = 0; j < MAXLABELS; j++)
      {
	 LCTree->states [i].child[j]       = RANGE;
	 LCTree->states [i].mc[j]          = NONE;
	 LCTree->states [i].tree_out[j]    = NULL;
	 LCTree->states [i].basis_out[j]    = NULL;
	 LCTree->states [i].tree_in[j]     = NULL;
	 LCTree->states [i].tree_out_nr[j] = 0;
	 LCTree->states [i].basis_out_nr[j] = 0;
	 LCTree->states [i].tree_in_nr[j]  = 0;
      }
   }

   /* initialize basis states */
   for (i = 0; i < wfa->basis_states; i++)
      LCTree->states[i].state_nr = lrw_to_lwr[i];
}

void
Build_LCTree (const wfa_t *wfa, LCTree_t *LCTree, toptions_t *options,
	      int local_root, int *lrw_to_lwr, int depth)
/*
 *  recursive construction of the new LCTree out of the wfa-tree
 *
 *  wfa        - pointer on wfa-tree containing the data
 *  LCTree     - pointer on LCTree-struct to build
 *  options    - pointer on options-struct for settings
 *  local_root - indetifies the current root state in this recursive call
 *  lrw_to_lwr - pointer on field for transformation from lrw to lwr
 *		 ordered state numbers
 *  depth       - depth in new tree (0 means root)
 */
{
   int i;
   int child[MAXLABELS];
   
   for (i = 0; i < MAXLABELS; i++)
      child [i] = wfa->tree [local_root][i];
   
   /* recursively insert childs in LCTree */
   for (i = 0; i < MAXLABELS; i++)
      if (ischild (child[i]))
	 Build_LCTree (wfa, LCTree, options, child[i], lrw_to_lwr, depth + 1);
   
   /* insert root in states and set depth */
   LCTree->states [lrw_to_lwr[local_root]].state_nr = local_root;
   LCTree->states [lrw_to_lwr[local_root]].depth    = depth;

   /* work to do for each subimage */
   for (i = 0; i < MAXLABELS; i++)
   {
      int j = 0;
      
      if (ischild(child[i]))
      {
	 /* store child numbers and motion compensation */
	 LCTree->states [lrw_to_lwr[local_root]].child[i]
	    = lrw_to_lwr[child[i]];
	 LCTree->states [lrw_to_lwr[local_root]].mc[i]
	    = wfa->mv_tree[local_root][i].type;
      }

      /* add linear combinations divided in edges to tree and edges to basis */
      while (isedge (wfa->into[local_root][i][j]))
      {
	 int edge = wfa->into [local_root][i][j];
	 if (edge < wfa->basis_states)
	 {
	    LCTree->states [lrw_to_lwr[local_root]].basis_out_nr[i]++;
	    InsertAscList (&(LCTree->states[lrw_to_lwr[local_root]].basis_out[i]),
			   lrw_to_lwr[edge], 1);
	 }
	 else if (edge <= options->root_state)
	 {
	    LCTree->states [lrw_to_lwr[local_root]].tree_out_nr[i]++;
	    InsertDesList (&(LCTree->states[lrw_to_lwr[local_root]].tree_out[i]),
			   lrw_to_lwr[edge], 1);
	 }
	 j++;
      }
   }
}

void
Remove_LCTree (LCTree_t *LCTree)
/*
 *  Free allocated memory of LCTree
 *
 *  LCTree - pointer on tree-struct to destroy
 */
{
   int i, j;
  
   /* remove linear combination lists from LCTree->states */
   for (i = 0; i <= LCTree->root_state; i++)
   {
      for(j = 0; j < MAXLABELS; j++)
      {
	 if (LCTree->states[i].tree_out[j])
	    RemoveList (LCTree->states[i].tree_out[j]);
	 if (LCTree->states[i].basis_out[j])
	    RemoveList (LCTree->states[i].basis_out[j]);
	 if (LCTree->states[i].tree_in[j])
	    RemoveList (LCTree->states[i].tree_in[j]);
      }
   }

   /* remove states */
   Free (LCTree->states);

   /* remove list containing the root's ancestors */
   if (LCTree->root_origin)
      RemoveList (LCTree->root_origin);
}


int
CalcLCTreeDepth (LCTree_t *LCTree, int local_root)
/*
 *  Recursively calculate the depth of the LCTree.
 *  This can vary from wfa through limitations in depth and so ...
 *
 *  LCTree     - pointer on LCTree
 *  local_root - root state of this step in recursion
 */
{
   int max_depth = 0; 
   int i;

   for (i = 0; i < MAXLABELS; i++)
      if (ischild (LCTree->states [local_root].child [i]))
	 max_depth = max (max_depth,
			  CalcLCTreeDepth (LCTree,
					   LCTree->states[local_root].child[i])
			  + 1);
   
   return max_depth;
}

void
ReplaceTargetState (LCTree_t *LCTree, int local_root, int state, int motherstate)
/*
 *  Recursively replace linear combinations to state through l.c. to motherstate
 *  in whole tree. This is necessary when joining a subtree to its root and
 *  removing the subtree from the tree. Therefore all linear combinations
 *  in this subtree must be redirected to the remaining multistate.
 *
 *  LCTree      - pointer on LCTree to work on
 *  local_root  - root of this recursion step
 *  state       - state to replace
 *  motherstate - new target of the l.c.
 */
{
   int i;
   
   for (i = 0; i < MAXLABELS; i++)
   {
      tlist_t** pointer = &(LCTree->states [local_root].tree_out [i]);
      tlist_t*  pos = LCTree->states [local_root].tree_out [i];
      int	counter = 0;

      /* first remove all l.c.s from the list counting their number */
      while (pos)
      {
	 if (pos->value == state)
	 {
	    tlist_t *tmp = pos->next;
	    counter += pos->count;
	    *pointer = pos->next;
	    Free (pos);
	    pos = tmp;
	 }
	 else
	 {
	    pointer = &pos->next;
	    pos     = pos->next;
	 }
      }

      /* if l.c.s where found,
	 insert counted l.c.s in list, but with new target state */
      if (counter > 0)
	 InsertDesList (&(LCTree->states [local_root].tree_out [i]), motherstate,
			counter);

   }

   /* recursively do it for all childs */
   
   for (i = 0; i < MAXLABELS; i++)
      if (ischild (LCTree->states [local_root].child[i]))
	 ReplaceTargetState (LCTree, LCTree->states [local_root].child [i],
			     state, motherstate);
}

void
JoinStates (LCTree_t *LCTree, int state, int motherstate)
/*
 *  Recursively reduce a subtree to its root state by joining all
 *  linear combinations to the root_state "motherstate" and removing the rest
 *  of the subtree from the tree.
 *
 *  LCTree      - pointer on LCTree to work on
 *  state       - state in this recursion step that is joined to motherstate
 *  motherstate - state the subtree is joined to
 */
{
   int i;
   /*
    *  Store range of states joined in motherstate, this is different for normal
    *  states and states that already represent a subtree
    */
   if (isrange (LCTree->states [state].multistate))
   {
      if (LCTree->states [motherstate].multistate
	  > LCTree->states [state].state_nr)
	 LCTree->states [motherstate].multistate
	    = LCTree->states [state].state_nr;
   }
   else
   {
      if (LCTree->states [motherstate].multistate
	  > LCTree->states [state].multistate)
	 LCTree->states [motherstate].multistate
	    = LCTree->states [state].multistate;
   }
      
   /* copy all l.c.s to the motherstate */
   for (i = 0; i < MAXLABELS; i++)
   {
      tlist_t* pos = LCTree->states [state].tree_out [i];
      while (pos)
      {
	 InsertDesList (&(LCTree->states [motherstate].tree_out [i]), pos->value,
			pos->count);
	 LCTree->states [motherstate].tree_out_nr[i] += pos->count;
	 pos = pos->next;
      }
      pos = LCTree->states [state].basis_out [i];
      while (pos)
      {
	 InsertAscList (&(LCTree->states [motherstate].basis_out [i]), pos->value,
			pos->count);
	 LCTree->states [motherstate].basis_out_nr [i] += pos->count;
	 pos = pos->next;
      }
   }
   
   /* recursively add the childs */
   for (i = 0; i < MAXLABELS; i++)
      if (ischild (LCTree->states [state].child [i]))
	 JoinStates (LCTree, LCTree->states [state].child [i], motherstate);

   /* remove this state from the tree */
   LCTree->states [state].state_nr = RANGE;
   for (i = 0; i < MAXLABELS; i++)
   {
      LCTree->states [state].child [i] = RANGE;
      if (LCTree->states [state].tree_out [i])
	 RemoveList (LCTree->states [state].tree_out [i]);
      if (LCTree->states [state].basis_out [i])
	 RemoveList(LCTree->states [state].basis_out [i]);
      if (LCTree->states [state].tree_in [i])
	 RemoveList(LCTree->states [state].tree_in [i]);
      LCTree->states [state].tree_out [i]    = NULL;
      LCTree->states [state].basis_out [i]    = NULL;
      LCTree->states [state].tree_in [i]     = NULL;
      LCTree->states [state].tree_out_nr [i] = 0;
      LCTree->states [state].basis_out_nr [i] = 0;
      LCTree->states [state].tree_in_nr [i]  = 0;
   }

   /* redirect all edges to this state to motherstate */
   ReplaceTargetState (LCTree, LCTree->root_state, state, motherstate);
   
}

void
CreateMultiState (LCTree_t *LCTree, int state)
/*
 *  Replace a whole subtree through one multistate, this is used when
 *  the tree is limited by depth or through other restrictions
 *
 *  LCTree - pointer on LCTree to work on
 *  state  - state, that is converted from a subtree to a multistate
 */
{
   int i;
   int childs = NO;

   /* check for childs */
   for (i = 0; i < MAXLABELS; i++)
      if (ischild (LCTree->states [state].child [i]))
	 childs = YES;
   
   if (childs)
      LCTree->states [state].multistate = LCTree->states [state].state_nr;

   /* join childs to state and remove them from the tree */
   for (i = 0; i < MAXLABELS; i++)
      if (ischild (LCTree->states [state].child [i]))
      {
	 JoinStates (LCTree, LCTree->states [state].child [i], state);
	 LCTree->states [state].child [i] = RANGE;
      }

   /* remove linear combinations that go from this multistate in itself */
   RemoveInternalLC (LCTree, state);
}

void
RemoveInternalLC (LCTree_t *LCTree, int state)
/*
 *  Remove edges from this state that point on the state itself.
 *  this happens when a subtree is melted to a multistate.
 *  only tree_out edges are killed, because basiss aren't melted
 *  and the tree_in lists aren't yet calculated when this function
 *  is executed.
 *
 *  LCTree - pointer to LCTree to work on
 *  state  - state to check
 */
{
   int i;
   
   for (i = 0; i < MAXLABELS; i++)
   {
      tlist_t** pointer = &(LCTree->states[state].tree_out[i]);
      tlist_t*	pos = *pointer;
      int	counter = 0;

      /* first remove all edges on this state */
      while (pos)
      {
	 if (pos->value == state)
	 {
	    counter += pos->count;
	    *pointer = pos->next;
	    Free (pos);
	    pos = *pointer;
	 }
	 else
	 {
	    pointer = &pos->next;
	    pos     = pos->next;
	 }
      }
      /* then set the counter to the new value */
      if (counter > 0)
	 LCTree->states [state].tree_out_nr [i] -= counter;
   }
}

void
RemoveLowerLC (LCTree_t* LCTree, int limit_state, int local_root)
/*
 *  Recursively remove edges from the tree into states, that aren't anymore
 *  in the tree.
 *  this happens when only a subtree is drawn, then there are edges to states
 *  with a lower ID than that of most left leaf in the specified subtree possible.
 *  only tree_out edges are killed, because basiss aren't part of a subtree and are
 *  handled separately.
 *
 *  LCTree       - pointer to LCTree to work on
 *  limit_state  - ID of the  most left leaf in the drawn subtree,
 *		   all lower ID's are removed
 *  local_root   - root state in this recursion step
 */
{
   int i;
   
   for (i = 0; i < MAXLABELS; i++)
   {
      tlist_t** pointer = &(LCTree->states [local_root].tree_out [i]);
      tlist_t*	pos = *pointer;
      int	counter = 0;
      /* first remove all edges to states with a lower ID than limit_state */
      while (pos)
      {
	 if (pos->value < limit_state)
	 {
	    counter += pos->count;
	    *pointer = pos->next;
	    Free (pos);
	    pos = *pointer;
	 }
	 else
	 {
	    pointer = &pos->next;
	    pos     = pos->next;
	 }
      }
      /* then set the counter to the new value */
      if (counter > 0)
	 LCTree->states [local_root].tree_out_nr [i] -= counter;

      /* recursively do the same with all childs */
      if(ischild (LCTree->states [local_root].child [i]))
	 RemoveLowerLC (LCTree, limit_state,
			LCTree->states [local_root].child [i]);
   }
}

void
AdjustLC (const wfa_t *wfa, LCTree_t *LCTree, toptions_t *options)
/*
 *  Remove not wished linear combination edges from the tree.
 *  this is necessary when only a subtree is drawn or if only
 *  edges from specified states should be drawn.
 *
 *  wfa     - pointer on wfa-tree, neede for comparison of root states
 *  LCTree  - pointer on LCTree to work on
 *  options - pointer on options-struct
 */
{
   int pos;

   /* if only a subtree should be drawn ...*/
   if (wfa->root_state != LCTree->states [LCTree->root_state].state_nr)
   {
      pos = LCTree->root_state;
      /* search the most left state in the subtree */
      while (ischild (LCTree->states [pos].child [0]))
	 pos = LCTree->states [pos].child [0];
      /* remove all edges to states with a lower ID than this */
      RemoveLowerLC (LCTree, pos, LCTree->root_state);
   }


   /* check all states in tree */
   for (pos = LCTree->basis_states; pos <= LCTree->nr_of_states; pos++)
   {
      int i;
      
      if (!isrange (LCTree->states [pos].state_nr)) /* used state ? */
      {
	 /*
	  * if a list is used to specify the states
	  * edges should be shown from, remove all edges from
	  * states not in this list
	  */
	 if (options->LC_tree_list
	     && !SearchAscList (options->LC_tree_list,
				LCTree->states [pos].state_nr))
	    for(i = 0; i < MAXLABELS; i++)
	    {
	       if (LCTree->states [pos].tree_out [i])
		  RemoveList (LCTree->states [pos].tree_out [i]);
	       LCTree->states [pos].tree_out [i] = NULL;
	       LCTree->states [pos].tree_out_nr [i] = 0;
	    }
	 if (options->LC_basis_list
	     && !SearchAscList  (options->LC_basis_list,
				 LCTree->states [pos].state_nr))
	    for(i = 0; i < MAXLABELS; i++)
	    {
	       if (LCTree->states [pos].basis_out [i])
		  RemoveList (LCTree->states [pos].basis_out [i]);
	       LCTree->states [pos].basis_out [i] = NULL;
	       LCTree->states [pos].basis_out_nr [i] = 0;
	    }
      }
   }
}


void
Depth_limit_LCTree (LCTree_t *LCTree, int local_root, int depth)
/*
 *  Recursivly limit the tree to a specified depth joining all subtrees
 *  at the highest allowed depth to multistates representating a subtree.
 *
 *  LCTree     - pointer on LCTree to work on
 *  local_root - root state of this recursion step
 *  depth       - depth countdown, 0 means highest allowed depth reached
 */
{
   int i;

   /* if max depth reached transform subtree to multistate */
   if (depth <= 0)
      CreateMultiState (LCTree, local_root);
   /* else recursively climb down the tree */
   else
   {
      depth--;
      for (i = 0; i < MAXLABELS; i++)
	 if (ischild (LCTree->states [local_root].child [i]))
	    Depth_limit_LCTree (LCTree, LCTree->states [local_root].child [i],
				depth);
   }
}

void
LC_limit_LCTree (LCTree_t *LCTree, int local_root)
/*
 *  This function represents the option -c, that transforms a subtree
 *  to a multistate, if the first linear combination out of its root
 *  is detected by recursively climbing down the tree
 *
 *  LCTree - pointer on LCTree to work on
 *  local_root - root state of this recursion step
 */
{
   int i;
   int lc_exist = NO;

   /* check for linear combinations */
   for (i = 0; i < MAXLABELS; i++)
   {
      if (LCTree->states [local_root].tree_out [i])
	 lc_exist = YES;
      if (LCTree->states [local_root].basis_out [i])
	 lc_exist = YES;
   }

   /* create multistate, if some exists, else recursively climb down */
   if (lc_exist)
      CreateMultiState (LCTree, local_root);
   else
      for (i = 0; i < MAXLABELS; i++)
	 if (ischild (LCTree->states [local_root].child [i]))
	    LC_limit_LCTree (LCTree, LCTree->states [local_root].child [i]);
}

void
CalcTreeCoordinates (LCTree_t *LCTree)
/*
 *  Calculate the coordinates of the tree states in the xfig-image.
 *  this won't be done symetrically, because this would lead to very
 *  wide trees with almost no states on large distances, but each state
 *  in lwr-order will be drawn more right than its preceding state
 *  a predefined distance.
 *  the y-coordinates can be calculated out of the stored depth of each state.
 *
 *  LCTree - pointer on LCTree to work on
 */
{
   int i;
   int j = X_TREE_MARGIN;

   /* go through all tree states in lwr-order */
   for (i = LCTree->basis_states; i <= LCTree->nr_of_states; i++)
   {
      if (!isrange(LCTree->states [i].state_nr))
	 /* if valid state, set coordinates */
      {
	 LCTree->states[i].x = j;
	 LCTree->states[i].y = LCTree->states[i].depth * Y_STATE_DISTANCE
			       + Y_TREE_MARGIN;
	 j += X_STATE_DISTANCE;
      }
   }
}

void
CalcBasisCoordinates (LCTree_t *LCTree, int y_offset)
/*
 *  Calculate the coordinates of the basis states in the xfig-image.
 *  the x-position depends on if the tree is drawn or not, if not, the basis
 *  will be centered on the predefined sheet width, else it will be drawn with
 *  a predefined offset to the left margin of the sheet.
 *  the y-coordinate depends on a offset that gives the coordinates of the most
 *  down piece of the tree drawing.
 *
 *  LCTree   - pointer on LCTree to work on
 *  y_offset - last "occupied" y_coordinate
 *  options  - pointer on options-struct
 */
{
   int i;
   int dx, dy;

   dx = X_BASIS_MARGIN;
   dy = y_offset + Y_BASIS_MARGIN;

   /* calculate coordinates for all basis states */
   for (i = 0; i < LCTree->basis_states; i++)
   {
      LCTree->states [i].x = dx;
      LCTree->states [i].y = dy;
      dy += Y_BASIS_DISTANCE;      
   }
}

void
GetOrigin (const wfa_t *wfa, tlist_t **state_list, int searchstate)
/*
 *  Get origin of the root state of the LCTree in the wfa-tree
 *  (only makes sense if the LCTree is a real subtree of the wfa-tree).
 *  the path from the root of the wfa-tree to the root of the LCTree
 *  is stored in a descending ordered tlist_t-list.
 *
 *  wfa         - pointer on wfa-tree to search in
 *  state_list  - pointer on pointer on head of origin list
 *  searchstate - state to search for
 */
{
   int pos = wfa->root_state;
   
   if (pos != searchstate)
   {
      while (pos != searchstate)
      {
	 int i = 0;
	 /* store position */
	 InsertDesList (state_list, pos, 1);
	 /* search for subtree with searchstate */
	 while (i < MAXLABELS)
	 {
	    if (ischild (wfa->tree [pos][i]) && wfa->tree [pos][i] >= searchstate)
	    {
	       pos = wfa->tree [pos][i];
	       i = MAXLABELS; /* terminate while-loop */
	    }
	    else i++;
	 }
      }
      InsertDesList (state_list, pos, 1); /* add searchstate to list */
   }
}

int
GetThick (int count)
/*
 *  Return the line-thickness of count lines depending on preset values.
 *
 *  count - # of lines to draw
 *  return value: thickness of line to draw
 */
{
   if (count<=MAX_COUNT_1)
      return COUNT_1_THICK;
   else if (count<=MAX_COUNT_2)
      return COUNT_2_THICK;
   else if (count<=MAX_COUNT_3)
      return COUNT_3_THICK;
   else
      return COUNT_4_THICK;
}

void
DrawState (FILE *out, state_t *state, toptions_t *options, int color)
/*
 *  Decide what kind of state to draw and draw it to the xfig-file.
 *  different kind of states are normal states, states with motion compensation
 *  and multistates representing a whole subtree.
 *
 *  out     - xfig-file to draw the state in
 *  state   - pointer on state to draw
 *  options - pointer on options-struct
 *  color   - background color for state
 */
{
   /* decide what kind of state to draw */
   if (isrange (state->multistate))
   {
      if (state->mc [0] == NONE && state->mc [1] == NONE)
	 DrawSingleState (out, state->x, state->y, state->state_nr,
			  options, color);
      else
	 DrawMCState(out, state->x, state->y, state->state_nr, options,
		     state->mc[0], state->mc[1], color);
   }
   else
      DrawMultistate(out,state->x, state->y, state->multistate, state->state_nr,
		     options, color);
}

void
DrawSingleState (FILE *out, int x, int y, int state, toptions_t *options,
		 int color)
/*
 *  Draw a normal state to the xfig-file, using shadows and state number,
 *  if set in options. shape of a normal state is a circle.
 *
 *  out     - xfig-file to draw the state in
 *  x, y    - coordinates of the center of the state
 *  state   - state number
 *  options - pointer on options struct
 *  color   - background color to use or DEFAULT
 */
{
   if (options->with_shadows)
      xfig_circle (out, x + SHADOW_OFFSET, y + SHADOW_OFFSET, STATE_RADIUS,
		   SHADOW_COLOR, SHADOW_COLOR, STATE_LEVEL + 1);

   if (color != DEFAULT)
      xfig_circle (out, x, y, STATE_RADIUS, STATE_COLOR, color, STATE_LEVEL);
   else
      xfig_circle (out, x, y, STATE_RADIUS, STATE_COLOR, STATE_FILL_COLOR,
		   STATE_LEVEL);
   
   if (options->state_text)
      fprintf (out, "4 1 %d %d 0 0 %d 0.0000 4 50 120 %d %d %d\\001\n",
	       TEXT_COLOR, TEXT_LEVEL, STATE_RADIUS / 20, x + 5, y + 25, state);
}

void
DrawMCState (FILE *out, int x, int y, int state, toptions_t *options,
	     int mc1, int mc2, int color)
/*
 *  Draw a motion compensation state to the xfig-file, using shadows and
 *  state number, if set in options and also small arrows above the state
 *  to indicate the direction of m.c. for each subimage. shape of a mc state
 *  is a rectangle.
 *
 *  out      - xfig-file to draw the state in
 *  x, y     - coordinates of the center of the state
 *  state    - state number
 *  options  - pointer on options struct
 *  mc1, mc2 - motion compensation vectors of subimages 0 and 1
 *  color    - background color to use or DEFAULT
 */
{
   int left, right;

   /*
    * draw shape
    */
   
   if (options->with_shadows)
      xfig_centerbox (out, x + SHADOW_OFFSET, y + SHADOW_OFFSET, STATE_RADIUS,
		      STATE_RADIUS, SHADOW_COLOR, SHADOW_COLOR, STATE_LEVEL + 1);

   if (color != DEFAULT)
      xfig_centerbox (out, x, y, STATE_RADIUS, STATE_RADIUS, MV_STATE_COLOR,
		      color, STATE_LEVEL);
   else
      xfig_centerbox (out, x, y, STATE_RADIUS, STATE_RADIUS, MV_STATE_COLOR,
		      MV_STATE_FILL_COLOR, STATE_LEVEL);     

   /*
    * draw arrows ...
    */

   /* ... for left subimage */
   if (mc1 == BACKWARD || mc1 == INTERPOLATED)
      left = 1;
   else
      left = 0;
   if (mc1 == FORWARD || mc1 == INTERPOLATED)
      right = 1;
   else
      right = 0;
   if (left + right)
   {
      fprintf (out, "2 1 0 1 %d -1 %d 0 20 0.000 0 1 7 %d %d 2\n", LEFT_MV_COLOR,
	       STATE_LEVEL, left, right);
      if (left)
	 fprintf (out, "\t 1 1 1 20 20\n");
      if (right)
	 fprintf (out, "\t 1 1 1 20 20\n");
      fprintf (out, "\t %d %d %d %d\n", x - 3 * STATE_RADIUS / 2,
	       y - STATE_RADIUS - 20, x - 10, y - STATE_RADIUS - 20);
   }
      
   /* ... for right subimage */
   if (mc2 == BACKWARD || mc2 == INTERPOLATED)
      left = 1;
   else
      left = 0;
   if (mc2 == FORWARD || mc2 == INTERPOLATED)
      right = 1;
   else
      right = 0;
   if (left + right)
   {
      fprintf (out, "2 1 0 1 %d -1 %d 0 20 0.000 0 1 7 %d %d 2\n",
	       RIGHT_MV_COLOR, STATE_LEVEL, left, right);
      if (left)
	 fprintf (out, "\t 1 1 1 20 20\n");
      if (right)
	 fprintf (out, "\t 1 1 1 20 20\n");
      fprintf (out, "\t %d %d %d %d\n",
	       x + 10, y - STATE_RADIUS - 20,
	       x + 3 * STATE_RADIUS / 2, y - STATE_RADIUS - 20);
   }

   /*
    *  Draw state text
    */

   if (options->state_text)
      fprintf (out, "4 1 %d %d 0 0 %d 0.0000 4 50 120 %d %d %d\\001\n",
	       TEXT_COLOR, TEXT_LEVEL, STATE_RADIUS / 20, x + 5, y + 25,
	       state);      
}

void
DrawMultistate (FILE *out, int x, int y, int state1, int state2,
		toptions_t *options, int color)
/*
 *  Draw a multistate to the xfig-file, using shadows and state numbers,
 *  if set in options. shape of a multistate is a triangle.
 *  The state text left of the triangle means the first state ID in the
 *  multistate, the text on the right means the last (its the root)  
 *
 *  out            - xfig-file to draw the state in
 *  x, y           - coordinates of the top of the triangle
 *  state1, state2 - state numbers specifiing the range of the represented subtree
 *  options        - pointer on options struct
 *  color          - background color to use or DEFAULT
 */
{
   if (options->with_shadows)
      xfig_triangle (out, x + SHADOW_OFFSET,y + SHADOW_OFFSET,
		     MULTISTATE_HEIGHT, MULTISTATE_WIDTH, SHADOW_COLOR,
		     SHADOW_COLOR, STATE_LEVEL + 1);
   if (color != DEFAULT)
      xfig_triangle (out, x, y, MULTISTATE_HEIGHT, MULTISTATE_WIDTH,
		     MULTISTATE_COLOR, color, STATE_LEVEL);
   else
      xfig_triangle (out, x, y, MULTISTATE_HEIGHT, MULTISTATE_WIDTH,
		     MULTISTATE_COLOR, MULTISTATE_FILL_COLOR, STATE_LEVEL);

   if (options->state_text)
      fprintf(out, "4 1 %d %d 0 0 %d 0.0000 4 50 120 %d %d %d     %d\\001\n",
	      TEXT_COLOR, TEXT_LEVEL, STATE_RADIUS / 20,
	      x + 5, y + 25, state1, state2);
}

void
DrawOrigin (FILE* out, int x, int y, tlist_t *list)
/*
 *  Draw the list of states to from the wfa-root to the LCTree-root
 *  centered over the LCTree-root
 *
 *  out  - xfig-file to draw in
 *  x, y - coordinates of the LCTree-root
 *  list - pointer to the list containing the ancestors of the LCTree-root
 */
{
   if (list)
   {
      fprintf (out, "4 1 %d %d 0 0 6 0.0000 4 50 120 %d %d ",
	       TEXT_COLOR, TEXT_LEVEL, x, y - 2 * STATE_RADIUS);
      while (list)
      {
	 fprintf (out, "%d", list->value);
	 list = list->next;
	 if (list)
	    fprintf (out, "->");
	 else
	    fprintf (out, "\\001\n");
      }
   }
}

void
DrawLevels (FILE* out, LCTree_t* LCTree)
/*
 *  Draw the level lines for better overview
 *
 *  out    - xfig-file to draw in
 *  LCTree - LCTree to work on
 */
{
   int i;
   int depth = CalcLCTreeDepth (LCTree, LCTree->root_state); /* real depth */
   
   int y = LCTree->states [LCTree->root_state].y;
   int x1, x2;

   int pos1 = LCTree->root_state;
   int pos2 = LCTree->root_state;

   /* get the coordinates of the most left and right states */
   while (ischild (LCTree->states [pos1].child [0]))
      pos1 = LCTree->states [pos1].child [0];
   while (ischild (LCTree->states [pos2].child [MAXLABELS-1]))
      pos2 = LCTree->states [pos2].child [MAXLABELS-1];

   x1 = LCTree->states [pos1].x;
   x2 = LCTree->states [pos2].x;

   /* draw the lines */
   for(i = 0; i <= depth; i++)
   {
      xfig_line (out, x1 - 200, y, x2 + 200, y, LEVEL_COLOR, DASHED, 1,
		 LEVEL_LEVEL);
      y += Y_STATE_DISTANCE;
   }
}

int
DrawLegend (FILE *out, const wfa_t *wfa, toptions_t *options, int frame_nr,
	    int color_image, int y_offset)
/*
 *  Draw the legend to inform about colors, line thickness and so on.
 *
 *  out         - xfig-file to draw in
 *  wfa         - pointer on wfa-tree, needed to get the name of the basis
 *  options     - pointer on options struct, needed to decide what do draw
 *  frame_nr    - number of the frame in the video sequence, also displayed
 *  color_image - is this wfa a color image ?
 *  y_offset    - the last "occupied" y-coordinate
 *
 *  Return value:
 *	the largest x-coordinate used
 */
{
   int x = X_LEGEND_MARGIN;
   int y = y_offset + Y_LEGEND_MARGIN;

   /* common infos */
   fprintf (out, "4 0 %d %d 0 0 7 0.0000 4 50 120 %d %d %s\\001\n",
	    TEXT_COLOR, TEXT_LEVEL, x, y, options->parameter_string);

   y_offset += Y_LEGEND_MARGIN + 250;
   y = y_offset;

   fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d basis name:\\001\n",
	    TEXT_COLOR, LEGEND_LEVEL, x, y);
   fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d %s\\001\n",
	    TEXT_COLOR, LEGEND_LEVEL, x+700, y, wfa->wfainfo->basis_name);
   y += Y_LEGEND_DISTANCE;
   fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d frame number:\\001\n",
	    TEXT_COLOR, LEGEND_LEVEL, x, y);
   fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d %d\\001\n",
	    TEXT_COLOR, LEGEND_LEVEL, x+700, y, frame_nr);
   y += Y_LEGEND_DISTANCE;
   fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d color image:\\001\n",
	    TEXT_COLOR, LEGEND_LEVEL, x, y);
   fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d %s\\001\n",
	    TEXT_COLOR, LEGEND_LEVEL, x + 700, y, color_image ? "YES" : "NO");
   x += 1200;

   /* color description */
   if (options->into_states || options->into_basis)
   {
      x += 600;
      y  = y_offset;
      fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d "
	       "linear combinations:\\001\n", TEXT_COLOR, LEGEND_LEVEL, x, y);
   }

   if (options->into_states)
   {
      y += Y_LEGEND_DISTANCE;
      xfig_line (out, x, y - 30, x + 100, y - 30, LEFT_LC_TREE_COLOR,
		 LEFT_LC_TREE_STYLE, 1, LEGEND_LEVEL);
      fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d "
	       "from subimage 0 to tree\\001\n", TEXT_COLOR, LEGEND_LEVEL,
	       x + 200, y);
      y += Y_LEGEND_DISTANCE;
      xfig_line (out, x, y - 30, x + 100, y - 30, RIGHT_LC_TREE_COLOR,
		 RIGHT_LC_TREE_STYLE, 1, LEGEND_LEVEL);
      fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d "
	       "from subimage 1 to tree\\001\n", TEXT_COLOR, LEGEND_LEVEL,
	       x + 200, y);
      x += 1200;
      y = y_offset;
   }
   if (options->into_basis)
   {
      y += Y_LEGEND_DISTANCE;
      xfig_line (out, x, y - 30, x + 100, y - 30, LEFT_LC_BASIS_COLOR,
		 LEFT_LC_BASIS_STYLE,
		 1, LEGEND_LEVEL);
      fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d "
	       "from subimage 0 to basis\\001\n", TEXT_COLOR, LEGEND_LEVEL,
	       x + 200, y);
      y += Y_LEGEND_DISTANCE;
      xfig_line (out, x, y - 30, x + 100, y - 30, RIGHT_LC_BASIS_COLOR,
		 RIGHT_LC_BASIS_STYLE, 1, LEGEND_LEVEL);
      fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d "
	       "from subimage 1 to basis\\001\n", TEXT_COLOR, LEGEND_LEVEL,
	       x + 200, y);
      x += 1200;
      y = y_offset;
   }

   /* line thickness description */
   if (options->into_states || options->into_basis)
   {
      x += 600;
      fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d "
	       "line thickness:\\001\n", TEXT_COLOR, LEGEND_LEVEL, x, y);
      y += Y_LEGEND_DISTANCE;
      xfig_line (out, x, y - 30, x + 100, y - 30, TEXT_COLOR, SOLID,
		 COUNT_1_THICK, LEGEND_LEVEL);
      fprintf(out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d #lines <= %d\\001\n",
	      TEXT_COLOR, LEGEND_LEVEL, x + 200, y, MAX_COUNT_1);
      y += Y_LEGEND_DISTANCE;
      xfig_line (out, x, y - 30, x + 100, y - 30, TEXT_COLOR, SOLID,
		 COUNT_2_THICK, LEGEND_LEVEL);
      fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d %d "
	       "< #lines <= %d\\001\n",
	      TEXT_COLOR, LEGEND_LEVEL, x + 200, y, MAX_COUNT_1, MAX_COUNT_2);
      x += 1000;
      y = y_offset + Y_LEGEND_DISTANCE;
      xfig_line (out, x, y - 30, x + 100, y - 30, TEXT_COLOR, SOLID,
		 COUNT_3_THICK, LEGEND_LEVEL);
      fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d %d "
	       "< #lines <= %d\\001\n",
	      TEXT_COLOR, LEGEND_LEVEL, x + 200, y, MAX_COUNT_2, MAX_COUNT_3);
      y += Y_LEGEND_DISTANCE;
      xfig_line (out, x, y - 30, x + 100, y - 30, TEXT_COLOR, SOLID,
		 COUNT_4_THICK, LEGEND_LEVEL);
      fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d %d < #lines\\001\n",
	       TEXT_COLOR, LEGEND_LEVEL, x + 200, y, MAX_COUNT_3);
   }

   /* draw a box around all */
   xfig_box (out, X_LEGEND_MARGIN - 100, y_offset - Y_LEGEND_DISTANCE,
	     y - y_offset + 1.5 * Y_LEGEND_DISTANCE, x - X_LEGEND_MARGIN + 1000,
	     LEGEND_FRAME_COLOR, LEGEND_FILL_COLOR, LEGEND_LEVEL + 1);

   /* return needed space */
   return x + 1000;
}

void
DrawGrid (FILE *out, const wfa_t *wfa, LCTree_t *LCTree, int color_image,
	  int *color_field, int legend_offset, toptions_t *options)
/*
 *  Draw a partition grid showing a "top view" of the tree.
 *  decide if image is a color image, then 3 grids are drawn for
 *  Y-, Cb- and Cr-band, else one grid is enough
 *
 *  out           - xfig-file to draw in
 *  wfa           - pointer on wfa-tree, for the partitions
 *  LCTree        - pointer on LCTree, for the area to colorize
 *  color_image   - is this wfa a color image ?
 *  color_field   - field containing the colorvalues for each state
 *  legend_offset - the last x-coordinate "occupied" by the legend
 *  options       - pointer on options struct
 */
{
   int pos;
   int x, y;

   /* get most right state of tree */
   pos = LCTree->root_state;
   while (ischild (LCTree->states[pos].child[MAXLABELS-1]))
      pos = LCTree->states [pos].child[MAXLABELS-1];
   x = X_GRID_MARGIN + LCTree->states[pos].x;

   /* get last "occupied" x-coordinate at all */
   if (x <= X_GRID_MARGIN + legend_offset)
      x = X_GRID_MARGIN + legend_offset;
   
   y = Y_GRID_MARGIN;

   /* draw grid(s) */
   if (color_image)
   {
      int root1 = wfa->tree[wfa->tree[wfa->root_state][0]][0];
      int root2 = wfa->tree[wfa->tree[wfa->root_state][0]][1];
      int root3 = wfa->tree[wfa->tree[wfa->root_state][1]][0];

      fprintf(out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d Y-Band:\\001\n",
	      TEXT_COLOR, GRID_LEVEL, x, y);
      y += Y_LEGEND_DISTANCE;
      DrawGreyGrid (out, wfa, x, y, root1, color_field, options);
      y += height_of_level (wfa->level_of_state[root1]) * Y_GRID_SCALE
	   + Y_GRID_DISTANCE;

      fprintf (out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d Cb-Band:\\001\n",
	       TEXT_COLOR, GRID_LEVEL, x, y);
      y += Y_LEGEND_DISTANCE;
      DrawGreyGrid (out, wfa, x, y, root2, color_field, options);
      y += height_of_level(wfa->level_of_state[root1]) * Y_GRID_SCALE
	   + Y_GRID_DISTANCE;

      fprintf(out, "4 0 %d %d 0 0 6 0.0000 4 50 120 %d %d Cr-Band:\\001\n",
	      TEXT_COLOR, GRID_LEVEL, x, y);
      y += Y_LEGEND_DISTANCE;
      DrawGreyGrid (out, wfa, x, y, root3, color_field, options);
   }
   else
      DrawGreyGrid (out, wfa, x, y, wfa->root_state, color_field, options);
}

void
DrawGreyGrid (FILE *out, const wfa_t *wfa, int x_offset, int y_offset,
	      int local_root, int *color_field, toptions_t *options)
/*
 *  Recursively draw a partition grid showing a "top view" of the tree.
 *  This can be the Y-, Cb- or Cr-band or the greyscale image, depending
 *  on the image type. The image parts are colorized as the corresponding
 *  tree parts, if set in options, else the LCTree-part of the whole image
 *  is filled with a predefined color
 *
 *  out                - xfig-file to draw in
 *  wfa                - pointer on wfa-tree, for the partitions
 *  x_offset, y_offset - the upper left corder of the grid
 *  local_root         - the root of the current recursion step
 *  color_field        - field containing the colorvalues for each state
 *  options            - pointer on options struct
 */
{
   int i;

   for (i = 0; i < MAXLABELS; i++)
      if (ischild (wfa->tree [local_root][i]))
	 /* for child do it recursive */
	 DrawGreyGrid (out, wfa, x_offset, y_offset, wfa->tree[local_root][i],
		       color_field, options);
      else
      {
	 /* draw leaf */
	 int color;
	 if (color_field [local_root] == DEFAULT)
	    color = GRID_FILL_COLOR;
	 else if (options->color_grid)
	    color = GRID_COLOR(color_field [local_root]);
	 else
	    color = GRID_SELECTED_COLOR;
	 
	 xfig_box (out,
		   x_offset + wfa->x [local_root][i] * X_GRID_SCALE,
		   y_offset + wfa->y [local_root][i] * Y_GRID_SCALE,
		   height_of_level (wfa->level_of_state [local_root] - 1)
		   * X_GRID_SCALE,
		   width_of_level (wfa->level_of_state[local_root]-1)
		   * Y_GRID_SCALE,
		   GRID_LINE_COLOR, color, GRID_LEVEL);
      }
}

void
FillColor (int wfa_root, const wfa_t *wfa, int color, int *color_field)
/*
 *  Recursively fill a whole subtree in color_field with color.
 *  this is used for colorization of tree and partition grid.
 *
 *  wfa_root    - local root in current recursion step
 *  wfa         - pointer on wfa-tree
 *  color       - color to fill in
 *  color_field - pointer on color_field containing the colors for the states
 */
{
   int i;

   for (i = 0; i < MAXLABELS; i++)
   {
      color_field [wfa_root] = color;
      if (ischild (wfa->tree [wfa_root][i]))
	 FillColor (wfa->tree [wfa_root][i], wfa, color, color_field);
   }
}

void
SetColor (int wfa_root, int *lrw_to_lwr, const wfa_t *wfa, LCTree_t *LCTree,
	  int color, int depth, int *color_field)
/*
 *  Recursively colorize LCTree by calculating the colors and storing them
 *  in color_field. try to use a different color for each subtree at depth
 *  MAX_COLOR_DEPTh, but if a state has not all childs, the substree of this
 *  state will be filled with one color, not dependend on the current depth.
 *  This is necessary to avoid regions in the partition grid colorized with
 *  colors not shown in the tree.
 *
 *  wfa_root    - local root in current recursion step
 *  lrw_to_lwr  - field to transform state numbers from lrw-order to lwr-order
 *  wfa         - pointer on wfa-tree
 *  LCTree      - pointer on LCTree
 *  color       - color-offset at this depth
 *  depth        - current depth (of the LCTree), 0 means root
 *  color_field - pointer on color_field containing the colors for the states
 */
{
   int i;
   int colors [MAXLABELS];
   int childs = 0;
   
   for(i = 0; i < MAXLABELS; i++)
   {
      colors [i] = color;
      if (ischild (wfa->tree [wfa_root][i]))
      {
	 if (depth < MAX_COLOR_DEPTH
	     && !isrange (LCTree->states [lrw_to_lwr [wfa_root]].state_nr)
 	     && ischild (LCTree->states [lrw_to_lwr [wfa_root]].child[i]))
	 {
	    /* calculate new color-offsets */
	    int offset = pow (MAXLABELS, MAX_COLOR_DEPTH - depth - 1);

	    childs++;
	    colors[i] = color + i * offset;
	 }
	 /*
	  * if no child in LCTree, fill the wfa-subtree with current color
	  * this can happen, if the LCTree-state is a Multistate
	  */
	 if (!ischild (LCTree->states [lrw_to_lwr [wfa_root]].child [i]))
	    FillColor (wfa->tree [wfa_root][i], wfa, colors [0], color_field);
	 else
	    /* else recursively colorize the subtree */
	    SetColor (wfa->tree [wfa_root][i], lrw_to_lwr, wfa, LCTree,
		      colors [i], depth + 1, color_field);
      }
      /* colorize root, but only if depth is reached */
      if (depth >= MAX_COLOR_DEPTH)
	 color_field [wfa_root] = colors [i];
   }

   /* if not a "full" state, fill subtree with current color */
   if (childs < MAXLABELS)
      FillColor (wfa_root, wfa, colors [0], color_field);
 
}

void
GetColorField (const wfa_t* wfa, LCTree_t *LCTree, int *color_field,
	       int *lrw_to_lwr)
/*
 *  Initialize the color-field and start calculation of colors
 *
 *  wfa         - pointer on wfa-tree
 *  LCTree      - pointer on LCTree
 *  color_field - pointer on color_field containing the colors for the states
 *  lrw_to_lwr  - field to transform state numbers from lrw-order to lwr-order
 */
{
   int i;
   for (i = 0; i <= wfa->root_state; i++)
      color_field [i] = DEFAULT;
   
   SetColor (LCTree->states [LCTree->root_state].state_nr, lrw_to_lwr, wfa,
	     LCTree, 0, 0, color_field);
}

int
DrawTree (FILE *outfile, LCTree_t *LCTree, toptions_t *options, int depth,
	  int *color_field)
/*
 *  Draw tree-states, tree-edges, levels and linear combinations to tree
 *
 *  outfile     - xfig-file to draw in
 *  LCTree      - pointer on LCTree to draw
 *  options     - pointer on options-struct
 *  depth        - depth of the tree
 *  color_field - pointer on field containing the colors for the states
 *  return value: max y-coordinate used for tree (incl. l.c.s to tree)
 */
{
   int i, j;
   int TreeDepth = Y_TREE_MARGIN + depth * Y_STATE_DISTANCE;

   /* for all tree-states */
   for (i = LCTree->basis_states; i <= LCTree->nr_of_states; i++)
   {
      if (!isrange (LCTree->states [i].state_nr))
      {
	 if (options->states || options->state_text) 
	 {
	    int color;

	    /* draw states using the defined color */
	    if (options->color_grid)
	    {
	       color = color_field [LCTree->states [i].state_nr];
	       if (color != DEFAULT)
		  color = GRID_COLOR (color);
	    }
	    else
	       color = DEFAULT;
	    DrawState (outfile, &(LCTree->states [i]), options, color);
	 }
	 
	 for(j = 0; j < MAXLABELS; j++) /* draw tree-edges */
	    if (ischild (LCTree->states [i].child [j]))
	       xfig_line (outfile, LCTree->states [i].x, LCTree->states [i].y,
			  LCTree->states [LCTree->states [i].child [j]].x,
			  LCTree->states [LCTree->states [i].child [j]].y,
			  TREE_COLOR, SOLID, 1, EDGE_LEVEL);
      }
   }

   /* draw origin of root-state */
   DrawOrigin (outfile, LCTree->states [LCTree->root_state].x,
	       LCTree->states [LCTree->root_state].y, LCTree->root_origin);

   /* draw levels, if set */
   if (options->with_levels)
      DrawLevels (outfile, LCTree);

   /* draw linear combinations to tree */
   if (options->into_states)
      TreeDepth = DrawTreeLC (outfile, LCTree, depth);

   /* return max y used */
   return TreeDepth;
}

void
DrawBasis (FILE* outfile, LCTree_t *LCTree, toptions_t *options)
/*
 *  Draw basis-states and linear combinations to basis
 *
 *  outfile     - xfig-file to draw in
 *  LCTree      - pointer on LCTree to draw
 *  options     - pointer on options-struct
 */
{
   int i;

   /* draw basis */
   for (i = 0; i < LCTree->basis_states; i++)
      if (!isrange (LCTree->states [i].state_nr))
	 DrawState (outfile, &LCTree->states [i], options, -1);

   /* draw linear combinations to basis */
   if (options->into_basis)
      DrawBasisLC (outfile, LCTree);
}

int
DrawTreeLC (FILE* outfile, LCTree_t *LCTree, int depth)
/*
 *  Draw the linear combinations from tree states to tree states, starting from
 *  the right. First from each state a line is drawn down to a y-coordinate that
 *  is increased with each line down. Then the line is drawn to the left and at
 *  the x-coordinate of each target state the line thickness is reduced to the
 *  #l.c.s left. The y-coordinate and the number of lines is stored in the
 *  tree_in-list of each target state. After this from each state the
 *  tree_in-list is used to draw the lines down from the target to the lines
 *  coming from the right.
 *
 *  outfile - xfig-file to drawn in
 *  LCTree  - pointer on the LCTree
 *  options - pointer on options-struct
 *  depth    - depth of the tree
 *  return value: max y_coordinate used
 */
{
   int i, j;
   int offset [3][MAXLABELS];
   int d = STATE_RADIUS / (3 * MAXLABELS);
   int x1;
   int x2;
   int y1 = Y_TREE_MARGIN + Y_LC_MARGIN + depth * Y_STATE_DISTANCE;
   int y2;
   int LC_depth;
   int count;
   int color;
   tlist_t* pos;

   /*
    * calculate offsets for the tree_out-/tree_in-lines for each
    * subimage at a state
    */
   for (i = 0; i < 2; i++)
      for (j = 0; j < MAXLABELS; j++)
	 offset [i][j] = 2 * d * (i * 2 * MAXLABELS + j)
			 - d * (3 * MAXLABELS - 1);

   /* Draw l.c.-lines to tree-states down and left */
   for (i = LCTree->nr_of_states; i >= LCTree->basis_states; i--)
      if (!isrange (LCTree->states[i].state_nr))
      {
	 for(j = 0; j < MAXLABELS; j++)
	 {
	    pos = LCTree->states [i].tree_out [j];

	    if(pos)
	    {
	       unsigned style;
	       
	       if (j == 1)
	       {
		  color = RIGHT_LC_TREE_COLOR;
		  style	= RIGHT_LC_TREE_STYLE;
	       }
	       else
	       {
		  color = LEFT_LC_TREE_COLOR;
		  style = LEFT_LC_TREE_STYLE;
	       }
	       
	       count = LCTree->states [i].tree_out_nr [j];

	       x1 = LCTree->states [i].x+offset [0][j];
	       x2 = x1 - LC_RADIUS;

	       y2 = LCTree->states [i].y;
	       if (!isrange (LCTree->states [i].multistate))
		  y2 += MULTISTATE_HEIGHT;
	    
	       xfig_line (outfile, x1, y2, x1, y1, color, style,
			  GetThick (count), LC_TREE_LEVEL);
	       xfig_line (outfile, x1, y1, x2, y1 + LC_RADIUS, color, style,
			  GetThick (count), LC_TREE_LEVEL);

	       while (pos)
	       {
		  InsertAscList (&(LCTree->states [pos->value].tree_in [j]),
				 y1, pos->count);
		  LCTree->states [pos->value].tree_in_nr [j] += pos->count;
		  x1 = x2;
		  x2 = LCTree->states [pos->value].x + offset [1][MAXLABELS-1-j]
		       + LC_RADIUS;
		  xfig_line (outfile, x1, y1 + LC_RADIUS, x2, y1 + LC_RADIUS,
			     color, style, GetThick (count), LC_TREE_LEVEL);
		  xfig_line (outfile, x2, y1 + LC_RADIUS, x2 - LC_RADIUS, y1,
			     color, style, GetThick (pos->count), LC_TREE_LEVEL);
		  count -= pos->count;
		  pos = pos->next;
	       }
	       y1 += LC_RADIUS;
	    }
	 }
	 y1 += Y_LC_DISTANCE;
      }

   LC_depth = y1;
   
   /* Draw incoming l.c.-lines to tree-states down */
   for (i = LCTree->basis_states; i <= LCTree->nr_of_states; i++)
      if (!isrange (LCTree->states [i].state_nr))
	 for(j = 0; j < MAXLABELS; j++)
	 {
	    pos = LCTree->states [i].tree_in [j];

	    if (pos)
	    {
	       unsigned style;
	       
	       if (j == 1)
	       {
		  color = RIGHT_LC_TREE_COLOR;
		  style = RIGHT_LC_TREE_STYLE;
	       }
	       else
	       {
		  color = LEFT_LC_TREE_COLOR;
		  style = LEFT_LC_TREE_STYLE;
	       }

	       count = LCTree->states [i].tree_in_nr [j];

	       x1 = LCTree->states [i].x+offset [1][MAXLABELS-1-j];
	       y1 = LCTree->states [i].y;       
	       if (!isrange (LCTree->states [i].multistate))
		  y1 += MULTISTATE_HEIGHT;

	       while (pos)
	       {
		  xfig_line (outfile, x1, y1, x1, pos->value, color, style,
			     GetThick (count), LC_TREE_LEVEL);
		  y1     = pos->value;
		  count -= pos->count;
		  pos    = pos->next;
	       }
	    }
	 }
   
   return LC_depth;
}

void
DrawBasisLC (FILE* outfile, LCTree_t *LCTree)
/*
 *  Draw the linear combinations from tree states to basis states, starting
 *  from the right. First from each state a line is drawn down to the
 *  y-coordinates of the basis states shrinking in thickness by the number
 *  of lines going out to each basis state. The x-coordinate and the number
 *  of lines is stored in the tree_in-list of each basis state. After this from
 *  each basis state the tree_in-list is used to draw the lines right to the
 *  lines coming down from the tree states shrinking in thickness by the number
 *  of lines going up to the tree.
 *
 *  outfile - xfig-file to drawn in
 *  LCTree  - pointer on the LCTree
 *  options - pointer on options-struct
 */
{
   int i, j;
   int x_offset [MAXLABELS];
   int y_offset [MAXLABELS];

   int d = STATE_RADIUS / (3 * MAXLABELS);

   int x1, x2;
   int y1, y2;

   int count;
   
   int color;
   tlist_t* pos;

   /*
    * calulate x_offsets at the tree_states for the l.c.s out for each subimage
    * and the y_offsets for the lc.s in at the basis states for each subimage
    */
   for (i = 0; i < MAXLABELS; i++)
   {
      x_offset [i] =  2 * d * (MAXLABELS + i) - d * (3 * MAXLABELS - 1);
      y_offset [i] = (2 * i + 1 - MAXLABELS) * STATE_RADIUS / MAXLABELS;
   }
   
   /* Draw l.c.-lines to basis-states down */
   for (i = LCTree->nr_of_states; i >= LCTree->basis_states; i--)
      if (!isrange (LCTree->states [i].state_nr))
	 for(j = 0; j < MAXLABELS; j++)
	 {
	    pos = LCTree->states [i].basis_out [j];

	    if (pos)
	    {
	       unsigned style;
	       
	       if (j == 1)
	       {
		  color = RIGHT_LC_BASIS_COLOR;
		  style = RIGHT_LC_BASIS_STYLE;
	       }
	       else
	       {
		  color = LEFT_LC_BASIS_COLOR;
		  style = LEFT_LC_BASIS_STYLE;
	       }
	       
	       count = LCTree->states [i].basis_out_nr [j];
	       
	       x1 = LCTree->states [i].x + x_offset [j];
	       x2 = x1 - LC_RADIUS;
	       
	       y1 = LCTree->states [i].y;
	       if (!isrange (LCTree->states [i].multistate))
		  y1 += MULTISTATE_HEIGHT;

	       while (pos)
	       {
		  InsertAscList (&(LCTree->states [pos->value].tree_in [j]),
				 x2, pos->count);
		  LCTree->states [pos->value].tree_in_nr [j] += pos->count;

		  y2 = LCTree->states[pos->value].y + y_offset[j] - LC_RADIUS;

		  xfig_line (outfile, x1, y1, x1, y2, color, style,
			     GetThick (count), LC_BASIS_LEVEL);
		  xfig_line (outfile, x1, y2, x2, y2 + LC_RADIUS, color, style,
			     GetThick (pos->count), LC_BASIS_LEVEL);
		  y1     = y2;
		  count -= pos->count;
		  pos    = pos->next;
	       }
	    }
	 }

   /* Draw l.c-lines from basis-states to right */
   for (i = 0; i < LCTree->basis_states; i++)
      if (!isrange (LCTree->states [i].state_nr))
	 for(j = 0; j < MAXLABELS; j++)
	 {
	    pos = LCTree->states [i].tree_in [j];

	    if (pos)
	    {
	       unsigned style;
	       if (j == 1)
	       {
		  color = RIGHT_LC_BASIS_COLOR;
		  style = RIGHT_LC_BASIS_STYLE;
	       }
	       else
	       {
		  color = LEFT_LC_BASIS_COLOR;
		  style = LEFT_LC_BASIS_STYLE;
	       }
	       count = LCTree->states [i].tree_in_nr [j];

	       x1 = LCTree->states [i].x;
	       y1 = LCTree->states [i].y + y_offset [j];
	       
	       while (pos)
	       {
		  x2 = pos->value;
		  xfig_line (outfile, x1, y1, x2, y1, color, style,
			     GetThick (count), LC_BASIS_LEVEL+1);
		  x1     = x2;
		  count -= pos->count;
		  pos    = pos->next;
	       }
	    }
	 }
}


