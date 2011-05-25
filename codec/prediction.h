/*
 *  prediction.h
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

#ifndef _PREDICTION_H
#define _PREDICTION_H

#include "types.h"
#include "cwfa.h"

real_t
predict_range (real_t max_costs, real_t price, range_t *range, wfa_t *wfa,
	       coding_t *c, unsigned band, int y_state, unsigned states,
	       const tree_t *tree_model, const tree_t *p_tree_model,
	       const void *domain_model, const void *d_domain_model,
	       const void *coeff_model, const void *d_coeff_model);
void
update_norms_table (unsigned level, const wfa_info_t *wi, motion_t *mt);
void
clear_norms_table (unsigned level, const wfa_info_t *wi, motion_t *mt);

#endif /* not _PREDICTION_H */

