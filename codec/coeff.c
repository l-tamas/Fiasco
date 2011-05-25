/*
 *  coeff.c:		Matching pursuit coefficients probability model
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

#include "types.h"
#include "macros.h"
#include "error.h"

#include "rpf.h"
#include "misc.h"
#include "coeff.h"

/*
 *  Coefficient model interface:
 *  Implementing the coefficients model interface requires the
 *  following steps: 
 *  - Add a constructor that initializes the coeff_t structure
 *  - Allocate new model with default_alloc() 
 *  - Fill the c_array_t coeff_models[] array with constructor and name
 *  - Write code for methods bits() and update()
 *  - Either use default functions for remaining methods or override them
 *  The new model is automatically registered at the command line.
 */

/*****************************************************************************
		  uniform distribution coefficients model
*****************************************************************************/

static coeff_t *
alloc_uniform_coeff_model (rpf_t *rpf, rpf_t *dc_rpf,
			   unsigned min_level, unsigned max_level);
static void
uniform_update (const real_t *used_coeff, const word_t *used_states,
		unsigned level, coeff_t *coeff);
static real_t
uniform_bits (const real_t *used_coeff, const word_t *used_states,
	      unsigned level, const coeff_t *coeff);

/*****************************************************************************
			  default functions
*****************************************************************************/

static void
default_model_free (void *model);
static void *
default_model_duplicate (const coeff_t *coeff, const void *model);
static void
default_free (coeff_t *coeff);
static coeff_t *
default_alloc (rpf_t *rpf, rpf_t *dc_rpf,
	       unsigned min_level, unsigned max_level);

/*****************************************************************************
		adaptive arithmetic coding model
*****************************************************************************/

static coeff_t *
alloc_aac_coeff_model (rpf_t *rpf, rpf_t *dc_rpf,
		       unsigned min_level, unsigned max_level);
static void
aac_model_free (void *model);
static void *
aac_model_alloc (const coeff_t *coeff);
static void *
aac_model_duplicate (const coeff_t *coeff, const void *model);
static void
aac_update (const real_t *used_coeff, const word_t *used_states,
	    unsigned level, coeff_t *coeff);
static real_t
aac_bits (const real_t *used_coeff, const word_t *used_states,
	  unsigned level, const coeff_t *coeff);

/*****************************************************************************

				public code
  
*****************************************************************************/

typedef struct c_array
{
   const char *identifier;
   coeff_t    *(*function) (rpf_t *rpf, rpf_t *dc_rpf,
			    unsigned min_level, unsigned max_level);
} c_array_t;

c_array_t coeff_models[] = {{"adaptive", alloc_aac_coeff_model},
			    {"uniform",	 alloc_uniform_coeff_model},
			    {NULL,	 NULL}};

coeff_t *
alloc_coeff_model (const char *coeff_model_name, rpf_t *rpf, rpf_t *dc_rpf,
		   unsigned min_level, unsigned max_level)
/*
 *  Allocate a new coefficients model which is identified by the string
 *  'coeff_model_name'.  'rpf' and 'dc_rpf' define the reduced
 *  precision formats the should be used to quantize normal and DC
 *  components, respectively. 'min_level' and 'max_level' define the
 *  range of range approximations.
 * 
 *  Return value:
 *	pointer to the allocated coefficients model
 *
 *  Note:
 *      Refer to 'coeff.h' for a short description of the member functions.  */
{
   unsigned n;
   
   for (n = 0; coeff_models [n].identifier; n++) /* step through all id's */
      if (strcaseeq (coeff_models [n].identifier, coeff_model_name)) 
	 return coeff_models [n].function (rpf, dc_rpf, min_level, max_level);

   warning ("Can't initialize coefficients model '%s'. "
	    "Using default value '%s'.",
	    coeff_model_name, coeff_models [0].identifier);

   return coeff_models [0].function (rpf, dc_rpf, min_level, max_level);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

/*****************************************************************************
		  uniform distribution coefficients model
*****************************************************************************/

static coeff_t *
alloc_uniform_coeff_model (rpf_t *rpf, rpf_t *dc_rpf,
			   unsigned min_level, unsigned max_level)
/*
 *  Underlying probability model: uniform distribution.
 *  I.e. each coefficient is written as such with
 *  (1 + exponent_bits + mantissa_bits) bits.
 */
{
   coeff_t *coeff = default_alloc (rpf, dc_rpf, min_level, max_level);

   coeff->bits   = uniform_bits;
   coeff->update = uniform_update;
   
   return coeff;
}

static real_t
uniform_bits (const real_t *used_coeff, const word_t *used_states,
	      unsigned level, const coeff_t *coeff)
{
   unsigned edge;
   real_t   bits = 0;			/* #bits to store coefficients */
   
   for (edge = 0; isedge (used_states [edge]); edge++)
   {
      rpf_t *rpf = used_states [edge] ? coeff->rpf : coeff->dc_rpf;
      
      bits += rpf->mantissa_bits + 1;
   }

   return bits;
}

static void
uniform_update (const real_t *used_coeff, const word_t *used_states,
		unsigned level, coeff_t *coeff)
{
   return;				/* nothing to do */
}

/*****************************************************************************
		adaptive arithmetic coding  model
*****************************************************************************/

typedef struct aac_model
{
   word_t *counts;
   word_t *totals;
} aac_model_t;

static coeff_t *
alloc_aac_coeff_model (rpf_t *rpf, rpf_t *dc_rpf,
		       unsigned min_level, unsigned max_level)
/*
 *  Underlying probability model: adaptive arithmetic coding using
 *  the level of a range as context.
 */
{
   coeff_t *coeff = default_alloc (rpf, dc_rpf, min_level, max_level);
   
   coeff->bits            = aac_bits;
   coeff->update          = aac_update;
   coeff->model_free      = aac_model_free;
   coeff->model_duplicate = aac_model_duplicate;
   coeff->model		  = aac_model_alloc (coeff);
   
   return coeff;
}

static real_t
aac_bits (const real_t *used_coeff, const word_t *used_states,
	  unsigned level, const coeff_t *coeff)
{
   real_t	bits  = 0;		/* # bits to store coefficients */
   unsigned	edge;
   int		state;
   word_t      *counts;
   aac_model_t *model = (aac_model_t *) coeff->model;

   counts = model->counts
	    + (1 << (1 + coeff->dc_rpf->mantissa_bits))
	    + ((level - coeff->min_level)
	       * (1 << (1 + coeff->rpf->mantissa_bits)));
   
   for (edge = 0; isedge (state = used_states [edge]); edge++)
      if (state)
	 bits -= log2 (counts [rtob (used_coeff [edge], coeff->rpf)]
		       / (real_t) model->totals [level
						- coeff->min_level + 1]);
      else
	 bits -= log2 (model->counts [rtob (used_coeff [edge], coeff->dc_rpf)]
		       / (real_t) model->totals [0]);
   
   return bits;
}

static void
aac_update (const real_t *used_coeff, const word_t *used_states,
	    unsigned level, coeff_t *coeff)
{
   unsigned	edge;
   int		state;
   word_t      *counts;
   aac_model_t *model = (aac_model_t *) coeff->model;

   counts = model->counts
	    + (1 << (1 + coeff->dc_rpf->mantissa_bits))
	    + ((level - coeff->min_level)
	       * (1 << (1 + coeff->rpf->mantissa_bits)));

   for (edge = 0; isedge (state = used_states [edge]); edge++)
      if (state)
      {
	 counts [rtob (used_coeff [edge], coeff->rpf)]++;
	 model->totals [level - coeff->min_level + 1]++;
      }
      else
      {
	 model->counts [rtob (used_coeff [edge], coeff->dc_rpf)]++;
	 model->totals [0]++;
      }
}

static void *
aac_model_duplicate (const coeff_t *coeff, const void *model)
{
   aac_model_t *src = (aac_model_t *) model;
   aac_model_t *dst = aac_model_alloc (coeff);

   memcpy (dst->counts, src->counts,
	   sizeof (word_t) * ((coeff->max_level - coeff->min_level + 1)
			      * (1 << (1 + coeff->rpf->mantissa_bits))
			      + (1 << (1 + coeff->dc_rpf->mantissa_bits))));
   memcpy (dst->totals, src->totals,
	   sizeof (word_t) * (coeff->max_level - coeff->min_level + 1 + 1));
   
   return dst;
}

static void *
aac_model_alloc (const coeff_t *coeff)
{
   aac_model_t *model;
   unsigned	size = (coeff->max_level - coeff->min_level + 1)
		       * (1 << (1 + coeff->rpf->mantissa_bits))
		       + (1 << (1 + coeff->dc_rpf->mantissa_bits));
   
   model 	 = Calloc (1, sizeof (aac_model_t));
   model->counts = Calloc (size, sizeof (word_t));
   model->totals = Calloc (coeff->max_level - coeff->min_level + 1 + 1,
			   sizeof (word_t));
   /*
    *  Initialize model
    */
   {
      unsigned  n;
      word_t   *ptr = model->counts;
      
      for (n = size; n; n--)
	 *ptr++ = 1;
      model->totals [0] = 1 << (1 + coeff->dc_rpf->mantissa_bits);
      for (n = coeff->min_level; n <= coeff->max_level; n++)
	 model->totals [n - coeff->min_level + 1]
	    = 1 << (1 + coeff->rpf->mantissa_bits);
   }
   
   return (void *) model;
}

static void
aac_model_free (void *model)
{
   aac_model_t	*aac_model = (aac_model_t *) model;

   if (aac_model)
   {
      Free (aac_model->counts);
      Free (aac_model->totals);
      Free (aac_model);
   }
}

/*****************************************************************************
				default functions
*****************************************************************************/

static coeff_t *
default_alloc (rpf_t *rpf, rpf_t *dc_rpf,
	       unsigned min_level, unsigned max_level)
{
   coeff_t *coeff = Calloc (1, sizeof (coeff_t));

   coeff->rpf 	      	  = rpf;
   coeff->dc_rpf       	  = dc_rpf;
   coeff->min_level	  = min_level;
   coeff->max_level	  = max_level;
   coeff->model	      	  = NULL;
   coeff->bits	      	  = NULL;
   coeff->update      	  = NULL;
   coeff->free	      	  = default_free;
   coeff->model_free  	  = default_model_free;
   coeff->model_duplicate = default_model_duplicate;
   
   return coeff;
}

static void
default_free (coeff_t *coeff)
{
   coeff->model_free (coeff->model);
   Free (coeff);
}

static void *
default_model_duplicate (const coeff_t *coeff, const void *model)
{
   return NULL;
}

static void
default_model_free (void *model)
{
   if (model)
      Free (model);
}
