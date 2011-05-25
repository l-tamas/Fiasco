/*
 *  coeff.h
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

#ifndef _COEFF_H
#define _COEFF_H

#include "types.h"
#include "rpf.h"
#include "wfa.h"

typedef struct coeff
{
   rpf_t    *rpf;			/* reduced precision format */
   rpf_t    *dc_rpf;			/* RPF of DC (state 0) component */
   unsigned min_level, max_level;	/* allocate memory for [min,..,max] */
   void	    *model;			/* generic pointer to prob. model */
   real_t (*bits) (const real_t *used_coeff, const word_t *used_domains,
		   unsigned level, const struct coeff *coeff);
   /*
    *  Compute bit-rate of a range approximation with coefficients given by
    *  -1 terminated list 'used_domains'.
    */
   void   (*update) (const real_t *used_coeff, const word_t *used_domains,
		       unsigned level, struct coeff *coeff);
   /*
    *  Update the probability model according to the chosen approximation.
    *  (given by the -1 terminated list 'used_domains').
    */
   void	  (*free) (struct coeff *coeff);
   /*
    *  Discard the given coefficients struct.
    */
   void   (*model_free) (void *model);
   /*
    *  Free given probability model.
    */
   void   *(*model_duplicate) (const struct coeff *coeff, const void *model);
   /*
    *  Duplicate the given probability model (i.e. alloc and copy).
    */
} coeff_t;

coeff_t *
alloc_coeff_model (const char *coeff_model_name, rpf_t *rpf, rpf_t *dc_rpf,
		   unsigned min_level, unsigned max_level);

#endif /* not _COEFF_H */

