/*      
 *  twfa.h
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

#ifndef _TWFA_H
#define _TWFA_H

#include "tlist.h"
#include "wfa.h"

typedef struct toptions
{
   char		*parameter_string;	/* the string that was entered at
					   the command line */
   int		root_state;		/* the (local) root state */
   bool_t	grid;			/* draw WFA partitioning */
   bool_t	color_grid;		/* draw color WFA partitioning */
   int		max_depth;
   bool_t	cut_first;
   bool_t	states;
   bool_t	state_text;
   bool_t	basis;
   bool_t	into_basis;
   bool_t	into_states;
   bool_t	with_shadows;
   bool_t	with_key;
   bool_t	with_levels;
   char		*wfa_name;		/* MWFA filename */
   char		*output_name;		/* FIG output filename */
   tlist_t	*LC_tree_list;		/* list of linear combinations to tree
					   that have to be drawn,
					   list->count isn't used */
   tlist_t	*LC_basis_list;		/* list of linear combinations to basis
					   that have to be drawn,
					   list->count isn't used */
   tlist_t	*frames_list;		/* list of frames to be calculated,
					   count starts at 0,
					   list->count isn't used */
   int		nr_of_frames;		/* number of entries in frames_list */
} toptions_t;

#endif /* _TWFA_H */

