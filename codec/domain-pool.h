/*
 *  domain-pool.h
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

#ifndef _DOMAIN_POOL_H
#define _DOMAIN_POOL_H

#include "cwfa.h"
#include "types.h"

typedef struct domain_pool
{
   void	  *model;			/* probability model */
   word_t *(*generate) (unsigned level, int y_state, const wfa_t *wfa,
			const void  *model);
   /*
    *  Generate set of domain images which may be used for an approximation.
    *  Use parameters 'level', 'y_state' and 'wfa' to make the decision.
    */
   real_t (*bits) (const word_t *domains, const word_t *used_domains,
		   unsigned level, int y_state, const wfa_t *wfa,
		   const void *model);
   /*
    *  Compute bit-rate of a range approximation with domains given by
    *  the -1 terminated list 'used_domains'.
    */
   void	  (*update) (const word_t *domains, const word_t *used_domains,
		     unsigned level, int y_state, const wfa_t *wfa,
		     void *model);
   /*
    *  Update the probability model according to the chosen approximation.
    *  (given by the -1 terminated list 'used_domains').
    */
   bool_t (*append) (unsigned state, unsigned level, const wfa_t *wfa,
		     void *model);
   /*
    *  Try to append a new state to the domain pool.
    */
   void	  (*chroma) (unsigned max_domains, const wfa_t *wfa, void *model);
   /*
    *  Derive a new domain pool that will be used for chroma channel
    *  coding 
    */
   void   (*free) (struct domain_pool *pool);
   /*
    *  Discard the given domain pool struct.
    */
   void   (*model_free)	(void *model);
   /*
    *  Free given probability model.
    */
   void   *(*model_duplicate) (const void *src);
   /*
    *  Duplicate the given probability model (i.e. alloc and copy).
    */
} domain_pool_t;

domain_pool_t *
alloc_domain_pool (const char *domain_pool_name, unsigned max_domains,
		   unsigned max_edges, const wfa_t *wfa);

#endif /* not _DOMAIN_POOL_H */

