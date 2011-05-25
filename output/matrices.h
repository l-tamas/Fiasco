/*
 *  matrices.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:50:31 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#ifndef _MATRICES_H
#define _MATRICES_H

#include "wfa.h"
#include "bit-io.h"

unsigned
write_matrices (bool_t use_normal_domains, bool_t use_delta_domains,
		const wfa_t *wfa, bitfile_t *output);

#endif /* _MATRICES_H */

