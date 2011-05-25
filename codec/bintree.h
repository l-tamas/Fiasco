/*
 *  bintree.h
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

#ifndef _BINTREE_H
#define _BINTREE_H

#include "wfa.h"
#include "types.h"

typedef struct tree
/*
 *  Used for estimating the number of bits needed for storing the
 *  tree array. For each level a different context is used.
 *  The binary alphabet consists of the two symbols NO_RANGE and RANGE,
 *  which indicate whether there exists a tree edge or not.
 */
{
   unsigned counts [MAXLEVEL];		/* # NO_RANGE symbols at given level */
   unsigned total [MAXLEVEL];		/* total number of symbols at  ''   */
} tree_t;

real_t
tree_bits (bool_t child, unsigned level, const tree_t *model);
void
init_tree_model (tree_t *tree_model);
void
tree_update (bool_t child, unsigned level, tree_t *model);

#endif /* not _BINTREE_H */

