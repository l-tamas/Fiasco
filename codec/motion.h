/*
 *  motion.h
 *
 *  Written by:		Ullrich Hafner
 *			Michael Unger
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

#ifndef _MOTION_H
#define _MOTION_H

#include "wfa.h"
#include "types.h"
#include "image.h"

void
restore_mc (int enlarge_factor, image_t *image, const image_t *past,
	    const image_t *future, const wfa_t *wfa);
void
extract_mc_block (word_t *mcblock, unsigned width, unsigned height,
		  const word_t *reference, unsigned ref_width,
		  bool_t half_pixel, unsigned xo, unsigned yo,
		  unsigned mx, unsigned my);

#endif /* not _MOTION_H */

