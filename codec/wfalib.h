/*
 *  wfalib.h
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

#ifndef _WFALIB_H
#define _WFALIB_H

#include "types.h"
#include "wfa.h"
#include "list.h"

typedef struct range_sort
{
   u_word_t *range_state;
   byte_t   *range_label;
   u_word_t *range_max_domain;
   bool_t   *range_subdivided;
   unsigned  range_no;
} range_sort_t;

bool_t
locate_delta_images (wfa_t *wfa);
void
sort_ranges (unsigned state, unsigned *domain,
	     range_sort_t *rs, const wfa_t *wfa);
bool_t
find_range (unsigned x, unsigned y, unsigned band,
	    const wfa_t *wfa, unsigned *range_state, unsigned *range_label);
void
compute_spiral (int *vorder, unsigned image_width, unsigned image_height,
		unsigned tiling_exp, bool_t inc_spiral);
void
locate_subimage (unsigned orig_level, unsigned level, unsigned bintree,
		 unsigned *x, unsigned *y, unsigned *width, unsigned *height);
void
copy_wfa (wfa_t *dst, const wfa_t *src);
void 
remove_states (unsigned from, wfa_t *wfa);
void
append_edge (unsigned from, unsigned into, real_t weight,
	     unsigned label, wfa_t *wfa);
word_t *
compute_hits (unsigned from, unsigned to, unsigned n, const wfa_t *wfa);
real_t 
compute_final_distribution (unsigned state, const wfa_t *wfa);
wfa_t *
alloc_wfa (bool_t coding);
void
free_wfa (wfa_t *wfa);
bool_t
locate_delta_images (wfa_t *wfa);

#endif /* not _WFALIB_H */

