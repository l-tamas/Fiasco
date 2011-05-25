/*
 *  basis.c:		WFA initial basis files	
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/25 16:38:06 $
 *  $Author: hafner $
 *  $Revision: 5.3 $
 *  $State: Exp $
 */

#include "config.h"

#include "types.h"
#include "macros.h"
#include "error.h"

#include "wfa.h"
#include "wfalib.h"

#include "basis.h"

typedef struct basis_values
{
   unsigned  states;
   real_t   *final;
   bool_t   *use_domain;
   real_t (*transitions)[4];
} basis_values_t;

typedef struct
{
   char *filename;
   void (*function)(basis_values_t *bv);
} basis_file_t;

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
small_init (basis_values_t *bv);

static const basis_file_t basis_files[] = { {"small.fco", small_init},
					    {"small.wfa", small_init},
					    {NULL, NULL} };

/*****************************************************************************

				public code
  
*****************************************************************************/

bool_t
get_linked_basis (const char *basis_name, wfa_t *wfa)
/*
 *  Check wether given WFA initial basis 'basis_name' is already linked
 *  with the excecutable. If the basis is available then fill the 'wfa' struct
 *  according to the stored data, otherwise print a warning message.
 *
 *  Return value:
 *	true on success, false if basis is not available yet.
 *
 *  Side effects:
 *	'wfa' struct is filled on success.
 */
{
   bool_t	  success = NO;		/* indicates if basis is found */
   unsigned	  n;			/* counter */
   basis_values_t bv;			/* basis values */
   
   for (n = 0; basis_files [n].filename != NULL; n++)
      if (streq (basis_files [n].filename, basis_name))	/* basis is stored */
      {
	 unsigned state, edge;		
	 
	 (*basis_files [n].function) (&bv); /* initialize local variables */
	 /*
	  *  Generate WFA
	  */
	 wfa->basis_states = wfa->states = bv.states + 1;
	 wfa->domain_type[0]             = USE_DOMAIN_MASK; 
	 wfa->final_distribution[0]      = 128;
	 append_edge (0, 0, 1.0, 0, wfa);
	 append_edge (0, 0, 1.0, 1, wfa);
	 for (state = 1; state < wfa->basis_states; state++)
	 {
	    wfa->final_distribution [state] = bv.final [state - 1];
	    wfa->domain_type [state]        = bv.use_domain [state - 1]
					      ? USE_DOMAIN_MASK
					      : AUXILIARY_MASK;
	 }
	 for (edge = 0; isedge (bv.transitions [edge][0]); edge++)
	    append_edge (bv.transitions [edge][0], bv.transitions [edge][1],
			 bv.transitions [edge][2], bv.transitions [edge][3],
			 wfa);
	 
	 success = YES;
	 break;
      }

   if (!success)
      warning ("WFA initial basis '%s' isn't linked with the excecutable yet."
	       "\nLoading basis from disk instead.", basis_name);

   return success;
}

/*****************************************************************************

				private code
  
*****************************************************************************/

/*****************************************************************************
				basis "small.wfa"
*****************************************************************************/

static unsigned	states_small           = 2;
static bool_t	use_domain_small[]     = {YES, YES};
static real_t 	final_small[]          = {64, 64};
static real_t 	transitions_small[][4] = {{1, 2, 0.5, 0}, {1, 2, 0.5, 1},
				      	 {1, 0, 0.5, 1}, {2, 1, 1.0, 0},
				      	 {2, 1, 1.0, 1}, {-1, 0, 0, 0}};
static void
small_init (basis_values_t *bv)
{
   bv->states      = states_small;
   bv->final       = final_small;
   bv->use_domain  = use_domain_small;
   bv->transitions = transitions_small;
}
