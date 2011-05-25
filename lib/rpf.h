/*
 *  rpf.h
 *
 *  Written by:		Stefan Frank
 *			Richard Krampfl
 *			Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:49:37 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#ifndef _RPF_H
#define _RPF_H

#include "types.h"
#include "fiasco.h"

typedef struct rpf
{
   unsigned    	      mantissa_bits;	/* number of bits used for mantissa */
   real_t      	      range;		/* scale value to [-range, +range] */
   fiasco_rpf_range_e range_e;
} rpf_t;

int
rtob (real_t real, const rpf_t *rpf);
real_t 
btor (int b, const rpf_t *rpf);
rpf_t *
alloc_rpf (unsigned mantissa, fiasco_rpf_range_e range);

extern const int RPF_ZERO;

#endif /* not _RPF_H */






