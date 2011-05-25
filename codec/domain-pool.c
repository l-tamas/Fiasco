/*
 *  domain-pool.c:	Domain pool management (probability model)
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

#include "config.h"

#include <math.h>

#if STDC_HEADERS
#	include <stdlib.h>
#endif /* not STDC_HEADERS */

#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */

#include "types.h"
#include "macros.h"
#include "error.h"

#include "misc.h"
#include "cwfa.h"
#include "wfalib.h"
#include "domain-pool.h"

/*
 *  Domain pool model interface:
 *  Implementing the domain pool model interface requires the
 *  following steps: 
 *  - Add a constructor that initializes the domain_pool_t structure
 *  - Allocate new model with default_alloc() 
 *  - Fill the dp_array_t domain_pools array with constructor and name
 *  - Write code for methods bits() and generate()
 *  - Either use default functions for remaining methods or override them
 *  The new model is automatically registered at the command line.
 */

/*****************************************************************************
									      
			      local variables
			      
*****************************************************************************/

static real_t *matrix_0 = NULL;
static real_t *matrix_1 = NULL;

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static domain_pool_t *
alloc_empty_domain_pool (unsigned max_domains, unsigned max_edges,
			 const wfa_t *wfa);

/*****************************************************************************
			  non-adaptive domain pool
*****************************************************************************/

static void
qac_chroma (unsigned max_domains, const wfa_t *wfa, void *model);
static bool_t
qac_append (unsigned new_state, unsigned level, const wfa_t *wfa, void *model);
static void
qac_update (const word_t *domains, const word_t *used_domains,
	    unsigned level, int y_state, const wfa_t *wfa, void *model);
static real_t
qac_bits (const word_t *domains, const word_t *used_domains,
	  unsigned level, int y_state, const wfa_t *wfa, const void *model);
static word_t *
qac_generate (unsigned level, int y_state, const wfa_t *wfa,
	      const void *model);
static void *
qac_model_duplicate (const void *src);
static void
qac_model_free (void *model);
static void *
qac_model_alloc (unsigned max_domains);
static domain_pool_t *
alloc_qac_domain_pool (unsigned max_domains, unsigned max_edges,
		       const wfa_t *wfa);

/*****************************************************************************
			  run length encoding pool
*****************************************************************************/

static domain_pool_t *
alloc_rle_no_chroma_domain_pool (unsigned max_domains, unsigned max_edges,
				 const wfa_t *wfa);
static void
rle_chroma (unsigned max_domains, const wfa_t *wfa, void *model);
static bool_t
rle_append (unsigned new_state, unsigned level, const wfa_t *wfa, void *model);
static void
rle_update (const word_t *domains, const word_t *used_domains,
	    unsigned level, int y_state, const wfa_t *wfa, void *model);
static real_t
rle_bits (const word_t *domains, const word_t *used_domains,
	  unsigned level, int y_state, const wfa_t *wfa, const void *model);
static word_t *
rle_generate (unsigned level, int y_state, const wfa_t *wfa,
	      const void *model);
static void *
rle_model_duplicate (const void *src);
static void
rle_model_free (void *model);
static void *
rle_model_alloc (unsigned max_domains);
static domain_pool_t *
alloc_rle_domain_pool (unsigned max_domains, unsigned max_edges,
		       const wfa_t *wfa);

/*****************************************************************************
			  const domain pool
*****************************************************************************/

static real_t
const_bits (const word_t *domains, const word_t *used_domains,
	    unsigned level, int y_state, const wfa_t *wfa, const void *model);
static word_t *
const_generate (unsigned level, int y_state, const wfa_t *wfa,
		const void *model);
static domain_pool_t *
alloc_const_domain_pool (unsigned max_domains, unsigned max_edges,
			 const wfa_t *wfa);

/*****************************************************************************
			  basis domain pool
*****************************************************************************/

static domain_pool_t *
alloc_basis_domain_pool (unsigned max_domains, unsigned max_edges,
			 const wfa_t *wfa);

/*****************************************************************************
			  uniform distribution pool
*****************************************************************************/

static real_t
uniform_bits (const word_t *domains, const word_t *used_domains,
	      unsigned level, int y_state, const wfa_t *wfa,
	      const void *model);
static word_t *
uniform_generate (unsigned level, int y_state, const wfa_t *wfa,
		  const void *model);
static domain_pool_t *
alloc_uniform_domain_pool (unsigned max_domains, unsigned max_edges,
			   const wfa_t *wfa);

/*****************************************************************************
			  default functions
*****************************************************************************/

static void
init_matrix_probabilities (void);
static void
default_chroma (unsigned max_domains, const wfa_t *wfa, void *model);
static bool_t
default_append (unsigned new_state, unsigned level,
		const wfa_t *wfa, void *model);
static void
default_update (const word_t *domains, const word_t *used_domains,
		unsigned level, int y_state, const wfa_t *wfa, void *model);
static void
default_free (domain_pool_t *pool);
static void
default_model_free (void *model);
static void *
default_model_alloc (unsigned max_domains);
static void *
default_model_duplicate (const void *src);
static domain_pool_t *
default_alloc (void);

/*****************************************************************************

				public code
  
*****************************************************************************/

typedef struct dp_array
{
   char		 *identifier;
   domain_pool_t *(*function) (unsigned max_domains, unsigned max_edges,
			       const wfa_t *wfa);
} dp_array_t;

dp_array_t domain_pools[] = {{"adaptive",	alloc_qac_domain_pool},
 			     {"constant",	alloc_const_domain_pool}, 
			     {"basis",		alloc_basis_domain_pool},
			     {"uniform",	alloc_uniform_domain_pool},
			     {"rle",		alloc_rle_domain_pool},
			     {"rle-no-chroma",  alloc_rle_no_chroma_domain_pool},
			     {NULL,		NULL}};

domain_pool_t *
alloc_domain_pool (const char *domain_pool_name, unsigned max_domains,
		   unsigned max_edges, const wfa_t *wfa)
/*
 *  Allocate a new domain pool identified by the string
 *  'domain_pool_name'.  Maximum number of domain images (each one
 *  represented by one state of the given 'wfa') is specified by
 *  'max_domains'. 
 * 
 *  Return value:
 *	pointer to the allocated domain pool
 *
 *  Note:
 *      refer to 'domain-pool.h' for a short description of the member functions.
 */
{
   unsigned n;
   
   if (!max_domains)
   {
      warning ("Can't generate empty domain pool. "
	       "Using at least DC component.");
      max_domains = 1;
   }
   
   for (n = 0; domain_pools [n].identifier; n++) /* step through all id's */
      if (strcaseeq (domain_pools [n].identifier, domain_pool_name)) 
	 return domain_pools [n].function (max_domains, max_edges, wfa);

   warning ("Can't initialize domain pool '%s'. Using default value '%s'.",
	    domain_pool_name, domain_pools [0].identifier);

   return domain_pools [0].function (max_domains, max_edges, wfa);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static domain_pool_t *
alloc_empty_domain_pool (unsigned max_domains, unsigned max_edges,
			 const wfa_t *wfa)
/*
 *  Domain pool with no state images available.
 */
{
   return default_alloc ();
}


/*****************************************************************************
			  adaptive domain pool
*****************************************************************************/

typedef struct qac_model
{
   word_t   *index;			/* probability of domains */
   word_t   *states;			/* mapping states -> domains */
   u_word_t  y_index;			/* pointer to prob of Y domain */
   u_word_t  n;				/* number of domains in the pool */
   u_word_t  max_domains;		/* max. number of domains */
} qac_model_t;

static domain_pool_t *
alloc_qac_domain_pool (unsigned max_domains, unsigned max_edges,
		       const wfa_t *wfa)
/*
 *  Domain pool with state images {0, ..., 'max_domains').
 *  Underlying probability model: quasi arithmetic coding of columns.
 */
{
   domain_pool_t *pool;
   unsigned	  state;
   
   pool                  = default_alloc ();
   pool->model           = qac_model_alloc (max_domains);
   pool->generate        = qac_generate;
   pool->bits            = qac_bits;
   pool->update          = qac_update;
   pool->append          = qac_append;
   pool->chroma		 = qac_chroma;
   pool->model_free      = qac_model_free;
   pool->model_duplicate = qac_model_duplicate;
   
   for (state = 0; state < wfa->basis_states; state++)
      if (usedomain (state, wfa))
	  qac_append (state, -1, wfa, pool->model);

   return pool;
}

static void *
qac_model_alloc (unsigned max_domains)
{
   qac_model_t *model;

   init_matrix_probabilities ();

   model 	      = Calloc (1, sizeof (qac_model_t));
   model->index       = Calloc (max_domains, sizeof (word_t));
   model->states      = Calloc (max_domains, sizeof (word_t));
   model->y_index     = 0;
   model->n	      = 0;
   model->max_domains = max_domains;

   return model;
}

static void
qac_model_free (void *model)
{
   Free (((qac_model_t *) model)->index);
   Free (((qac_model_t *) model)->states);
   Free (model);
}

static void *
qac_model_duplicate (const void *src)
{
   qac_model_t	     *qdst;
   const qac_model_t *qsrc = (qac_model_t *) src;

   qdst 	 = qac_model_alloc (qsrc->max_domains);
   qdst->y_index = qsrc->y_index;
   qdst->n       = qsrc->n;
   
   memcpy (qdst->index, qsrc->index, qsrc->n * sizeof (word_t));
   memcpy (qdst->states, qsrc->states, qsrc->n * sizeof (word_t));

   return qdst;
}

static word_t *
qac_generate (unsigned level, int y_state, const wfa_t *wfa, const void *model)
{
   word_t      *domains;
   unsigned	n;
   qac_model_t *qac_model 	  = (qac_model_t *) model;
   bool_t	y_state_is_domain = NO;

   if (y_state >= 0 && !usedomain (y_state, wfa)) /* don't use y-state */
      y_state = -1;
   
   domains = Calloc (qac_model->n + 2, sizeof (word_t));

   memcpy (domains, qac_model->states, qac_model->n * sizeof (word_t));

   for (n = 0; n < qac_model->n; n++)
      if (domains [n] == y_state)	/* match */
	 y_state_is_domain = YES;		

   if (y_state_is_domain)
      domains [qac_model->n] = -1;	/* end marker */
   else
   {
      domains [qac_model->n] 	 = y_state; /* additional y-state */
      domains [qac_model->n + 1] = -1;	/* end marker */
   }

   return domains;
}

static real_t
qac_bits (const word_t *domains, const word_t *used_domains,
	  unsigned level, int y_state, const wfa_t *wfa, const void *model)
{
   int		domain;			/* counter */
   real_t	bits 	  = 0;		/* bit rate R */
   qac_model_t *qac_model = (qac_model_t *) model; /* probability model */

   if (y_state >= 0 && !usedomain (y_state, wfa)) /* don't use y-state */
      y_state = -1;

   for (domain = 0; domain < qac_model->n; domain++)
      if (qac_model->states [domain] != y_state)
	 bits += matrix_0 [qac_model->index [domain]];
   if (y_state >= 0)
      bits += matrix_0 [qac_model->y_index];
   
   if (used_domains != NULL)
   {
      unsigned edge;
      
      for (edge = 0; isedge (domain = used_domains [edge]); edge++)
	 if (domains [domain] == y_state)
	 {
	    bits -= matrix_0 [qac_model->y_index];
	    bits += matrix_1 [qac_model->y_index];
	 }
	 else
	 {
	    bits -= matrix_0 [qac_model->index [domain]];
	    bits += matrix_1 [qac_model->index [domain]];
	 }
   } 
   
   return bits;
}

static void
qac_update (const word_t *domains, const word_t *used_domains,
	    unsigned level, int y_state, const wfa_t *wfa, void *model)
{
   int		domain;
   unsigned	edge;
   bool_t	used_y_state 	  = NO;
   qac_model_t *qac_model 	  = (qac_model_t *) model;
   bool_t	y_state_is_domain = NO;
   
   if (y_state >= 0 && !usedomain (y_state, wfa)) /* don't use y-state */
      y_state = -1;

   for (domain = 0; domain < qac_model->n; domain++)
   {
      qac_model->index [domain]++;	/* mark domains unused. */
      if (qac_model->states [domain] == y_state) /* match */
	 y_state_is_domain = YES;		
   }
   
   for (edge = 0; isedge (domain = used_domains [edge]); edge++)
      if (domains [domain] == y_state) /* chroma coding */
      {
	 if (y_state_is_domain)
	    qac_model->index [domain]--; /* undo */
	 qac_model->y_index >>= 1;
	 used_y_state = YES;
      }
      else				/* luminance coding */
      {
	 qac_model->index [used_domains [edge]]--; /* undo */
	 qac_model->index [used_domains [edge]] >>= 1;		
      }

   if (y_state >= 0 && !used_y_state)
      qac_model->y_index++;		/* update y-state model */
   
   for (domain = 0; domain < qac_model->n; domain++)
      if (qac_model->index [domain] > 1020) /* check for overflow */
	 qac_model->index [domain] = 1020; 
   if (qac_model->y_index > 1020)	/* check for overflow */
      qac_model->y_index = 1020; 
}

static bool_t
qac_append (unsigned new_state, unsigned level, const wfa_t *wfa, void *model)
{
   qac_model_t	*qac_model = (qac_model_t *) model; /* probability model */
   
   if (qac_model->n >= qac_model->max_domains)
      return NO;			/* don't use state in domain pool */
   else
   {
      qac_model->index [qac_model->n]
	 = qac_model->n > 0 ? qac_model->index [qac_model->n - 1] : 0;
      qac_model->states [qac_model->n] = new_state;
      qac_model->n++;

      return YES;			/* state will be used in domain pool */
   }
}

static void
qac_chroma (unsigned max_domains, const wfa_t *wfa, void *model)
{
   qac_model_t *qac_model = (qac_model_t *) model; /* probability model */
   
   if (max_domains < qac_model->n)	/* choose most probable domains */
   {
      word_t   *domains;
      unsigned  n, new, old;
      word_t   *states = Calloc (max_domains, sizeof (word_t));
      word_t   *index  = Calloc (max_domains, sizeof (word_t));
   
      domains = compute_hits (wfa->basis_states, wfa->states - 1,
			      max_domains, wfa);
      for (n = 0; n < max_domains && domains [n] >= 0; n++)
	 states [n] = domains [n];
      max_domains = min (max_domains, n);
      Free (domains);

      for (old = 0, new = 0; new < max_domains && old < qac_model->n; old++)
	 if (qac_model->states [old] == states [new])
	    index [new++] = qac_model->index [old];

      Free (qac_model->states);
      Free (qac_model->index);
      qac_model->states      = states;
      qac_model->index       = index;
      qac_model->n           = max_domains;
      qac_model->max_domains = max_domains;
   }
   qac_model->y_index 	  = 0;
   qac_model->max_domains = qac_model->n;
}

/*****************************************************************************
			  const domain pool
*****************************************************************************/

static domain_pool_t *
alloc_const_domain_pool (unsigned max_domains, unsigned max_edges,
			 const wfa_t *wfa)
/*
 *  Domain pool with state image 0 (constant function f(x, y) = 1).
 *  No probability model is used.
 */
{
   domain_pool_t *pool;
   
   pool           = default_alloc ();	
   pool->generate = const_generate;
   pool->bits     = const_bits;
   
   return pool;
}

static word_t *
const_generate (unsigned level, int y_state, const wfa_t *wfa,
		const void *model)
{
   word_t *domains = Calloc (2, sizeof (word_t));
   
   domains [0] = 0;
   domains [1] = -1;
   
   return domains;
}

static real_t
const_bits (const word_t *domains, const word_t *used_domains,
	    unsigned level, int y_state, const wfa_t *wfa, const void *model)
{
   return 0;				/* 0 bits,
					   either we have a lc or not */
}

/*****************************************************************************
				basis domain pool
*****************************************************************************/

static domain_pool_t *
alloc_basis_domain_pool (unsigned max_domains, unsigned max_edges,
			 const wfa_t *wfa)
/*
 *  Domain pool with state images {0, ..., 'basis_states' - 1).
 *  Underlying probability model: quasi arithmetic coding of columns.
 *  I.e. domain pool = qac_domainpool ('max_domains' == wfa->basis_states)
 */
{
   return alloc_qac_domain_pool (wfa->basis_states, max_edges, wfa);
}

/*****************************************************************************
			  uniform-distribution pool
*****************************************************************************/

static domain_pool_t *
alloc_uniform_domain_pool (unsigned max_domains, unsigned max_edges,
			   const wfa_t *wfa)
/*
 *  Domain pool with state images {0, ..., 'max_domains').
 *  Underlying probability model: uniform distribution.
 */
{
   domain_pool_t *pool;
   
   pool           = default_alloc ();	
   pool->generate = uniform_generate;
   pool->bits     = uniform_bits;
   
   return pool;
}

static word_t *
uniform_generate (unsigned level, int y_state, const wfa_t *wfa,
		  const void *model)
{
   unsigned  state, n;
   word_t   *domains = Calloc (wfa->states + 1, sizeof (word_t));

   for (state = 0, n = 0; state < wfa->states; state++)
      if (usedomain (state, wfa))
	 domains [n++] = state;
   domains [n] = -1;
   
   return domains;
}
 
static real_t
uniform_bits (const word_t *domains, const word_t *used_domains,
	      unsigned level, int y_state, const wfa_t *wfa, const void *model)
{
   unsigned state, n;
   real_t   bits = 0;
   
   for (state = 0, n = 0; state < wfa->states; state++)
      if (usedomain (state, wfa))
	 n++;

   bits = - n * log2 ((n - 1) / (real_t) n);

   if (used_domains != NULL)
   {
      int edge;
      
      for (edge = 0; isedge (used_domains [edge]); edge++)
	 bits -= log2 (1.0 / n);
   }

   return bits;
}

/*****************************************************************************
			  run length encoding pool
*****************************************************************************/

typedef struct rle_model
{
   word_t	count [MAXEDGES + 1];
   u_word_t	total;
   u_word_t	n;
   u_word_t	max_domains;
   u_word_t	y_index;		/* pointer to prob of Y domain */
   word_t      *states;			/* mapping states -> domains */
   qac_model_t *domain_0;
} rle_model_t;

static domain_pool_t *
alloc_rle_domain_pool (unsigned max_domains, unsigned max_edges,
		       const wfa_t *wfa)
/*
 *  Domain pool with state images {0, ..., 'max_domains').
 *  Underlying probability model: rle 
 */
{
   domain_pool_t *pool;
   unsigned	  state;
   
   pool                  = default_alloc ();	
   pool->model           = rle_model_alloc (max_domains);
   pool->model_free      = rle_model_free;
   pool->model_duplicate = rle_model_duplicate;
   pool->generate        = rle_generate;
   pool->update          = rle_update;
   pool->bits            = rle_bits;
   pool->append          = rle_append;
   pool->chroma          = rle_chroma;

   for (state = 0; state < wfa->basis_states; state++)
      if (usedomain (state, wfa))
	  rle_append (state, -1, wfa, pool->model);

   return pool;
}

static void *
rle_model_alloc (unsigned max_domains)
{
   unsigned	m;
   rle_model_t *model = Calloc (1, sizeof (rle_model_t));
   
   for (m = model->total = 0; m < MAXEDGES + 1; m++, model->total++)
      model->count [m] = 1;

   model->domain_0    = qac_model_alloc (1);
   model->states      = Calloc (max_domains, sizeof (word_t));
   model->n	      = 0;
   model->y_index     = 0;
   model->max_domains = max_domains;
   
   return model;
}

static void
rle_model_free (void *model)
{
   qac_model_free (((rle_model_t *) model)->domain_0);
   Free (((rle_model_t *) model)->states);
   Free (model);
}

static void *
rle_model_duplicate (const void *src)
{
   const rle_model_t *rle_src = (rle_model_t *) src;
   rle_model_t	     *model   = Calloc (1, sizeof (rle_model_t));

   model->domain_0    = qac_model_duplicate (rle_src->domain_0);
   model->n	      = rle_src->n;
   model->max_domains = rle_src->max_domains;
   model->states      = Calloc (model->max_domains, sizeof (word_t));
   model->total       = rle_src->total;
   model->y_index     = rle_src->y_index;
   
   memcpy (model->states, rle_src->states,
	   model->max_domains * sizeof (word_t));
   memcpy (model->count, rle_src->count,
	   (MAXEDGES + 1) * sizeof (word_t));
   
   return model;
}

static word_t *
rle_generate (unsigned level, int y_state, const wfa_t *wfa, const void *model)
{
   word_t      *domains;
   unsigned	n;
   rle_model_t *rle_model 	  = (rle_model_t *) model;
   bool_t	y_state_is_domain = NO;
   
   if (y_state >= 0 && !usedomain (y_state, wfa)) /* don't use y-state */
      y_state = -1;
   
   domains = Calloc (rle_model->n + 2, sizeof (word_t));

   memcpy (domains, rle_model->states, rle_model->n * sizeof (word_t));

   for (n = 0; n < rle_model->n; n++)
      if (domains [n] == y_state)	/* match */
	 y_state_is_domain = YES;		

   if (y_state_is_domain)
      domains [rle_model->n] = -1;	/* end marker */
   else
   {
      domains [rle_model->n]     = y_state; /* additional y-state */
      domains [rle_model->n + 1] = -1;	/* end marker */
   }

   return domains;
}

static real_t
rle_bits (const word_t *domains, const word_t *used_domains,
	  unsigned level, int y_state, const wfa_t *wfa, const void *model)
{
   unsigned	edge;
   unsigned	n 	  = 0;
   real_t	bits 	  = 0;
   word_t	sorted [MAXEDGES + 1];
   rle_model_t *rle_model = (rle_model_t *) model;
   unsigned	last;
   int		into;
   
   if (y_state >= 0 && !usedomain (y_state, wfa)) /* don't use y-state */
      y_state = -1;

   if (used_domains)
   {
      word_t domain;
      
      if (y_state >= 0)
	 bits += matrix_0 [rle_model->y_index];
      
      for (edge = n = 0; isedge (domain = used_domains [edge]); edge++)
	 if (domains [domain] != y_state)
	    sorted [n++] = used_domains [edge];
	 else
	 {
	    bits -= matrix_0 [rle_model->y_index];
	    bits += matrix_1 [rle_model->y_index];
	 }
      
      if (n > 1)
	 qsort (sorted, n, sizeof (word_t), sort_asc_word);
   }

   bits = - log2 (rle_model->count [n] / (real_t) rle_model->total);
   if (used_domains && n && sorted [0] == 0)
   {
      word_t array0 [2] = {0, NO_EDGE};
      bits += qac_bits (array0, array0, level, y_state, wfa, rle_model->domain_0);
   }
   else
   {
      word_t array0 [2] = {NO_EDGE};
      bits += qac_bits (array0, array0, level, y_state, wfa, rle_model->domain_0);
   }
   
   last = 1;
   for (edge = 0; edge < n; edge++)
      if ((into = sorted [edge]) && rle_model->n - 1 - last)
      {
	 bits += bits_bin_code (into - last, rle_model->n - 1 - last);
	 last  = into + 1;
      }
   
   return bits;
}

static void
rle_update (const word_t *domains, const word_t *used_domains,
	    unsigned level, int y_state, const wfa_t *wfa, void *model)
{
   rle_model_t *rle_model  = (rle_model_t *) model;
   bool_t	state_0    = NO, state_y = NO;
   word_t	array0 [2] = {0, NO_EDGE};
   unsigned 	edge = 0;
   
   if (y_state >= 0 && !usedomain (y_state, wfa)) /* don't use y-state */
      y_state = -1;

   if (used_domains)
   {
      word_t   domain;
      
      for (edge = 0; isedge (domain = used_domains [edge]); edge++)
	 if (domains [domain] == 0)
	    state_0 = YES;
	 else if (domains [domain] == y_state)
	    state_y = YES;
   }
   
   rle_model->count [edge]++;
   rle_model->total++;

   qac_update (array0, array0 + (state_0 ? 0 : 1), level, y_state, wfa,
	       rle_model->domain_0);

   if (state_y)
      rle_model->y_index >>= 1;
   else
      rle_model->y_index++;
   if (rle_model->y_index > 1020)	/* check for overflow */
      rle_model->y_index = 1020; 
}

static bool_t
rle_append (unsigned new_state, unsigned level, const wfa_t *wfa, void *model)
{
   rle_model_t *rle_model = (rle_model_t *) model; /* probability model */
   
   if (rle_model->n >= rle_model->max_domains)
      return NO;			/* don't use state in domain pool */
   else
   {
      rle_model->states [rle_model->n] = new_state;
      rle_model->n++;

      if (new_state == 0)
      {
	 assert (rle_model->n == 1);
	 qac_append (0, -1, wfa, rle_model->domain_0);
      }
      
      return YES;			/* state will be used in domain pool */
   }
}

static void
rle_chroma (unsigned max_domains, const wfa_t *wfa, void *model)
{
   rle_model_t *rle_model = (rle_model_t *) model; /* probability model */
   
   if (max_domains < rle_model->n)	/* choose most probable domains */
   {
      unsigned  n;
      word_t   *states  = Calloc (max_domains, sizeof (word_t));
      word_t   *domains = compute_hits (wfa->basis_states, wfa->states - 1,
					max_domains, wfa);
      
      for (n = 0; n < max_domains && domains [n] >= 0; n++)
	 states [n] = domains [n];

      assert (states [0] == 0);
      max_domains = min (max_domains, n);
      Free (domains);

      Free (rle_model->states);
      rle_model->states = states;
      rle_model->n      = max_domains;
   }
   rle_model->y_index 	  = 0;
   rle_model->max_domains = rle_model->n;
}

/*****************************************************************************
		 run length encoding pool no special chroma pool
*****************************************************************************/

static domain_pool_t *
alloc_rle_no_chroma_domain_pool (unsigned max_domains, unsigned max_edges,
				 const wfa_t *wfa)
/*
 *  Domain pool with state images {0, ..., 'max_domains').
 *  Underlying probability model: rle 
 *  Domain pool is not changed for chroma bands
 */
{
   domain_pool_t *pool = alloc_rle_domain_pool (max_domains, max_edges, wfa);
   
   pool->chroma = default_chroma;

   return pool;
}

/*****************************************************************************
			  default functions (see domain-pool.h)
*****************************************************************************/

static domain_pool_t *
default_alloc (void)
{
   domain_pool_t *pool;

   pool                  = Calloc (1, sizeof (domain_pool_t));
   pool->model           = NULL;
   pool->generate        = NULL;
   pool->bits            = NULL;
   pool->update          = default_update;
   pool->append          = default_append;
   pool->chroma          = default_chroma;
   pool->free            = default_free;
   pool->model_free      = default_model_free;
   pool->model_duplicate = default_model_duplicate;
   
   return pool;
}

static void *
default_model_duplicate (const void *src)
{
   return NULL;
}

static void *
default_model_alloc (unsigned max_domains)
{
   return NULL;
}

static void
default_model_free (void *model)
{
   if (model)
      Free (model);
}

static void
default_free (domain_pool_t *pool)
{
   pool->model_free (pool->model);
   Free (pool);
}

static void
default_update (const word_t *domains, const word_t *used_domains,
		unsigned level, int y_state, const wfa_t *wfa, void *model)
{
   return;				/* nothing to do */
}

static bool_t
default_append (unsigned new_state, unsigned level,
		const wfa_t *wfa, void *model)
{
   return YES;				/* use every state in lin comb */
}

static void
default_chroma (unsigned max_domains, const wfa_t *wfa, void *model)
{
   return;				/* don't alter domain pool */
}

static void
init_matrix_probabilities (void)
/*
 *  Compute the information contents of matrix element '0' and '1' for
 *  each possible probability index 0, ... ,  1023. These values are
 *  obtained from the probability model in the quasi arithmetic
 *  coding module.
 *
 *  No return value.
 *
 *  Side effects:
 *	local arrays matrix_0 and matrix_1 are initialized if not already done.
 */
{
   if (matrix_0 == NULL || matrix_1 == NULL)
   {
      unsigned index;			
      unsigned n, exp;
      
      matrix_0 = Calloc (1 << (MAX_PROB + 1), sizeof (real_t));
      matrix_1 = Calloc (1 << (MAX_PROB + 1), sizeof (real_t));
   
      for (index = 0, n = MIN_PROB; n <= MAX_PROB; n++)
	 for (exp = 0; exp < (unsigned) 1 << n; exp++, index++)
	 {
	    matrix_1 [index] = -log2 (1 / (real_t) (1 << n));
	    matrix_0 [index] = -log2 (1 - 1 / (real_t) (1 << n));
	 }
   }
}
