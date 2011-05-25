/*
 *  tiling.h
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

#ifndef _TILING_H
#define _TILING_H

#include "image.h"
#include "fiasco.h"

typedef struct tiling
{
   unsigned    	    exponent;		/* Image is split in 2^exp tiles */
   fiasco_tiling_e  method;		/* Method of Image tiling */
   int	      	   *vorder;		/* Block permutation (size = 2^exp)
					   -1 indicates empty block */
} tiling_t;

void
perform_tiling (const image_t *image, tiling_t *tiling);
tiling_t *
alloc_tiling (fiasco_tiling_e method, unsigned tiling_exponent,
	      unsigned image_level);
void
free_tiling (tiling_t *tiling);

#endif /* not _TILING_H */

