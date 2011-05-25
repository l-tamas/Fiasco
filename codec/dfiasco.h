/*
 *  dfiasco.h
 *		
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/15 18:00:53 $
 *  $Author: hafner $
 *  $Revision: 5.2 $
 *  $State: Exp $
 */

#ifndef _DFIASCO_H
#define _DFIASCO_H

#include "types.h"
#include "bit-io.h"
#include "decoder.h"
#include "image.h"
#include "wfa.h"

typedef struct dfiasco
{
   char       id [8];
   wfa_t     *wfa;
   video_t   *video;
   bitfile_t *input;
   int	      enlarge_factor;
   int        smoothing;
   format_e   image_format;
} dfiasco_t;

#endif /* not _DFIASCO_H */

