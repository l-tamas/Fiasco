/*
 *  motion.c:		Motion compensation code	
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

#include "config.h"

#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */

#include "types.h"
#include "macros.h"
#include "error.h"

#include "image.h"
#include "misc.h"
#include "motion.h"

/*****************************************************************************

				public code
  
*****************************************************************************/

void
restore_mc (int enlarge_factor, image_t *image, const image_t *past,
	    const image_t *future, const wfa_t *wfa)
/*
 *  Restore motion compensated prediction of 'image' represented by 'wfa'.
 *  If 'enlarge_factor' != 0 then enlarge image by given amount.
 *  Reference frames are given by 'past' and 'future'.
 *
 *  No return values.
 */
{
   unsigned  state, label;
   unsigned  root_state;
   word_t   *mcblock1, *mcblock2;	/* MC blocks */

#define FX(v) ((image->format == FORMAT_4_2_0) && band != Y ? ((v) / 2) : v)
   
   mcblock1 = Calloc (size_of_level (max ((int) wfa->wfainfo->p_max_level
					  + 2 * enlarge_factor, 0)),
		      sizeof (word_t));
   mcblock2 = Calloc (size_of_level (max ((int) wfa->wfainfo->p_max_level
					  + 2 * enlarge_factor, 0)),
		      sizeof (word_t));

   if (!image->color)
      root_state = wfa->root_state;
   else
      root_state  = wfa->tree [wfa->tree [wfa->root_state][0]][0];
   
   for (state = wfa->basis_states; state <= root_state; state++)
      for (label = 0; label < MAXLABELS; label++)
	 if (wfa->mv_tree[state][label].type != NONE)
	 {
	    color_e band;
	    unsigned level  = wfa->level_of_state [state] - 1;
	    unsigned width  = width_of_level (level);
	    unsigned height = height_of_level (level);
	    unsigned offset = image->width - width;
	    
	    switch (wfa->mv_tree [state][label].type)
	    {
	       case FORWARD:
		  for (band  = first_band (image->color);
		       band <= last_band (image->color); band++)
		  {
		     extract_mc_block (mcblock1, FX (width), FX (height),
				       past->pixels [band], FX (past->width),
				       wfa->wfainfo->half_pixel,
				       FX (wfa->x [state][label]),
				       FX (wfa->y [state][label]),
				       FX (wfa->mv_tree [state][label].fx),
				       FX (wfa->mv_tree [state][label].fy));
		     {
			word_t   *mc1;	/* current pixel in MC block */
			word_t 	 *orig;	/* current pixel in original image */
			unsigned  x, y;	/* pixel coordinates */
			
			mc1  = mcblock1;
			orig = (word_t *) image->pixels [band]
			       + FX (wfa->x[state][label])
			       + FX (wfa->y[state][label]) * FX (image->width);
		     
			for (y = FX (height); y; y--)
			{
			   for (x = FX (width); x; x--)
			      *orig++ += *mc1++;

			   orig += FX (offset);
			}
		     }
		  }
		  break;
	       case BACKWARD:
		  for (band  = first_band (image->color);
		       band <= last_band (image->color); band++)
		  {
		     extract_mc_block (mcblock1, FX (width), FX (height),
				       future->pixels [band],
				       FX (future->width),
				       wfa->wfainfo->half_pixel,
				       FX (wfa->x [state][label]),
				       FX (wfa->y [state][label]),
				       FX (wfa->mv_tree [state][label].bx),
				       FX (wfa->mv_tree [state][label].by));
		     {
			word_t   *mc1;	/* current pixel in MC block 1 */
			word_t   *orig;	/* current pixel in original image */
			unsigned  x, y;	/* pixel coordinates */
			
			mc1  = mcblock1;
			orig = (word_t *) image->pixels [band]
			       + FX (wfa->x[state][label])
			       + FX (wfa->y[state][label]) * FX (image->width);
		     
			for (y = FX (height); y; y--)
			{
			   for (x = FX (width); x; x--)
			      *orig++ += *mc1++;

			   orig += FX (offset);
			}
		     }
		  }
		  break;
	       case INTERPOLATED:
		  for (band  = first_band (image->color);
		       band <= last_band (image->color); band++)
		  {
		     extract_mc_block (mcblock1, FX (width), FX (height),
				       past->pixels [band], FX (past->width),
				       wfa->wfainfo->half_pixel,
				       FX (wfa->x[state][label]),
				       FX (wfa->y[state][label]),
				       FX (wfa->mv_tree[state][label].fx),
				       FX (wfa->mv_tree[state][label].fy));
		     extract_mc_block (mcblock2, FX (width), FX (height),
				       future->pixels [band],
				       FX (future->width),
				       wfa->wfainfo->half_pixel,
				       FX (wfa->x[state][label]),
				       FX (wfa->y[state][label]),
				       FX (wfa->mv_tree[state][label].bx),
				       FX (wfa->mv_tree[state][label].by));
		     {
			word_t   *mc1;	/* current pixel in MC block 1 */
			word_t   *mc2;	/* current pixel in MC block 1 */
			word_t   *orig;	/* current pixel in original image */
			unsigned  x, y;	/* pixel coordinates */
			
			mc1  = mcblock1;
			mc2  = mcblock2;
			orig = (word_t *) image->pixels [band]
			       + FX (wfa->x[state][label])
			       + FX (wfa->y[state][label]) * FX (image->width);
			
			for (y = FX (height); y; y--)
			{
			   for (x = FX (width); x; x--)
#ifdef HAVE_SIGNED_SHIFT
			      *orig++ += (*mc1++ + *mc2++) >> 1;
#else /* not HAVE_SIGNED_SHIFT */
			   *orig++ += (*mc1++ + *mc2++) / 2;
#endif /* not HAVE_SIGNED_SHIFT */

			   orig += FX (offset);
			}
		     }
		  }
		  break;
	       default:
		  break;
	    }
	 }

   if (image->color)
   {
      unsigned	  n;
      word_t	 *ptr;
      static int *clipping = NULL;
      unsigned	  shift    = image->format == FORMAT_4_2_0 ? 2 : 0;

      if (!clipping)			/* initialize clipping table */
      {
	 int i;
	    
	 clipping = Calloc (256 * 3, sizeof (int));
	 for (i = -128; i < 128; i++)
	    clipping [256 + i + 128] = i;
	 for (i = 0; i < 256; i++)
	    clipping [i] = clipping [256];
	 for (i = 512; i < 512 + 256; i++)
	    clipping [i] = clipping [511];
	 clipping += 256 + 128;
      }
	 
      ptr = image->pixels [Cb];
      for (n = (image->width * image->height) >> shift; n; n--, ptr++)
#ifdef HAVE_SIGNED_SHIFT
	 *ptr = clipping [*ptr >> 4] << 4;
#else /* not HAVE_SIGNED_SHIFT */
	 *ptr = clipping [*ptr / 16] * 16;
#endif /* not HAVE_SIGNED_SHIFT */
      ptr = image->pixels [Cr];
      for (n = (image->width * image->height) >> shift; n; n--, ptr++)
#ifdef HAVE_SIGNED_SHIFT
	*ptr = clipping [*ptr >> 4] << 4;
#else /* not HAVE_SIGNED_SHIFT */
        *ptr = clipping [*ptr / 16] * 16;
#endif /* not HAVE_SIGNED_SHIFT */
   }
   
   Free (mcblock1);
   Free (mcblock2);
}

void
extract_mc_block (word_t *mcblock, unsigned width, unsigned height,
		  const word_t *reference, unsigned ref_width,
		  bool_t half_pixel, unsigned xo, unsigned yo,
		  unsigned mx, unsigned my)
/*
 *  Extract motion compensation image 'mcblock' of size 'width'x'height'
 *  from 'reference' image (width is given by 'ref_width').
 *  Coordinates of reference block are given by ('xo' + 'mx', 'yo' + 'my').
 *  Use 'half_pixel' precision if specified.
 *
 *  No return value.
 *
 *  Side effects:
 *	'mcblock[]'	MCPE block is filled with reference pixels 
 */
{
   if (!half_pixel)			/* Fullpixel precision */
   {
      const word_t *rblock;		/* pointer to reference image */
      unsigned	    y;			/* current row */
      
      rblock  = reference + (yo + my) * ref_width + (xo + mx);
      for (y = height; y; y--) 
      {
	 memcpy (mcblock, rblock, width * sizeof (word_t));

	 mcblock += width;
	 rblock  += ref_width;
      }
   }
   else					/* Halfpixel precision */
   {
      unsigned	    x, y;		/* current coordinates */
      unsigned	    offset;		/* remaining pixels in row */
      const word_t *rblock;		/* pointer to reference image */
      const word_t *ryblock;		/* pointer to next line */
      const word_t *rxblock;		/* pointer to next column */
      const word_t *rxyblock;		/* pointer to next column & row */
   
      rblock   = reference + (yo + my / 2) * ref_width + (xo + mx / 2);
      ryblock  = rblock + ref_width;	/* pixel in next row */
      rxblock  = rblock + 1;		/* pixel in next column */
      rxyblock = ryblock + 1;		/* pixel in next row & column */
      offset   = ref_width - width;
      
      if ((mx & 1) == 0)
      {
	 if ((my & 1) == 0)		/* Don't use halfpixel refinement */
	    for (y = height; y; y--) 
	    {
	       memcpy (mcblock, rblock, width * sizeof (word_t));
	       
	       mcblock += width;
	       rblock  += ref_width;
	    }
	 else				/* Halfpixel in y direction */
	    for (y = height; y; y--) 
	    {
	       for (x = width; x; x--)
#ifdef HAVE_SIGNED_SHIFT
		  *mcblock++ = (*rblock++ + *ryblock++) >> 1;
#else /* not HAVE_SIGNED_SHIFT */
		  *mcblock++ = (*rblock++ + *ryblock++) / 2;
#endif /* not HAVE_SIGNED_SHIFT */

	       rblock  += offset;
	       ryblock += offset;
	    }
      }
      else
      {
	 if ((my & 1) == 0)		/* Halfpixel in x direction */
	    for (y = height; y; y--) 
	    {
	       for (x = width; x; x--)
#ifdef HAVE_SIGNED_SHIFT
		  *mcblock++ = (*rblock++ + *rxblock++) >> 1;
#else /* not HAVE_SIGNED_SHIFT */
		  *mcblock++ = (*rblock++ + *rxblock++) / 2;
#endif /* not HAVE_SIGNED_SHIFT */

	       rblock  += offset;
	       rxblock += offset;
	    }
	 else				/* Halfpixel in xy direction */
	    for (y = height; y; y--) 
	    {
	       for (x = width; x; x--)
#ifdef HAVE_SIGNED_SHIFT
		  *mcblock++ = (*rblock++ + *rxblock++
				+ *ryblock++ + *rxyblock++) >> 2;
#else /* not HAVE_SIGNED_SHIFT */
		  *mcblock++ = (*rblock++ + *rxblock++
				+ *ryblock++ + *rxyblock++) / 4;
#endif /* not HAVE_SIGNED_SHIFT */
	       rblock   += offset;
	       ryblock  += offset;
	       rxblock  += offset;
	       rxyblock += offset;
	    }
      }
   }
}
