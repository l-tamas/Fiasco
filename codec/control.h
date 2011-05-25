/*
 *  control.h
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

#ifndef _CONTROL_H
#define _CONTROL_H

#include "cwfa.h"
#include "types.h"

void 
append_transitions (unsigned state, unsigned label, const real_t *weight,
		    const word_t *into, wfa_t *wfa);
void 
append_basis_states (unsigned basis_states, wfa_t *wfa, coding_t *c);
void    
append_state (bool_t auxiliary_state, real_t final, unsigned level_of_state,
	      wfa_t *wfa, coding_t *c);

#endif /* not _CONTROL_H */

