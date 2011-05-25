/*
 *  write.c:		Output of WFA files
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/18 15:44:59 $
 *  $Author: hafner $
 *  $Revision: 5.3 $
 *  $State: Exp $
 */

#include "config.h"

#include "types.h"
#include "macros.h"
#include "error.h"

#include "cwfa.h"
#include "image.h"
#include "misc.h"
#include "bit-io.h"
#include "rpf.h"

#include "tree.h"
#include "matrices.h"
#include "weights.h"
#include "mc.h"
#include "nd.h"
#include "write.h"
 
/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
write_tiling (const tiling_t *tiling, bitfile_t *output);

/*****************************************************************************

				public code

				
*****************************************************************************/


void
write_next_wfa (const wfa_t *wfa, const coding_t *c, bitfile_t *output)
/*
 *  Write 'wfa' to stream 'output'. If the first frame should be written
 *  then also store the header information of the coding struct 'c'.
 *
 *  No return value.
 */
{
   unsigned edges = 0;			/* number of transitions */
   unsigned bits;
   
   debug_message ("--------------------------------------"
		  "--------------------------------------");

   if (c->mt->number == 0)				/* first WFA */
      write_header (wfa->wfainfo, output);
  
   bits = bits_processed (output);
   
   /*
    *  Frame header information
    */
   {
      const int rice_k = 8;		/* parameter of Rice Code */

      write_rice_code (wfa->states, rice_k, output);      
      write_rice_code (c->mt->frame_type, rice_k, output); 
      write_rice_code (c->mt->number, rice_k, output);     
   }
   
   OUTPUT_BYTE_ALIGN (output);

   debug_message ("frame-header: %5d bits.", bits_processed (output) - bits);

   if (c->tiling->exponent)		/* write tiling permutation */
   {
      put_bit (output, 1);
      write_tiling (c->tiling, output);
   }
   else
      put_bit (output, 0);

   OUTPUT_BYTE_ALIGN (output);

   write_tree (wfa, output);

   if (c->options.prediction)		/* write nondeterministic approx. */
   {
      put_bit (output, 1); 
      write_nd (wfa, output);
   }
   else
      put_bit (output, 0);

   if (c->mt->frame_type != I_FRAME)	/* write motion compensation info */
      write_mc (c->mt->frame_type, wfa, output);
   
   edges = write_matrices (c->options.normal_domains,
			   c->options.delta_domains, wfa, output);

   if (edges)				/* found at least one approximation */
      write_weights (edges, wfa, output);

   debug_message ("--------------------------------------"
		  "--------------------------------------");
}

void
write_header (const wfa_info_t *wi, bitfile_t *output)
/*
 *  Write the header information describing the type of 'wfa'
 *  to stream 'output'.
 *
 *  No return value.
 */
{
   const unsigned rice_k = 8;		/* parameter of Rice Code */
   unsigned	  bits 	 = bits_processed (output);
   char *text;			/* next character to write */

   /*
    *  Write magic number and name of initial basis
    */
   for (text = FIASCO_MAGIC; *text; text++)
      put_bits (output, *text, 8);
   put_bits (output, '\n', 8);
   for (text = wi->basis_name; *text; text++)
      put_bits (output, *text, 8);
   put_bits (output, *text, 8);
   
   write_rice_code (FIASCO_BINFILE_RELEASE, rice_k, output);

   write_rice_code (HEADER_TITLE, rice_k, output);
   for (text = wi->title;
	text && *text && text - wi->title < MAXSTRLEN - 2;
	text++)
      put_bits (output, *text, 8);
   put_bits (output, 0, 8);
   
   write_rice_code (HEADER_COMMENT, rice_k, output);
   for (text = wi->comment;
	text && *text && text - wi->comment < MAXSTRLEN - 2;
	text++)
      put_bits (output, *text, 8);
   put_bits (output, 0, 8);
   
   write_rice_code (HEADER_END, rice_k, output);
   
   write_rice_code (wi->max_states, rice_k, output); 
   put_bit (output, wi->color ? 1 : 0); 
   write_rice_code (wi->width, rice_k, output);
   write_rice_code (wi->height, rice_k, output);
   if (wi->color)
      write_rice_code (wi->chroma_max_states, rice_k, output); 
   write_rice_code (wi->p_min_level, rice_k, output); 
   write_rice_code (wi->p_max_level, rice_k, output); 
   write_rice_code (wi->frames, rice_k, output);
   write_rice_code (wi->smoothing, rice_k, output);

   put_bits (output, wi->rpf->mantissa_bits - 2, 3);
   put_bits (output, wi->rpf->range_e, 2);
   if (wi->rpf->mantissa_bits != wi->dc_rpf->mantissa_bits ||
       wi->rpf->range != wi->dc_rpf->range)
   {
      put_bit (output, YES);
      put_bits (output, wi->dc_rpf->mantissa_bits - 2, 3);
      put_bits (output, wi->dc_rpf->range_e, 2);
   }
   else
      put_bit (output, NO);
   if (wi->rpf->mantissa_bits != wi->d_rpf->mantissa_bits ||
       wi->rpf->range != wi->d_rpf->range)
   {
      put_bit (output, YES);
      put_bits (output, wi->d_rpf->mantissa_bits - 2, 3);
      put_bits (output, wi->d_rpf->range_e, 2);
   }
   else
      put_bit (output, NO);
   if (wi->dc_rpf->mantissa_bits != wi->d_dc_rpf->mantissa_bits ||
       wi->dc_rpf->range != wi->d_dc_rpf->range)
   {
      put_bit (output, YES);
      put_bits (output, wi->d_dc_rpf->mantissa_bits - 2, 3);
      put_bits (output, wi->d_dc_rpf->range_e, 2);
   }
   else
      put_bit (output, NO);

   if (wi->frames > 1)			/* motion compensation stuff */
   {
      write_rice_code (wi->fps, rice_k, output); 
      write_rice_code (wi->search_range, rice_k, output); 
      put_bit (output, wi->half_pixel ? 1 : 0); 
      put_bit (output, wi->B_as_past_ref ? 1 : 0);
   }

   OUTPUT_BYTE_ALIGN (output);
   debug_message ("header:         %d bits.", bits_processed (output) - bits);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
write_tiling (const tiling_t *tiling, bitfile_t *output)
/*
 *  Write image tiling information given by 'tiling' to stream 'output'.
 *
 *  No return value.
 */
{
   const unsigned rice_k = 8;		/* parameter of Rice Code */
   unsigned  	  bits   = bits_processed (output);
   
   write_rice_code (tiling->exponent, rice_k, output);
   if (tiling->method == FIASCO_TILING_VARIANCE_ASC
       || tiling->method == FIASCO_TILING_VARIANCE_DSC)
   {
      unsigned tile;			/* current image tile */

      put_bit (output, 1);		
      for (tile = 0; tile < 1U << tiling->exponent; tile++)
	 if (tiling->vorder [tile] != -1) /* image tile is visible */
	    put_bits (output, tiling->vorder [tile], tiling->exponent);
   }
   else
   {
      put_bit (output, 0);		
      put_bit (output, tiling->method == FIASCO_TILING_SPIRAL_ASC);
   }

   debug_message ("tiling:        %4d bits.", bits_processed (output) - bits);
}
