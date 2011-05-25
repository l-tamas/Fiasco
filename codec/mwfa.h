/*
 *  mwfa.h
 *
 *  Written by:		Michael Unger
 *			Ullrich Hafner
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

#ifndef _MWFA_H
#define _MWFA_H

#include "types.h"
#include "image.h"
#include "wfa.h"
#include "cwfa.h"

void
fill_norms_table (unsigned x0, unsigned y0, unsigned level,
		  const wfa_info_t *wi, motion_t *mt);
void
find_B_frame_mc (word_t *mcpe, real_t price, range_t *range,
		 const wfa_info_t *wi, const motion_t *mt);
void
find_P_frame_mc (word_t *mcpe, real_t price, range_t *range,
		 const wfa_info_t *wi, const motion_t *mt);
void
subtract_mc (image_t *image, const image_t *past, const image_t *future,
	     const wfa_t *wfa);
void
free_motion (motion_t *mt);
motion_t *
alloc_motion (const wfa_info_t *wi);

#endif /* not _MWFA_H */

