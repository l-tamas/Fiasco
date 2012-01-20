/*
 *  read.c:		Input of WFA files
 *
 *  Written by:		Ullrich Hafner
 *  
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/18 15:44:58 $
 *  $Author: hafner $
 *  $Revision: 5.4 $
 *  $State: Exp $
 */

#include "config.h"

#include <stdio.h>

#if STDC_HEADERS
#	include <string.h>
#else /* not STDC_HEADERS */
#	if HAVE_STRING_H
#		include <string.h>
#	else /* not HAVE_STRING_H */
#		include <strings.h>
#	endif /* not HAVE_STRING_H */
#endif /* not STDC_HEADERS */

#include "types.h"
#include "macros.h"
#include "error.h"

#include "wfa.h"
#include "misc.h"
#include "rpf.h"
#include "bit-io.h"
#include "wfalib.h"

#include "tree.h"
#include "matrices.h"
#include "weights.h"
#include "nd.h"
#include "mc.h"
#include "basis.h"
#include "read.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
read_tiling (tiling_t *tiling, unsigned image_width, unsigned image_height,
	     unsigned image_level, bitfile_t *input);

/*****************************************************************************

				public code
  
*****************************************************************************/

bitfile_t *
open_wfa (const char *filename, wfa_info_t *wi)
/*
 *  Open WFA file 'filename' and read header information.
 *
 *  Return value:
 *	Pointer to input stream (fileposition: first WFA frame)
 *
 *  Side effects:
 *	The values of the header of 'filename' are copied to 'wfainfo'. 
 *
 */
{
   bitfile_t *input;			/* pointer to WFA bitfile */
   
   assert (filename && wi);
   
   wi->wfa_name = strdup (filename);

   /*
    *  Check whether 'filename' is a regular WFA file
    */
   {
      unsigned 	n;
      char     *str;
      
      if (!(input = open_bitfile (filename, "FIASCO_DATA", READ_ACCESS)))
	 file_error (filename);
   
      for (str = FIASCO_MAGIC, n = strlen (FIASCO_MAGIC); n; n--)
	 if (get_bits (input, 8) != (unsigned) *str++)
	    error ("Input file %s is not a valid FIASCO file!", filename);
      get_bits (input, 8);		/* fetch newline */
   }
   
   /*
    *  Read WFA header information
    */
   {
      char	      basis_name [MAXSTRLEN]; /* temp. buffer */
      const unsigned  rice_k = 8; 	/* parameter of Rice Code */
      char     	     *str    = basis_name;
      
      while ((*str++ = get_bits (input, 8)) != 0
	     && str < basis_name + MAXSTRLEN)
	 ;
      if (str == basis_name + MAXSTRLEN)
	 error ("Input file %s is not a valid FIASCO file!", filename);
      
      {
	 wi->release = read_rice_code (rice_k, input);

	 if (wi->release > FIASCO_BINFILE_RELEASE)
	    error ("Can't decode FIASCO files of file format release `%d'."
		   "\nCurrent file format release is `%d'.", wi->release,
		   FIASCO_BINFILE_RELEASE);
      }

      if (wi->release > 1)
      {
	 header_type_e type;
	 
	 while ((type = read_rice_code (rice_k, input)) != HEADER_END)
	 {
	    char     buffer [MAXSTRLEN];
	    unsigned n = 0;
	    
	    switch (type)
	    {
	       case HEADER_TITLE:
		  while ((buffer [n++] = get_bits (input, 8)))
		     ;
		  wi->title = strdup (buffer);
		  break;
	       case HEADER_COMMENT:
		  while ((buffer [n++] = get_bits (input, 8)))
		     ;
		  wi->comment = strdup (buffer);
		  break;
	       default:			/* should not happen */
		  break;
	    }
	 }
      }

      wi->basis_name = strdup (basis_name);
      wi->max_states = read_rice_code (rice_k, input);
      wi->color      = get_bit (input) ? YES : NO;
      wi->width      = read_rice_code (rice_k, input);
      wi->height     = read_rice_code (rice_k, input);

      /*
       *  Compute bintree level
       */
      {
	 unsigned lx = log2 (wi->width - 1) + 1;
	 unsigned ly = log2 (wi->height - 1) + 1;
      
	 wi->level = max (lx, ly) * 2 - ((ly == lx + 1) ? 1 : 0);
      }
      wi->chroma_max_states = wi->color ? read_rice_code (rice_k, input) : -1;
      wi->p_min_level       = read_rice_code (rice_k, input);
      wi->p_max_level       = read_rice_code (rice_k, input);
      wi->frames            = read_rice_code (rice_k, input);
      wi->smoothing	    = read_rice_code (rice_k, input);

      /*
       *  Read RPF models from disk
       */
      {
	 unsigned 	    mantissa;
	 fiasco_rpf_range_e range;

	 mantissa = get_bits (input, 3) + 2;
	 range    = get_bits (input, 2);
	 wi->rpf  = alloc_rpf (mantissa, range);
	 
	 if (get_bit (input))		/* different DC model */
	 {
	    mantissa   = get_bits (input, 3) + 2;
	    range      = get_bits (input, 2);
	    wi->dc_rpf = alloc_rpf (mantissa, range);
	 }
	 else				/* use same model for DC coefficents */
	    wi->dc_rpf = alloc_rpf (wi->rpf->mantissa_bits,
				    wi->rpf->range_e);

	 if (get_bit (input))		/* different delta model */
	 {
	    mantissa  = get_bits (input, 3) + 2;
	    range     = get_bits (input, 2);
	    wi->d_rpf = alloc_rpf (mantissa, range);
	 }
	 else
	    wi->d_rpf = alloc_rpf (wi->rpf->mantissa_bits,
				   wi->rpf->range_e);
	 
	 if (get_bit (input))		/* different DC delta model */
	 {
	    mantissa  	 = get_bits (input, 3) + 2;
	    range     	 = get_bits (input, 2);
	    wi->d_dc_rpf = alloc_rpf (mantissa, range);
	 }
	 else
	    wi->d_dc_rpf = alloc_rpf (wi->dc_rpf->mantissa_bits,
				      wi->dc_rpf->range_e);
      }

      if (wi->frames > 1)		/* motion compensation stuff */
      {
	 wi->fps           = read_rice_code (rice_k, input);
	 wi->search_range  = read_rice_code (rice_k, input);
	 wi->half_pixel    = get_bit (input) ? YES : NO;
	 wi->B_as_past_ref = get_bit (input) ? YES : NO;
      }
   }
   
   INPUT_BYTE_ALIGN (input);

   return input;
}

void
read_basis (const char *filename, wfa_t *wfa)
/*
 *  Read WFA initial basis 'filename' and fill 'wfa' struct.
 *
 *  No return value.
 *
 *  Side effects:
 *	wfa->into, wfa->weights, wfa->final_distribution, wfa->basis_states
 *	wfa->domain_type wfa->wfainfo->basis_name, are filled with the
 *	values of the WFA basis.
 */
{
   FILE	*input;				/* ASCII WFA initial basis file */

   assert (filename && wfa);

   if (!wfa->wfainfo->basis_name ||
       !streq (wfa->wfainfo->basis_name, filename))
   {
      if (wfa->wfainfo->basis_name)
	 fiasco_free (wfa->wfainfo->basis_name);
      wfa->wfainfo->basis_name = strdup (filename);
   }
   
   if (get_linked_basis (filename, wfa))
      return;				/* basis is linked with excecutable */
   
   /*
    *  Check whether 'wfa_name' is a regular ASCII WFA initial basis file
    */
   {
      char magic [MAXSTRLEN];		/* WFA magic number */

      if (!(input = open_file (filename, "FIASCO_DATA", READ_ACCESS)))
	 file_error(filename);
      
      if (fscanf (input, MAXSTRLEN_SCANF, magic) != 1)
	 error ("Format error: ASCII FIASCO initial basis file %s", filename);
      else if (strneq (FIASCO_BASIS_MAGIC, magic))
	 error ("Input file %s is not an ASCII FIASCO initial basis!",
		filename);
   }
   
   /*
    *  WFA ASCII format:
    *
    *  Note: State 0 is assumed to be the constant function f(x, y) = 128.
    *        Don't define any transitions of state 0 in an initial basis. 
    *
    *  Header:
    *   type		|description
    *	----------------+-----------
    *   string		|MAGIC Number "Wfa"
    *	int		|Number of basis states 'N'
    *	bool_t-array[N]	|use vector in linear combinations,
    *			|0: don't use vector (auxilliary state)
    *			|1: use vector in linear combinations
    *	float-array[N]	|final distribution of every state
    *
    *  Transitions:
    *
    *      <state 1>			current state
    *      <label> <into> <weight>	transition 1 of current state
    *      <label> <into> <weight>	transition 2 of current state
    *      ...
    *      <-1>				last transition marker
    *      <state 2>
    *      ...
    *      <-1>				last transition marker
    *      <state N>
    *      ...
    *
    *      <-1>				last transition marker
    *      <-1>				last state marker
    */
   {
      unsigned state;

      if (fscanf (input ,"%d", &wfa->basis_states) != 1)
	 error ("Format error: ASCII FIASCO initial basis file %s", filename);

      /*
       *  State 0 is assumed to be the constant function f(x, y) = 128.
       */
      wfa->domain_type [0]        = USE_DOMAIN_MASK; 
      wfa->final_distribution [0] = 128;
      wfa->states 		  = wfa->basis_states;
      wfa->basis_states++;

      append_edge (0, 0, 1.0, 0, wfa);
      append_edge (0, 0, 1.0, 1, wfa);
   
      for (state = 1; state < wfa->basis_states; state++)
	 wfa->domain_type [state]
	    = read_int (input) ? USE_DOMAIN_MASK : AUXILIARY_MASK;

      for (state = 1; state < wfa->basis_states; state++)
	 wfa->final_distribution[state] = read_real (input);

      /*
       *  Read transitions
       */
      for (state = 1; state < wfa->basis_states; state++)
      {
	 unsigned domain;
	 int      label;
	 real_t   weight;

	 if (read_int (input) != (int) state)
	    error ("Format error: ASCII FIASCO initial basis file %s",
		   filename);

	 while((label = read_int (input)) != -1)
	 {
	    domain = read_int (input);
	    weight = read_real (input);
	    append_edge (state, domain, weight, label, wfa);
	 }
      }
   }
   
   fclose (input);
}

unsigned
read_next_wfa (wfa_t *wfa, bitfile_t *input)
/*
 *  Read next WFA frame of the WFA stream 'input'.
 *  WFA header information has to be already present in the 'wfainfo' struct.
 *  (i.e. open_wfa must be called first!)
 *  
 *  No return value.
 *
 *  Side effects:
 *	wfa->into, wfa->weights, wfa->final_distribution, wfa->states
 *	wfa->x, wfa->y, wfa->level_of_state, wfa->domain_type
 *      mt->type, mt->number are filled with the values of the WFA file.
 */
{
   tiling_t tiling;			/* tiling information */
   unsigned frame_number;		/* current frame number */
   
   assert (wfa && input);
   
   /*
    *  Frame header information
    */
   {
      const unsigned rice_k = 8;	/* parameter of Rice Code */

      wfa->states     = read_rice_code (rice_k, input);
      wfa->frame_type = read_rice_code (rice_k, input);
      frame_number    = read_rice_code (rice_k, input);
   }

   if (wfa->wfainfo->release > 1)	/* no alignment in version 1 */
   {
      INPUT_BYTE_ALIGN (input);
   }
   
   /*
    *  Read image tiling info 
    */
   if (get_bit (input))			/* tiling performed ? */
      read_tiling (&tiling, wfa->wfainfo->width, wfa->wfainfo->height,
		   wfa->wfainfo->level, input);
   else
      tiling.exponent = 0;
   
   INPUT_BYTE_ALIGN (input);

   read_tree (wfa, &tiling, input);

   /*
    *  Compute domain pool.
    *  Large images have not been used due to image tiling.
    */
   {
      unsigned state;
   
      for (state = wfa->basis_states; state < wfa->states; state++)
	 if ((!wfa->wfainfo->color
	      || (int) state <= wfa->tree [wfa->tree [wfa->root_state][0]][0])
	     &&
	     (!tiling.exponent ||
	      wfa->level_of_state [state] <= (wfa->wfainfo->level
					      - tiling.exponent))
	     && ((wfa->x [state][0]
		 + width_of_level (wfa->level_of_state [state]))
		 <= wfa->wfainfo->width)
	     && ((wfa->y [state][0]
		 + height_of_level (wfa->level_of_state [state]))
		 <= wfa->wfainfo->height))
	    wfa->domain_type [state] = USE_DOMAIN_MASK;
	 else
	    wfa->domain_type [state] = 0;
   }
   
   if (tiling.exponent)
      fiasco_free (tiling.vorder);

   if (get_bit (input))			/* nondeterministic prediction used */
      read_nd (wfa, input);

   if (wfa->frame_type != I_FRAME)	/* motion compensation used */
      read_mc (wfa->frame_type, wfa, input);

   locate_delta_images (wfa);
   
   /*
    *  Read linear combinations (coefficients and indices)
    */
   {
      unsigned edges = read_matrices (wfa, input); 

      if (edges)
	 read_weights (edges, wfa, input);
   }

   /*
    *  Compute final distribution of all states
    */
   {
      unsigned state;
   
      for (state = wfa->basis_states; state <= wfa->states; state++)
	 wfa->final_distribution[state]
	    = compute_final_distribution (state, wfa);
   }

   return frame_number;
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
read_tiling (tiling_t *tiling, unsigned image_width, unsigned image_height,
	     unsigned image_level, bitfile_t *input)
/*
 *  Read image tiling information from the given file 'input'
 *  and store parameters in struct 'tiling'.
 *  
 *  No return value.
 */
{
   const unsigned rice_k = 8;		/* parameter of Rice Code */
   
   tiling->exponent = read_rice_code (rice_k, input);
   
   if (get_bit (input))			/* variance order */
   {
      unsigned tile;			/* current image tile */
      unsigned x0, y0;			/* NW corner of image tile */
      unsigned width, height;		/* size of image tile */

      tiling->vorder = fiasco_calloc (1 << tiling->exponent, sizeof (int));
      for (tile = 0; tile <  1U << tiling->exponent; tile++)
      {
	 locate_subimage (image_level, image_level - tiling->exponent, tile,
			  &x0, &y0, &width, &height);
	 if (x0 < image_width && y0 < image_height) 
	    tiling->vorder [tile] = get_bits (input, tiling->exponent);
	 else
	    tiling->vorder [tile] = -1;
      }
   }
   else					/* spiral order */
   {
      tiling->vorder = fiasco_calloc (1 << tiling->exponent, sizeof (int));
      compute_spiral (tiling->vorder, image_width, image_height,
		      tiling->exponent, get_bit (input) ? YES : NO);
   }
}
