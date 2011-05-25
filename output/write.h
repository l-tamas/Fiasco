/*
 *  write.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/15 17:27:30 $
 *  $Author: hafner $
 *  $Revision: 5.2 $
 *  $State: Exp $
 */

#ifndef _WRITE_H
#define _WRITE_H

#include "cwfa.h"
#include "bit-io.h"

void
write_next_wfa (const wfa_t *wfa, const coding_t *c, bitfile_t *output);
void
write_header (const wfa_info_t *wi, bitfile_t *output);

#endif /* not _WRITE_H */
