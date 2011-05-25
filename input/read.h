/*
 *  read.h
 *
 *  Written by:		Ullrich Hafner
 *  
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:50:13 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#ifndef _READ_H
#define _READ_H

#include "wfa.h"
#include "bit-io.h"

bitfile_t *
open_wfa (const char *filename, wfa_info_t *wfainfo);
void
read_basis (const char *filename, wfa_t *wfa);
unsigned
read_next_wfa (wfa_t *wfa, bitfile_t *input);

#endif /* not _READ_H */

