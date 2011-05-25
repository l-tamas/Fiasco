/*
 *  ip.h
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

#ifndef _IP_H
#define _IP_H

#include "cwfa.h"

void 
compute_ip_states_state (unsigned from, unsigned to,
			 const wfa_t *wfa, coding_t *c);
real_t 
get_ip_state_state (unsigned domain1, unsigned domain2, unsigned level,
		    const coding_t *c);
void 
compute_ip_images_state (unsigned image, unsigned address, unsigned level,
			 unsigned n, unsigned from,
			 const wfa_t *wfa, coding_t *c);
real_t 
get_ip_image_state (unsigned image, unsigned address, unsigned level,
		    unsigned domain, const coding_t *c);

#endif /* not _IP_H */

