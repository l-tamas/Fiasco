/*
 *  mwfa.c:		Initialization of MWFA coder
 *
 *  Written by:		Michael Unger
 *			Ullrich Hafner
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

#include <ctype.h>

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
#include "image.h"
#include "mwfa.h"

#include "motion.h"

/*****************************************************************************

			     local variables
  
*****************************************************************************/

unsigned mv_code_table [33][2] =
/*
 *  MPEG's huffman code for vector components. Format: code_value, length
 */
{
   {0x19, 11}, {0x1b, 11}, {0x1d, 11}, {0x1f, 11}, {0x21, 11}, {0x23, 11},
   {0x13, 10}, {0x15, 10}, {0x17, 10}, {0x7, 8}, {0x9, 8}, {0xb, 8}, {0x7, 7},
   {0x3, 5}, {0x3, 4}, {0x3, 3}, {0x1, 1}, {0x2, 3}, {0x2, 4}, {0x2, 5},
   {0x6, 7}, {0xa, 8}, {0x8, 8}, {0x6, 8}, {0x16, 10}, {0x14, 10}, {0x12, 10}, 
   {0x22, 11}, {0x20, 11}, {0x1e, 11}, {0x1c, 11}, {0x1a, 11}, {0x18, 11}
};

static const unsigned local_range = 6;

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
get_mcpe (word_t *mcpe, const image_t *original,
	  unsigned x0, unsigned y0, unsigned width, unsigned height,
	  const word_t *mcblock1, const word_t *mcblock2);
static real_t
mcpe_norm (const image_t *original, unsigned x0, unsigned y0, unsigned width,
	   unsigned height, const word_t *mcblock1, const word_t *mcblock2);
static real_t 
find_best_mv (real_t price, const image_t *original, const image_t *reference,
	      unsigned x0, unsigned y0, unsigned width, unsigned height,
	      real_t *bits, int *mx, int *my, const real_t *mc_norms,
	      const wfa_info_t *wi, const motion_t *mt);
static real_t
find_second_mv (real_t price, const image_t *original,
		const image_t *reference, const word_t *mcblock1,
		unsigned xr, unsigned yr, unsigned width, unsigned height,
		real_t *bits, int *mx, int *my, const wfa_info_t *wi,
		const motion_t *mt);

/*****************************************************************************

				public code
  
*****************************************************************************/

motion_t *
alloc_motion (const wfa_info_t *wi)
/*
 *  Motion structure constructor.
 *  Allocate memory for the motion structure and
 *  fill in default values specified by 'wi'.
 *
 *  Return value:
 *	pointer to the new option structure or NULL on error
 */
{
   int	     dx;			/* motion vector coordinate */
   unsigned  level;
   unsigned range_size = wi->half_pixel
			 ? square (wi->search_range)
			 : square (2 * wi->search_range);
   motion_t *mt        = Calloc (1, sizeof (motion_t));
   
   mt->original = NULL;
   mt->past     = NULL;
   mt->future   = NULL;
   mt->xbits 	= Calloc (2 * wi->search_range, sizeof (real_t));
   mt->ybits 	= Calloc (2 * wi->search_range, sizeof (real_t));

   for (dx = -wi->search_range; dx < (int) wi->search_range; dx++)
   {
      mt->xbits [dx + wi->search_range]
	 = mt->ybits [dx + wi->search_range]
	 = mv_code_table [dx + wi->search_range][1];
   }
   
   mt->mc_forward_norms = Calloc (MAXLEVEL, sizeof (real_t *));
   mt->mc_backward_norms = Calloc (MAXLEVEL, sizeof (real_t *));
   
   for (level = wi->p_min_level; level <= wi->p_max_level; level++)
   {
      mt->mc_forward_norms  [level] = Calloc (range_size, sizeof (real_t));
      mt->mc_backward_norms [level] = Calloc (range_size, sizeof (real_t));
   }

   return mt;
}

void
free_motion (motion_t *mt)
/*
 *  Motion struct destructor:
 *  Free memory of 'motion' struct.
 *
 *  No return value.
 *
 *  Side effects:
 *	structure 'motion' is discarded.
 */
{
   unsigned level;

   Free (mt->xbits);
   Free (mt->ybits);
   for (level = 0; level < MAXLEVEL; level++)
   {
      if (mt->mc_forward_norms [level])
	 Free (mt->mc_forward_norms [level]);
      if (mt->mc_backward_norms [level])
	 Free (mt->mc_backward_norms [level]);
   }
   Free (mt->mc_forward_norms);
   Free (mt->mc_backward_norms);
   Free (mt);
}

void
subtract_mc (image_t *image, const image_t *past, const image_t *future,
	     const wfa_t *wfa)
/*
 *  Subtract motion compensation from chrom channels of 'image'.
 *  Reference frames are given by 'past' and 'future'.
 *
 *  No return values.
 */
{
   unsigned  state, label;
   word_t   *mcblock1 = Calloc (size_of_level (wfa->wfainfo->p_max_level),
				sizeof (word_t));
   word_t   *mcblock2 = Calloc (size_of_level (wfa->wfainfo->p_max_level),
				sizeof (word_t));

   for (state = wfa->basis_states; state < wfa->states; state++)
      for (label = 0; label < MAXLABELS; label++)
	 if (wfa->mv_tree [state][label].type != NONE)
	 {
	    color_e  band;		/* current color band */
	    unsigned width, height;	/* size of mcblock */
	    unsigned offset;		/* remaining pixels in original */
	    
	    width  = width_of_level (wfa->level_of_state [state] - 1);
	    height = height_of_level (wfa->level_of_state [state] - 1);
	    offset = image->width - width;
	    
	    switch (wfa->mv_tree [state][label].type)
	    {
	       case FORWARD:
		  for (band  = first_band (image->color) + 1;
		       band <= last_band (image->color); band++)
		  {
		     unsigned  y;	/* row of block */
		     word_t   *mc1;	/* pixel in MC block 1 */
		     word_t   *orig;	/* pixel in original image */
		     
		     extract_mc_block (mcblock1, width, height,
				       past->pixels [band], past->width,
				       wfa->wfainfo->half_pixel,
				       wfa->x [state][label],
				       wfa->y [state][label],
				       (wfa->mv_tree [state][label].fx / 2)
				       * 2,
				       (wfa->mv_tree [state][label].fy / 2)
				       * 2);
		     mc1  = mcblock1;
		     orig = image->pixels [band] + wfa->x [state][label]
			    + wfa->y [state][label] * image->width;
		     
 		     for (y = height; y; y--)
		     {
			unsigned x;	/* column of block */
			
			for (x = width; x; x--)
			   *orig++ -= *mc1++;

			orig += offset;
		     }
		  }
		  break;
	       case BACKWARD:
		  for (band  = first_band (image->color) + 1;
		       band <= last_band (image->color); band++)
		  {
		     unsigned  y;	/* row of block */
		     word_t   *mc1;	/* pixel in MC block 1 */
		     word_t   *orig;	/* pixel in original image */
		     
		     extract_mc_block (mcblock1, width, height,
				       future->pixels [band], future->width,
				       wfa->wfainfo->half_pixel,
				       wfa->x [state][label],
				       wfa->y [state][label],
				       (wfa->mv_tree [state][label].bx / 2)
				       * 2,
				       (wfa->mv_tree [state][label].by / 2)
				       * 2);
		     mc1  = mcblock1;
		     orig = image->pixels [band] + wfa->x [state][label]
			    + wfa->y [state][label] * image->width;
		     
		     for (y = height; y; y--)
		     {
			unsigned x;	/* column of block */
			
			for (x = width; x; x--)
			   *orig++ -= *mc1++;

			orig += offset;
		     }
		  }
		  break;
	       case INTERPOLATED:
		  for (band  = first_band (image->color) + 1;
		       band <= last_band (image->color); band++)
		  {
		     unsigned  y;	/* row of block */
		     word_t   *mc1;	/* pixel in MC block 1 */
		     word_t   *mc2;	/* pixel in MC block 2 */
		     word_t   *orig;	/* pixel in original image */
		     
		     extract_mc_block (mcblock1, width, height,
				       past->pixels [band], past->width,
				       wfa->wfainfo->half_pixel,
				       wfa->x [state][label],
				       wfa->y [state][label],
				       (wfa->mv_tree[state][label].fx / 2)
				       * 2,
				       (wfa->mv_tree[state][label].fy / 2)
				       * 2);
		     extract_mc_block (mcblock2, width, height,
				       future->pixels [band], future->width,
				       wfa->wfainfo->half_pixel,
				       wfa->x [state][label],
				       wfa->y [state][label],
				       (wfa->mv_tree[state][label].bx / 2)
				       * 2,
				       (wfa->mv_tree[state][label].by / 2)
				       * 2);
		     mc1  = mcblock1;
		     mc2  = mcblock2;
		     orig = image->pixels [band] + wfa->x [state][label]
			    + wfa->y [state][label] * image->width;
		     
		     for (y = height; y; y--)
		     {
			unsigned x;	/* column of block */
			
			for (x = width; x; x--)
			   *orig++ -= (*mc1++ + *mc2++) / 2;

			orig += offset;
		     }
		  }
		  break;
	       default:
		  break;
	    }
	 }

   Free (mcblock1);
   Free (mcblock2);
}

void
find_P_frame_mc (word_t *mcpe, real_t price, range_t *range,
		 const wfa_info_t *wi, const motion_t *mt)
/*
 *  Determine best motion vector for P-frame.
 *
 *  No return value.
 *
 *  Side effects:
 *            range->mvt_bits  (# of mv-tree bits)
 *            range->mvxybits  (# of bits for vector components)
 *            mt->mcpe         (MCPE in scan-order)
 */
{
   unsigned  width   = width_of_level (range->level);
   unsigned  height  = height_of_level (range->level);
   word_t   *mcblock = Calloc (width * height, sizeof (word_t));
   
   range->mv_tree_bits = 1;
   range->mv.type      = FORWARD;

   /*
    *  Find best matching forward prediction
    */
   find_best_mv (price, mt->original, mt->past, range->x, range->y,
		 width, height, &range->mv_coord_bits, &range->mv.fx,
		 &range->mv.fy, mt->mc_forward_norms [range->level], wi, mt);

   /*
    *  Compute MCPE
    */
   extract_mc_block (mcblock, width, height, mt->past->pixels [GRAY],
		     mt->past->width, wi->half_pixel, range->x, range->y,
		     range->mv.fx, range->mv.fy);
   get_mcpe (mcpe, mt->original, range->x, range->y, width, height,
	     mcblock, NULL);

   Free (mcblock);
}

void
find_B_frame_mc (word_t *mcpe, real_t price, range_t *range,
		 const wfa_info_t *wi, const motion_t *mt)
/*
 *  Determines best motion compensation for B-frame.
 *  Steps:
 *         1)  find best forward motion vector
 *         2)  find best backward motion vector
 *         3)  try both motion vectors together (interpolation)
 *         4)  choose best mode (FORWARD, BACKWARD or INTERPOLATED)
 *  Bitcodes:
 *    FORWARD      000
 *    BACKWARD     001
 *    INTERPOLATED  01
 *  
 *  Return values:
 *            range->mvt_bits  (# of mv-tree bits)
 *            range->mvxybits  (# of bits for vector components)
 *            mt->mcpe         (MCPE in scan-order)
 */
{
   mc_type_e  mctype;			/* type of motion compensation */
   real_t     forward_costs;		/* costs of FORWARD mc */
   real_t     backward_costs;		/* costs of BACKWARD mc */
   real_t     interp_costs;		/* costs of INTERPOLATED mc */
   real_t     forward_bits;		/* bits for FORWARD mc */
   real_t     backward_bits;		/* bits for BACKWARD mc */
   real_t     interp_bits;		/* bits for INTERPOLATED mc */
   int	      fx,  fy;			/* coordinates FORWARD mc */
   int	      bx,  by;			/* coordinates BACKWARD mc */
   int	      ifx, ify;			/* coordinates forw. INTERPOLATED mc */
   int	      ibx, iby;			/* coordinates back. INTERPOLATED mc */
   unsigned   width    = width_of_level (range->level);
   unsigned   height   = height_of_level (range->level);
   word_t    *mcblock1 = Calloc (width * height, sizeof (word_t));
   word_t    *mcblock2 = Calloc (width * height, sizeof (word_t));
   
   /*
    *  Forward interpolation: use past frame as reference
    */
   forward_costs = find_best_mv (price, mt->original, mt->past,
				 range->x, range->y, width, height,
				 &forward_bits, &fx, &fy,
				 mt->mc_forward_norms [range->level], wi, mt)
		   + 3 * price; /* code 000 */

   /*
    *  Backward interpolation: use future frame as reference
    */
   backward_costs = find_best_mv (price, mt->original, mt->future,
				  range->x, range->y, width, height,
				  &backward_bits, &bx, &by,
				  mt->mc_backward_norms [range->level], wi, mt)
		    + 3 * price; /* code 001 */

   /*
    *  Bidirectional interpolation: use both past and future frame as reference
    */
   if (wi->cross_B_search) 
   {
      real_t icosts1;			/* costs interpolation alternative 1 */
      real_t icosts2;			/* costs interpolation alternative 2 */
      real_t ibackward_bits;		/* additional bits alternative 1 */
      real_t iforward_bits;		/* additional bits alternative 1 */
      
      /*
       *  Alternative 1: keep forward mv and vary backward mv locally
       */
      extract_mc_block (mcblock1, width, height, mt->past->pixels [GRAY],
			mt->past->width, wi->half_pixel,
			range->x, range->y, fx, fy);

      ibx = bx;				/* start with backward coordinates */
      iby = by;
      icosts1 = find_second_mv (price, mt->original, mt->future,
				mcblock1, range->x, range->y, width, height,
				&ibackward_bits, &ibx, &iby, wi, mt)
		+ (forward_bits + 2) * price; /* code 01 */

      /*
       *  Alternative 2: Keep backward mv and vary forward mv locally
       */
      extract_mc_block (mcblock1, width, height, mt->future->pixels [GRAY],
			mt->future->width, wi->half_pixel,
			range->x, range->y, bx, by);

      ifx = fx;
      ify = fy;
      icosts2 = find_second_mv (price, mt->original, mt->past,
				mcblock1, range->x, range->y, width, height,
				&iforward_bits, &ifx, &ify, wi, mt)
		+ (backward_bits + 2) * price; /* code 01 */
      
      /*
       *  Choose best alternative
       */
      if (icosts1 < icosts2)
      {
	 ifx 	      = fx;
	 ify 	      = fy;
	 interp_bits  = forward_bits + ibackward_bits;
	 interp_costs = icosts1;
      }
      else
      {
	 ibx 	      = bx;
	 iby 	      = by;
	 interp_bits  = iforward_bits + backward_bits;
	 interp_costs = icosts2;
      }
   }
   else					/* local exhaustive search */
   {
      /*
       *  Keep forward and backward mv due to time constraints
       */

      ifx = fx;
      ify = fy;
      ibx = bx;
      iby = by;
      interp_bits = forward_bits + backward_bits;

      extract_mc_block (mcblock1, width, height, mt->past->pixels [GRAY],
			mt->past->width, wi->half_pixel,
			range->x, range->y, fx, fy);
      extract_mc_block (mcblock2, width, height, mt->future->pixels [GRAY],
			mt->future->width, wi->half_pixel,
			range->x, range->y, bx, by);
      interp_costs = mcpe_norm (mt->original, range->x, range->y,
				width, height, mcblock1, mcblock2)
		     + (interp_bits + 2) * price; /* code 01 */
   }

   /*
    *  Choose alternative with smallest costs
    */
   if (forward_costs <= interp_costs)
   {
      if (forward_costs <= backward_costs)
	 mctype = FORWARD;
      else
	 mctype = BACKWARD;
   }
   else
   {
      if (backward_costs <= interp_costs)
	 mctype = BACKWARD;
      else
	 mctype = INTERPOLATED;
   }

   switch (mctype)
   {
      case FORWARD:
	 range->mv_tree_bits  = 3;
	 range->mv_coord_bits = forward_bits;
	 range->mv.type       = FORWARD;
	 range->mv.fx         = fx;
	 range->mv.fy         = fy;
	 extract_mc_block (mcblock1, width, height, mt->past->pixels [GRAY],
			   mt->past->width, wi->half_pixel,
			   range->x, range->y, range->mv.fx, range->mv.fy);
	 get_mcpe (mcpe, mt->original, range->x, range->y, width, height,
		   mcblock1, NULL);
	 break;
      case BACKWARD:
	 range->mv_tree_bits  = 3;
	 range->mv_coord_bits = backward_bits;
	 range->mv.type       = BACKWARD;
	 range->mv.bx         = bx;
	 range->mv.by         = by;
	 extract_mc_block (mcblock1, width, height, mt->future->pixels [GRAY],
			   mt->future->width, wi->half_pixel,
			   range->x, range->y, range->mv.bx, range->mv.by);
	 get_mcpe (mcpe, mt->original, range->x, range->y, width, height,
		   mcblock1, NULL);
	 break;
      case INTERPOLATED:
	 range->mv_tree_bits  = 2;
	 range->mv_coord_bits = interp_bits;
	 range->mv.type       = INTERPOLATED;
	 range->mv.fx         = ifx;
	 range->mv.fy         = ify;
	 range->mv.bx         = ibx;
	 range->mv.by         = iby;
	 extract_mc_block (mcblock1, width, height, mt->past->pixels [GRAY],
			   mt->past->width, wi->half_pixel,
			   range->x, range->y, range->mv.fx, range->mv.fy);
	 extract_mc_block (mcblock2, width, height, mt->future->pixels [GRAY],
			   mt->future->width, wi->half_pixel,
			   range->x, range->y, range->mv.bx, range->mv.by);
	 get_mcpe (mcpe, mt->original, range->x, range->y, width, height,
		   mcblock1, mcblock2);
	 break;
      default:
	 break;
   }

   Free (mcblock1);
   Free (mcblock2);
}

void
fill_norms_table (unsigned x0, unsigned y0, unsigned level,
		  const wfa_info_t *wi, motion_t *mt)
/*
 *  Compute norms of difference images for all possible displacements
 *  in 'mc_forward_norm' and 'mc_backward_norm'.
 *
 *  No return value.
 *
 *  Side effects:
 *	'mt->mc_backward_norms' are computed
 *	'mt->mc_forward_norms' are computed 
 */
{
   int	     mx, my;			/* coordinates of motion vector */
   unsigned  sr;			/* mv search range +-'sr' pixels */
   unsigned  index   = 0;		/* index of motion vector */
   unsigned  width   = width_of_level (level);
   unsigned  height  = height_of_level (level);
   word_t   *mcblock = Calloc (width * height, sizeof (word_t));

   sr = wi->half_pixel ? wi->search_range / 2 :  wi->search_range;
   
   for (my = -sr; my < (int) sr; my++)
      for (mx = -sr; mx < (int) sr; mx++, index++)
      {
	  if ((int) x0 + mx < 0 ||	/* block outside visible area */
	      x0 + mx + width > mt->original->width || 
	      (int) y0 + my < 0 ||
	      y0 + my + height > mt->original->height)
	  {
	     mt->mc_forward_norms [level][index]  = 0.0;
	     mt->mc_backward_norms [level][index] = 0.0;
	  }
	  else
	  {
	     extract_mc_block (mcblock, width, height, mt->past->pixels [GRAY],
			       mt->past->width, wi->half_pixel,
			       x0, y0, mx, my);
	     mt->mc_forward_norms [level][index]
		= mcpe_norm (mt->original, x0, y0, width, height,
			     mcblock, NULL);

	     if (mt->frame_type == B_FRAME)
	     {
		extract_mc_block (mcblock, width, height,
				  mt->future->pixels [GRAY],
				  mt->future->width, wi->half_pixel,
				  x0, y0, mx, my);
		mt->mc_backward_norms[level][index]
		   = mcpe_norm (mt->original, x0, y0, width, height,
				mcblock, NULL); 
	     }
	  }
       }

   Free (mcblock);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
get_mcpe (word_t *mcpe, const image_t *original, unsigned x0, unsigned y0,
	  unsigned width, unsigned height, const word_t *mcblock1,
	  const word_t *mcblock2)
/*
 *  Compute MCPE image 'original' - reference. The reference is either
 *  composed of 'mcblock1' or of ('mcblock1' + 'mcblock2') / 2 (if
 *  'mcblock2' != NULL).  Coordinates of original block are given by
 *  'x0', 'y0', 'width', and 'height'.
 *
 *  No return value.
 *
 *  Side effects:
 *	'mcpe []' is filled with the delta image
 */
{
   const word_t	*oblock;		/* pointer to original image */

   assert (mcpe);
   
   oblock = original->pixels [GRAY] + y0 * original->width + x0;

   if (mcblock2 != NULL)		/* interpolated prediction */
   {
      unsigned x, y;			/* current coordinates */
      
      for (y = height; y; y--) 
      {
	 for (x = width; x; x--)
	    *mcpe++ = *oblock++ - (*mcblock1++ + *mcblock2++) / 2;

	 oblock += original->width - width;
      }
   }
   else					/* forward or backward prediction */
   {
      unsigned x, y;			/* current coordinates */
      
      for (y = height; y; y--) 
      {
	 for (x = width; x; x--)
	    *mcpe++ = *oblock++ - *mcblock1++;
      
	 oblock += original->width - width;
      }
   }
}

static real_t
mcpe_norm (const image_t *original, unsigned x0, unsigned y0, unsigned width,
	   unsigned height, const word_t *mcblock1, const word_t *mcblock2)
/*
 *  Compute norm of motion compensation prediction error.
 *  Coordinates of 'original' block are given by ('x0', 'y0')
 *  and 'width', 'height'.
 *  Reference blocks are stored in 'mcimage1' and 'mcimage2'.
 *
 *  Return value:
 *	square of norm of difference image
 */
{
   unsigned  n;
   real_t    norm = 0;
   word_t   *mcpe = Calloc (width * height, sizeof (word_t));
   word_t   *ptr  = mcpe;
   
   get_mcpe (mcpe, original, x0, y0, width, height, mcblock1, mcblock2);

   for (n = height * width; n; n--, ptr++) 
      norm += square (*ptr / 16);
   
   Free (mcpe);
   
   return norm;
}

static real_t 
find_best_mv (real_t price, const image_t *original, const image_t *reference,
	      unsigned x0, unsigned y0, unsigned width, unsigned height,
	      real_t *bits, int *mx, int *my, const real_t *mc_norms,
	      const wfa_info_t *wi, const motion_t *mt)
/*
 *  Find best matching motion vector in image 'reference' to predict
 *  the block ('x0', 'y0') of size 'width'x'height in image 'original'.
 *
 *  Return values:
 *	prediction costs
 *
 *  Side effects:
 *	'mx', 'my'		coordinates of motion vector
 *	'bits'			number of bits to encode mv
 */
{
   unsigned sr;				/* mv search range +/- 'sr' pixels */
   unsigned index;			/* index of motion vector */
   int 	    x, y;			/* coordinates of motion vector */
   real_t   costs;			/* costs arising if mv is chosen */
   real_t   mincosts = MAXCOSTS;	/* best costs so far  */
   unsigned bitshift;			/* half_pixel coordinates multiplier */
   
   *mx = *my = 0;

   /*
    *  Find best fitting motion vector:
    *  Use exhaustive search in the interval x,y +- sr (no halfpixel accuracy)
    *					  or x,y +- sr/2  (halfpixel accuracy)
    */
   sr 	    = wi->half_pixel ? wi->search_range / 2 :  wi->search_range;
   bitshift = (wi->half_pixel ? 2 : 1);	/* bit0 reserved for halfpixel pred. */
   
   for (index = 0, y = -sr; y < (int) sr; y++)
      for (x = -sr; x < (int) sr; x++, index++)
	 if ((int) x0 + x >= 0 && (int) y0 + y >= 0 &&	
	     x0 + x + width  <= original->width && 
	     y0 + y + height <= original->height)
	 {
	    /*
	     *  Block is inside visible area.
	     *  Compare current costs with 'mincosts'
	     */
	    costs = mc_norms [index]
		    + (mt->xbits [(x + sr) * bitshift]
		       + mt->ybits [(y + sr) * bitshift]) * price;

	     if (costs < mincosts)
	     {
		mincosts = costs;
		*mx      = x * bitshift;
		*my      = y * bitshift;
	     }
	 }

   /*
    *  Halfpixel prediction:
    *  Compare all nine combinations (-1, y), (0, y), (+1, y) for y = -1,0,+1
    */
   if (wi->half_pixel)
   {
      int	rx, ry;			/* halfpixel refinement */
      unsigned	bestrx, bestry;		/* coordinates of best mv */
      word_t   *mcblock = Calloc (width * height, sizeof (word_t));
      
      bestrx = bestry = 0;
      for (rx = -1; rx <= 1; rx++)
	 for (ry = -1; ry <= 1; ry++)
	 {
	    /*
	     *  Check if the new motion vector is in allowed area
	     */
	    if (rx == 0 && ry == 0)	/* already tested */
	       continue;
	    if ((int) x0 + (*mx / 2) + rx < 0 || /* outside visible area */
		x0 + (*mx / 2) + rx + width > original->width ||
		(int) y0 + (*my / 2) + ry < 0 || 
		y0 + (*my / 2) + ry + height > original->height)
	       continue;
	    if (*mx + rx < (int) -sr || *mx + rx >= (int) sr ||
		*my + ry < (int) -sr || *my + ry >= (int) sr) 
	       continue;		/* out of bounds */

	    /*
	     *  Compute costs of new motion compensation
	     */
	    extract_mc_block (mcblock, width, height,
			      reference->pixels [GRAY],
			      reference->width, wi->half_pixel,
			      x0, y0, *mx + rx, *my + ry);
	    costs = mcpe_norm (mt->original, x0, y0, width, height, mcblock,
			       NULL)
		    + (mt->xbits [*mx + rx + sr * bitshift]
		       + mt->ybits [*my + ry + sr * bitshift]) * price;
	    if (costs < mincosts)
	    {
	       bestrx   = rx;
	       bestry   = ry;
	       mincosts = costs;
	    }
	 }

      *mx += bestrx;
      *my += bestry;

      Free (mcblock);
   } /* halfpixel */
	     
   *bits = mt->xbits [*mx + sr * bitshift] + mt->ybits [*my + sr * bitshift];

   return mincosts;
}

static real_t
find_second_mv (real_t price, const image_t *original,
		const image_t *reference, const word_t *mcblock1,
		unsigned xr, unsigned yr, unsigned width, unsigned height,
		real_t *bits, int *mx, int *my, const wfa_info_t *wi,
		const motion_t *mt)
/*
 *  Search local area (*mx,*my) for best additional mv.
 *  Overwrite mt->tmpblock.
 *  TODO check sr = search_range
 *
 *  Return values:
 *	prediction costs
 *
 *  Side effects:
 *	'mx','my'	coordinates of mv
 *      'bits'		number of bits to encode mv
 */
{
   real_t    mincosts = MAXCOSTS;	/* best costs so far  */
   unsigned  sr;			/* MV search range +/- 'sr' pixels */
   int       x, y;			/* coordinates of motion vector */
   int       y0, y1, x0, x1;		/* start/end coord. of search range */
   unsigned  bitshift;			/* half_pixel coordinates multiplier */
   word_t   *mcblock2 = Calloc (width * height, sizeof (word_t));

   sr = wi->search_range;

   y0 = max ((int) -sr, *my - (int) local_range);
   y1 = min ((int) sr, *my + (int) local_range);
   x0 = max ((int) -sr, *mx - (int) local_range);
   x1 = min ((int) sr, *mx + (int) local_range);

   *mx = *my = 0;

   bitshift = (wi->half_pixel ? 2 : 1);	/* bit0 reserved for halfpixel pred. */

   
   for (y = y0; y < y1; y++)
      for (x = x0; x < x1; x++)
      {
	 real_t costs;			/* costs arising if mv is chosen */
	 
	 /*
	  *  Test each mv ('x', 'y') in the given search range:
	  *  Get the new motion compensation image from 'reference' and compute
	  *  the norm of the motion compensation prediction error
	  *  'original' - 0.5 * ('firstmc' + 'reference')
	  */
	 if ((int) (xr * bitshift) + x < 0 ||	/* outside visible area */
	     xr * bitshift + x > (original->width - width) * bitshift ||
	     (int) (yr * bitshift) + y < 0 ||
	     yr * bitshift + y > (original->height - height) * bitshift)
	    continue;
	 
	 extract_mc_block (mcblock2, width, height,
			   reference->pixels [GRAY], reference->width,
			   wi->half_pixel, x0, y0, x, y);

	 costs  = mcpe_norm (mt->original, x0, y0, width, height,
			     mcblock1, mcblock2)
		  + (mt->xbits [x + sr] + mt->ybits [y + sr]) * price;
	 
	 if (costs < mincosts)
	 {
	    mincosts = costs;
	    *mx      = x;
	    *my      = y;
	 }
      }

   *bits = mt->xbits [*mx + sr] + mt->ybits [*my + sr];

   Free (mcblock2);

   return mincosts;
}
