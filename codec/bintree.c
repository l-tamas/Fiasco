/*
 *  bintree.c:		Bintree model of WFA tree	
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:50:51 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#include "config.h"

#include "types.h"
#include "macros.h"
#include "error.h"

#include <math.h>

#include "bintree.h"
#include "misc.h"
#include "cwfa.h"

/*****************************************************************************

				public code
  
*****************************************************************************/

void
tree_update (bool_t child, unsigned level, tree_t *model)
/*
 *  Update tree model of given 'level'.
 *
 *  No return value.
 *
 *  Side effects:
 *	tree model is changed.
 */
{
   if (!child)
      model->total [level]++;
   else
   {
      model->counts [level]++;
      model->total [level]++;
   }
}

real_t
tree_bits (bool_t child, unsigned level, const tree_t *model)
/*
 *  Compute number of bits needed for coding element 'child' of the bintree.
 *  For each 'level' a different context is used.
 *
 *  Return value:
 *	# bits
 */
{
   real_t prob = model->counts [level] / (real_t) model->total [level];

   return child ? - log2 (prob) : - log2 (1 - prob);
}

void
init_tree_model (tree_t *tree_model)
/*
 *  Initialize the model for the tree.
 *  Symbol RANGE is synonymous with the '0' symbol and
 *  symbol NO_RANGE is synonymous with the '1' symbol of the binary coder.
 *  The 'count' array counts the number of NO_RANGE symbols.
 *
 *  No return value.
 */
{
   unsigned level;
   unsigned counts_0 [MAXLEVEL] = {20, 17, 15, 10, 5,  4,  3,
				   2,  1,  1,  1,  1,  1,  1,  1,
				   1,  1,  1,  1 , 1,  1,  1};
   unsigned counts_1 [MAXLEVEL] = {1 , 1,  1,  1,  1,  1,  1,
				   1,  1,  2,  3,  5,  10, 15, 20,
				   25, 30, 35, 60, 60, 60, 60};
   
   for (level = 0; level < MAXLEVEL ; level++) 
   {
      tree_model->counts [level] = counts_1 [level];
      tree_model->total [level]  = counts_0 [level] + counts_1 [level];
   }
}
