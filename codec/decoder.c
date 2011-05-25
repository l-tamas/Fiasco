/*
 *  decode.c:		Decoding of an image represented by a WFA
 *
 *  Written by:		Ullrich Hafner
 *			Michael Unger
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */
 
/*
 *  $Date: 2000/10/22 10:44:48 $
 *  $Author: hafner $
 *  $Revision: 5.3 $
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

#include "wfa.h"
#include "image.h"
#include "misc.h"
#include "motion.h"
#include "read.h"
#include "wfalib.h"
#include "decoder.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
compute_state_images (unsigned frame_level, word_t **simg,
		      const u_word_t *offset, const wfa_t *wfa);
static void
free_state_images (unsigned max_level, bool_t color, word_t **state_image,
		   u_word_t *offset, const unsigned *root_state,
		   unsigned range_state, format_e format, const wfa_t *wfa);
static void
alloc_state_images (word_t ***images, u_word_t **offsets, const image_t *frame,
		    const unsigned *root_state, unsigned range_state,
		    unsigned max_level, format_e format, const wfa_t *wfa);
static void
compute_actual_size (unsigned luminance_root,
		     unsigned *width, unsigned *height, const wfa_t *wfa);
static void
enlarge_image (int enlarge_factor, format_e format, unsigned y_root,
	       wfa_t *wfa);
static word_t *
duplicate_state_image (const word_t *domain, unsigned offset, unsigned level);

/*****************************************************************************

				public code
  
*****************************************************************************/

video_t *
alloc_video (bool_t store_wfa)
/*
 *  Video struct constructor:
 *  Initialize video structure and allocate memory for current, past
 *  and future WFA if flag 'store_wfa' is TRUE.
 *
 *  Return value:
 *	pointer to the new video structure
 */
{
   video_t *video = Calloc (1, sizeof (video_t));
   
   video->future_display = -1;
   video->display        = 0;

   video->future = video->sfuture = video->past
		 = video->frame   = video->sframe = NULL;

   if (store_wfa)
   {
      video->wfa        = alloc_wfa (NO);
      video->wfa_past   = alloc_wfa (NO);
      video->wfa_future = alloc_wfa (NO);
   }
   else
      video->wfa = video->wfa_past = video->wfa_future = NULL;

   return video;
}

void
free_video (video_t *video)
/*
 *  Video struct destructor:
 *  Free memory of given 'video' struct.
 *
 *  No return value.
 *
 *  Side effects:
 *	'video' struct is discarded.
 */
{
   if (video->past)
      free_image (video->past);
   if (video->future)
      free_image (video->future);
   if (video->sfuture)
      free_image (video->sfuture);
   if (video->frame)
      free_image (video->frame);
   if (video->sframe)
      free_image (video->sframe);
   if (video->wfa)
      free_wfa (video->wfa);
   if (video->wfa_past)
      free_wfa (video->wfa_past);
   if (video->wfa_future)
      free_wfa (video->wfa_future);

   Free (video);
}

image_t *
get_next_frame (bool_t store_wfa, int enlarge_factor,
		int smoothing, const char *reference_frame,
		format_e format, video_t *video, dectimer_t *timer,
		wfa_t *orig_wfa, bitfile_t *input)
/*
 *  Get next frame of the WFA 'video' from stream 'input'.
 *  'orig_wfa' is the constant part of the WFA used by all frames.
 *  Depending on values of 'enlarge_factor' and 'smoothing' enlarge and
 *  smooth image, respectively. 
 *  If 'store_wfa' is TRUE, then store WFA structure of reference frames
 *  (used by analysis tool xwfa).
 *  If 'reference_frame' is not NULL, then load image 'reference_frame'
 *  from disk.
 *  'format' gives the color format to be used (either 4:2:0 or 4:4:4).
 *  If 'timer' is not NULL, then accumulate running time statistics. 
 *
 *  Return value:
 *	pointer to decoded frame
 *
 *  Side effects:
 *	'video' and 'timer' struct are modified.
 */
{
   image_t *frame 			  = NULL; /* current frame */
   image_t *sframe 			  = NULL; /* current smoothed frame */
   bool_t   current_frame_is_future_frame = NO;

   if (video->future_display == video->display)	 
   {
      /*
       *  Future frame is already computed since it has been used
       *  as reference frame. So just return the stored frame.
       */
      if (video->frame) /* discard current frame */
	 free_image (video->frame);
      video->frame  = video->future;
      video->future = NULL;

      if (video->sframe) /* discard current (smoothed) frame */
	 free_image (video->sframe);
      video->sframe  = video->sfuture;
      video->sfuture = NULL;

      if (store_wfa)
	 copy_wfa (video->wfa, video->wfa_future);

      video->display++;

      if (!store_wfa)
	 video->wfa = NULL;
   }
   else
   {
      do				/* compute next frame(s) */
      {
	 unsigned      frame_number;	/* current frame number */
	 clock_t       ptimer;
	 unsigned int  stop_timer [3];
	 wfa_t	      *tmp_wfa = NULL;
	 
	 if (!store_wfa)
	    video->wfa = orig_wfa;
	 else
	 {
	    tmp_wfa = alloc_wfa (NO);
	    copy_wfa (tmp_wfa, video->wfa);
	    copy_wfa (video->wfa, orig_wfa);
	 }
   
	 /*
	  *  First step: read WFA from disk
	  */
	 prg_timer (&ptimer, START);
	 frame_number = read_next_wfa (video->wfa, input);
	 stop_timer [0] = prg_timer (&ptimer, STOP);
	 if (timer)
	 {
	    timer->input [video->wfa->frame_type] += stop_timer [0];
	    timer->frames [video->wfa->frame_type]++;
	 }
      
	 /*
	  *  Read reference frame from disk if required
	  *  (i.e., 1st frame is of type B or P)
	  */
	 if (video->display == 0 && video->wfa->frame_type != I_FRAME)
	 {
	    if (!reference_frame)
	       error ("First frame is %c-frame but no "
		      "reference frame is given.",
		      video->wfa->frame_type == B_FRAME ? 'B' : 'P');

	    video->frame  = read_image (reference_frame);
	    video->sframe = NULL;
	 }
   
	 /*
	  *  Depending on current frame type update past and future frames
	  */
	 if (video->wfa->frame_type == I_FRAME)
	 {
	    if (video->past)		/* discard past frame */
	       free_image (video->past);
	    video->past = NULL;
	    if (video->future)		/* discard future frame */
	       free_image (video->future);
	    video->future = NULL;
	    if (video->sfuture)		/* discard (smoothed) future frame */
	       free_image (video->sfuture);
	    video->sfuture = NULL;
	    if (video->frame)		/* discard current frame */
	       free_image (video->frame);
	    video->frame = NULL;
	    if (video->sframe)		/* discard current (smoothed) frame */
	       free_image (video->sframe);
	    video->sframe = NULL;
	 }
	 else if (video->wfa->frame_type == P_FRAME)
	 {
	    if (video->past)		/* discard past frame */
	       free_image (video->past);
	    video->past = video->frame;	/* past <- current frame */
	    video->frame = NULL;
	    if (video->sframe)		/* discard current (smoothed) frame */
	       free_image (video->sframe);
	    video->sframe = NULL;
	    if (store_wfa)
	       copy_wfa (video->wfa_past, tmp_wfa);
	    if (video->future)		/* discard future frame */
	       free_image (video->future);
	    video->future = NULL;
	    if (video->sfuture)		/* discard (smoothed) future frame */
	       free_image (video->sfuture);
	    video->sfuture = NULL;
	 }
	 else				/* B_FRAME */
	 {
	    if (current_frame_is_future_frame)
	    {
	       if (video->future)	/* discard future frame */
		  free_image (video->future);
	       video->future = frame;	/* future <- current frame */
	       if (video->sfuture)	/* discard (smoothed) future frame */
		  free_image (video->sfuture);
	       video->sfuture = sframe;	/* future <- current (smoothed) */
	       if (store_wfa)
		  copy_wfa (video->wfa_future, tmp_wfa);
	       if (video->frame)	/* discard current frame */
		  free_image (video->frame);
	       video->frame = NULL;
	       if (video->sframe)	/* discard current (smoothed) frame */
		  free_image (video->sframe);
	       video->sframe = NULL;
	       frame  = NULL;
	       sframe = NULL;
	    }
	    else
	    {
	       if (video->wfa->wfainfo->B_as_past_ref == YES)
	       {
		  if (video->past)	/* discard past frame */
		     free_image (video->past);
		  video->past  = video->frame; /* past <- current frame */
		  video->frame = NULL;
		  if (video->sframe)	/* discard current (smoothed) frame */
		     free_image (video->sframe);
		  video->sframe = NULL;
		  if (store_wfa)
		     copy_wfa (video->wfa_past, tmp_wfa);
	       }
	       else
	       {
		  if (video->frame)	/* discard current */
		     free_image (video->frame);
		  video->frame = NULL;
		  if (video->sframe)	/* discard current (smoothed) frame */
		     free_image (video->sframe);
		  video->sframe = NULL;
	       }
	    }
	 }
	 if (tmp_wfa)
	    free_wfa (tmp_wfa);
	 
	 current_frame_is_future_frame = NO;
	 /*
	  *  Second step: decode image
	  *  Optionally enlarge image if specified by option 'enlarge_factor'.
	  */
	 {
	    unsigned orig_width, orig_height;

	    stop_timer [0] = stop_timer [1] = stop_timer [2] = 0;
	 
	    enlarge_image (enlarge_factor, format,
			   (video->wfa->wfainfo->color
			    && format == FORMAT_4_2_0)
			   ? video->wfa->tree [video->wfa->tree [video->wfa->root_state][0]][0] : -1, video->wfa);

	    if (enlarge_factor > 0)
	    {
	       orig_width  = video->wfa->wfainfo->width  << enlarge_factor;
	       orig_height = video->wfa->wfainfo->height << enlarge_factor; 
	    }
	    else
	    { 
	       orig_width  = video->wfa->wfainfo->width  >> - enlarge_factor;
	       orig_height = video->wfa->wfainfo->height >> - enlarge_factor;
	       if (orig_width & 1)
		  orig_width++;
	       if (orig_height & 1)
		  orig_height++;
	    }
	 
	    frame = decode_image (orig_width, orig_height, format,
				  timer != NULL ? stop_timer : NULL,
				  video->wfa);
	    if (timer)
	    {
	       timer->preprocessing [video->wfa->frame_type] += stop_timer [0];
	       timer->decoder [video->wfa->frame_type]       += stop_timer [1];
	       timer->cleanup [video->wfa->frame_type]       += stop_timer [2];
	    }
	 }

	 /*
	  *  Third step: restore motion compensation
	  */
	 if (video->wfa->frame_type != I_FRAME)
	 {
	    prg_timer (&ptimer, START);
	    restore_mc (enlarge_factor, frame, video->past, video->future,
			video->wfa);
	    stop_timer [0] = prg_timer (&ptimer, STOP);
	    if (timer)
	       timer->motion [video->wfa->frame_type] += stop_timer [0];
	 }

	 /*
	  *  Fourth step: smooth image along partitioning borders
	  */
	 prg_timer (&ptimer, START);
	 if (smoothing < 0)	/* smoothing not changed by user */
	    smoothing = video->wfa->wfainfo->smoothing;
	 if (smoothing > 0 && smoothing <= 100)
	 {
	    sframe = clone_image (frame);
	    smooth_image (smoothing, video->wfa, sframe);
	 }
	 else
	    sframe = NULL;
	 
	 stop_timer [0] = prg_timer (&ptimer, STOP);
	 if (timer)
	    timer->smooth [video->wfa->frame_type] += stop_timer [0];

	 if (frame_number == video->display)
	 {
	    video->display++;
	    video->frame  = frame;
	    video->sframe = sframe;
	    frame         = NULL;
	    sframe        = NULL;
	 }
	 else if (frame_number > video->display)
	 {
	    video->future_display 	  = frame_number;
	    current_frame_is_future_frame = YES;
	 }
      
	 if (!store_wfa)
	    remove_states (video->wfa->basis_states, video->wfa);
      } while (!video->frame);

      if (!store_wfa)
	 video->wfa = NULL;
   }
   
   return video->sframe ? video->sframe : video->frame;
}

image_t *
decode_image (unsigned orig_width, unsigned orig_height, format_e format,
	      unsigned *dec_timer, const wfa_t *wfa)
/*
 *  Compute image which is represented by the given 'wfa'.
 *  'orig_width'x'orig_height' gives the resolution of the image at
 *  coding time. Use 4:2:0 subsampling or 4:4:4 'format' for color images.
 *  If 'dec_timer' is given, accumulate running time statistics. 
 *  
 *  Return value:
 *	pointer to decoded image
 *
 *  Side effects:
 *	'*dectimer' is changed if 'dectimer' != NULL.
 */
{
   unsigned   root_state [3];		/* root of bintree for each band */
   unsigned   width, height;		/* computed image size */
   image_t   *frame;			/* regenerated frame */
   word_t   **images;			/* pointer to array of pointers
					   to state images */
   u_word_t  *offsets;			/* pointer to array of state image
					   offsets */
   unsigned   max_level;		/* max. level of state with approx. */
   unsigned   state;
   clock_t    ptimer;

   prg_timer (&ptimer, START);

   /*
    *  Compute root of bintree for each color band
    */
   if (wfa->wfainfo->color)
   {
      root_state [Y]  = wfa->tree [wfa->tree [wfa->root_state][0]][0];
      root_state [Cb] = wfa->tree [wfa->tree [wfa->root_state][0]][1];
      root_state [Cr] = wfa->tree [wfa->tree [wfa->root_state][1]][0];
   }
   else
      root_state [GRAY] = wfa->root_state;

   /*
    *  Compute maximum level of a linear combination
    */
   for (max_level = 0, state = wfa->basis_states; state < wfa->states; state++)
      if (isedge (wfa->into [state][0][0]) || isedge (wfa->into [state][1][0]))
	 max_level = max (max_level, wfa->level_of_state [state]);
   

   /*
    *  Allocate frame buffer for decoded image
    */
   compute_actual_size (format == FORMAT_4_2_0 ? root_state [Y] : MAXSTATES,
			&width, &height, wfa);
   width  = max (width, orig_width);
   height = max (height, orig_height);
   frame = alloc_image (width, height, wfa->wfainfo->color, format);
   
   /*
    *  Allocate buffers for intermediate state images
    */
   if (wfa->wfainfo->color)
   {
      wfa->level_of_state [wfa->root_state]               = 128;
      wfa->level_of_state [wfa->tree[wfa->root_state][0]] = 128;
      wfa->level_of_state [wfa->tree[wfa->root_state][1]] = 128;
   }
   alloc_state_images (&images, &offsets, frame, root_state, 0, max_level, 
		       format, wfa);

   if (dec_timer)
      dec_timer [0] += prg_timer (&ptimer, STOP);

   /*
    *  Decode all state images, forming the complete image.
    */
   prg_timer (&ptimer, START);
   compute_state_images (max_level, images, offsets, wfa);
   if (dec_timer)
      dec_timer [1] += prg_timer (&ptimer, STOP);

   /*
    *  Cleanup buffers used for intermediate state images
    */
   prg_timer (&ptimer, START);
   free_state_images (max_level, frame->color, images, offsets, root_state, 0,
		      format, wfa);
   
   /*
    *  Crop decoded image if the image size differs.
    */
   if (orig_width != width || orig_height != height)
   {
      frame->height = orig_height;	
      frame->width  = orig_width;	
      if (orig_width != width)		
      {
	 color_e   band;		/* current color band */
	 word_t	  *src, *dst;		/* source and destination pointers */
	 unsigned  y;			/* current row */
	 
	 for (band  = first_band (frame->color);
	      band <= last_band (frame->color); band++)
	 {
	    src = dst = frame->pixels [band];
	    for (y = orig_height; y; y--)
	    {
	       memmove (dst, src, orig_width * sizeof (word_t));
	       dst += orig_width;
	       src += width;
	    }
	    if (format == FORMAT_4_2_0 && band == Y)
	    {
	       orig_width  >>= 1;
	       orig_height >>= 1;
	       width       >>= 1;
	    }
	 }
      }
   }
   if (dec_timer)
      dec_timer [2] += prg_timer (&ptimer, STOP);

   return frame;
}

image_t *
decode_state (unsigned state, unsigned level, wfa_t *wfa)
/*
 *  Decode 'state' image of 'wfa' at given 'level'.
 *
 *  Return value.
 *	pointer to decoded state image
 *
 *  Side effects:
 *	'wfa' states > 'state' are removed.  
 */
{
   word_t  *domains [2];
   image_t *img = Calloc (1, sizeof (image_t));

   /*
    *  Generate a new state with a 1.0 transition to 'state'
    */
   remove_states (state + 1, wfa);
   append_edge (state + 1, state, 1.0, 0, wfa);
   wfa->states = state + 2;

   img->color  = NO;
   img->width  = width_of_level (level);
   img->height = height_of_level (level);
   img->format = FORMAT_4_4_4;
   img->pixels [GRAY] = decode_range (state + 1, 0, level, domains, wfa);

   /*
    *  Copy decoded range to the frame buffer
    */
   {
      word_t   *src, *dst;
      unsigned	y;
	    
      src = domains [0];
      dst = img->pixels [GRAY];
      for (y = img->height; y; y--)
      {
	 memcpy (dst, src, width_of_level (level) * sizeof (word_t));
	 src += width_of_level (level);
	 dst += img->width;
      }
      Free (domains [0]);
   }

   return img;
}

word_t *
decode_range (unsigned range_state, unsigned range_label, unsigned range_level,
	      word_t **domain, wfa_t *wfa)
/*
 *  Compute 'wfa' image of range (identified by 'state' and 'label')
 *  at 'range_level (works as function decode_image()).
 *
 *  Return value:
 *	pointer to the pixels in SHORT format
 *
 *  Side effects:
 *	if 'domain' != NULL then also the domain blocks
 *	of the corresponding range blocks are generated
 *      and returned in domain[]
 *	'wfa->level_of_state []' is changed
 */
{
   unsigned   root_state [3];		/* dummy (for alloc_state_images) */
   image_t   *state_image;		/* regenerated state image */
   word_t   **images;			/* pointer to array of pointers
					   to state images */
   u_word_t  *offsets;			/* pointer to array of state image
					   offsets */
   word_t    *range;

   enlarge_image (range_level - (wfa->level_of_state [range_state] - 1),
		  FORMAT_4_4_4, -1, wfa);
   root_state [0] = range_state;
   state_image    = alloc_image (width_of_level (range_level + 1),
				 height_of_level (range_level + 1),
				 NO, FORMAT_4_4_4);
   alloc_state_images (&images, &offsets, state_image, NULL, range_state,
		       range_level + 1, NO, wfa);
   compute_state_images (range_level + 1, images, offsets, wfa);

   range = Calloc (size_of_level (range_level), sizeof (word_t));

   if ((range_level & 1) == 0)		/* square image */
   {
      memcpy (range,
	      images [range_state + (range_level + 1) * wfa->states]
	      + range_label * size_of_level (range_level),
	      size_of_level (range_level) * sizeof (word_t));
   }
   else					/* rectangle */
   {
      word_t   *src, *dst;
      unsigned  y;
      
      src = images [range_state + (range_level + 1) * wfa->states]
	    + range_label * width_of_level (range_level);
      dst = range;
      for (y = height_of_level (range_level); y; y--)
      {
	 memcpy (dst, src, width_of_level (range_level) * sizeof (word_t));
	 dst += width_of_level (range_level);
	 src += width_of_level (range_level + 1);
      }
   }

   if (domain != NULL)			/* copy domain images */
   {
      int      s;			/* domain state */
      unsigned edge;			/* counter */
		
      if (ischild (s = wfa->tree [range_state][range_label]))
	 *domain++ = duplicate_state_image (images [s + (range_level)
						   * wfa->states],
					    offsets [s + (range_level)
						    * wfa->states],
					    range_level);
      for (edge = 0; isedge (s = wfa->into[range_state][range_label][edge]);
	   edge++)
	 *domain++ = duplicate_state_image (images [s + (range_level)
						   * wfa->states],
					    offsets [s + (range_level)
						    * wfa->states],
					    range_level);
      *domain = NULL;
   }
   
   free_state_images (range_level + 1, NO, images, offsets, NULL, range_state,
		      NO, wfa);
   free_image (state_image);
   
   return range;
}

void
smooth_image (unsigned sf, const wfa_t *wfa, image_t *image)
/*
 *  Smooth 'image' along the partitioning boundaries of the 'wfa'
 *  with factor 's'.
 *
 *  No return value.
 *
 *  Side effects:
 *	pixel values of the 'image' are modified with respect to 's'
 */
{
   int	    is, inegs;			/* integer factors of s and 1 - s*/
   unsigned state;			
   unsigned img_width  = image->width;
   unsigned img_height = image->height;
   real_t   s 	       = 1.0 - sf / 200.0;

   if (s < 0.5 || s >= 1)		/* value out of range */
      return;

   is 	 = s * 512 + .5;		/* integer representation of s */
   inegs = (1 - s) * 512 + .5;		/* integer representation of 1 - s */
   
   for (state = wfa->basis_states;
	state < (wfa->wfainfo->color
		 ? wfa->tree [wfa->root_state][0]
		 : wfa->states); state++)
   {
      word_t   *bptr   = image->pixels [Y]; /* pointer to right or
					       lower line */
      unsigned  level  = wfa->level_of_state[state]; /* level of state image */
      unsigned  width  = width_of_level (level); /* size of state image */
      unsigned  height = height_of_level (level); /* size of state image */
      
      if (wfa->y [state][1] >= img_height || wfa->x [state][1] >= img_width)
	 continue;			/* outside visible area */
	 
      if (level % 2)			/* horizontal smoothing */
      {
	 unsigned  i;			/* line counter */
	 word_t   *img1;		/* pointer to left or upper line */
	 word_t   *img2;		/* pointer to right or lower line */

	 img1 = bptr + (wfa->y [state][1] - 1) * img_width
		+ wfa->x [state][1];
	 img2 = bptr + wfa->y [state][1] * img_width + wfa->x [state][1];
	 
	 for (i = min (width, img_width - wfa->x [state][1]); i;
	      i--, img1++, img2++)
	 {
	    int tmp = *img1;
	    
#ifdef HAVE_SIGNED_SHIFT
	    *img1 = (((is * tmp) >> 10) << 1)
		    + (((inegs * (int) *img2) >> 10) << 1);
	    *img2 = (((is * (int) *img2) >> 10) << 1)
		    + (((inegs * tmp) >> 10) << 1);
#else /* not HAVE_SIGNED_SHIFT */
	    *img1 = (((is * tmp) / 1024) * 2)
		    + (((inegs * (int) *img2) / 1024) * 2);
	    *img2 = (((is * (int) *img2) / 1024) * 2)
		    + (((inegs * tmp) / 1024) *2);
#endif /* not HAVE_SIGNED_SHIFT */
	 }
      }
      else				/* vertical smoothing */
      {
	 unsigned  i;			/* line counter */
	 word_t   *img1;		/* pointer to left or upper line */
	 word_t   *img2;		/* pointer to right or lower line */

	 img1 = bptr + wfa->y [state][1] * img_width + wfa->x [state][1] - 1;
	 img2 = bptr + wfa->y [state][1] * img_width + wfa->x [state][1];
	 
	 for (i = min (height, img_height - wfa->y [state][1]); i;
	      i--, img1 += img_width, img2 += img_width)
	 {
	    int tmp = *img1;
	    
#ifdef HAVE_SIGNED_SHIFT
	    *img1 = (((is * tmp) >> 10) << 1)
		    + (((inegs * (int) *img2) >> 10) << 1);
	    *img2 = (((is * (int) *img2) >> 10) << 1)
		    + (((inegs * tmp) >> 10) << 1);
#else /* not HAVE_SIGNED_SHIFT */
	    *img1 = (((is * tmp) / 1024) * 2)
		    + (((inegs * (int) *img2) / 1024) * 2);
	    *img2 = (((is * (int) *img2) / 1024) * 2)
		    + (((inegs * tmp) / 1024) *2);
#endif /* not HAVE_SIGNED_SHIFT */
	 }
      }
   }
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
enlarge_image (int enlarge_factor, format_e format, unsigned y_root,
	       wfa_t *wfa)
/*
 *  Enlarge or reduce size of state images by factor 2^'enlarge_factor'.
 *  Use 4:2:0 subsampling if specified by 'format', else use 4:4:4 format.
 *  'wfa' root state of the first chroma band is given by 'y_root' + 1.
 *
 *  No return value.
 *
 *  Side effects:
 *	coordinates of ranges and motion blocks in the WFA structure 'wfa'
 *	are modified.
 */
{
   
   if (enlarge_factor != 0 || format == FORMAT_4_2_0)
   {
      unsigned state;

      if (enlarge_factor == 0)
      {
	 state 		= y_root + 1;
	 enlarge_factor = -1;
      }
      else
	 state = wfa->basis_states;
      
      for (; state < wfa->states; state++)
      {
	 unsigned label, n;
	 
	 wfa->level_of_state [state]
	    = max (wfa->level_of_state [state] + enlarge_factor * 2, 0);

	 for (label = 0; label < MAXLABELS; label++)
	    if (enlarge_factor > 0)
	    {
	       wfa->x [state][label] <<= enlarge_factor;
	       wfa->y [state][label] <<= enlarge_factor;
	       for (n = enlarge_factor; n; n--)
	       {
		  wfa->mv_tree [state][label].fx *= 2;
		  wfa->mv_tree [state][label].fy *= 2;
		  wfa->mv_tree [state][label].bx *= 2;
		  wfa->mv_tree [state][label].by *= 2;
	       }
	    }
	    else				/* enlarge_factor < 0 */
	    {
	       wfa->x [state][label] >>= - enlarge_factor;
	       wfa->y [state][label] >>= - enlarge_factor;
	       for (n = - enlarge_factor; n; n--)
	       {
		  wfa->mv_tree [state][label].fx /= 2;
		  wfa->mv_tree [state][label].fy /= 2;
		  wfa->mv_tree [state][label].bx /= 2;
		  wfa->mv_tree [state][label].by /= 2;
	       }
	    }
	 if (format == FORMAT_4_2_0 && state == y_root)
	    enlarge_factor--;
      }
   }
}

static void
compute_actual_size (unsigned luminance_root,
		     unsigned *width, unsigned *height, const wfa_t *wfa)
/*
 *  Compute actual size of the frame represented by the given 'wfa'.
 *  (The reconstructed frame may get larger than the original due
 *   to the bintree partitioning.)
 *  If 'luminance_root' < MAXSTATES then the size of chroma ranges (4:2:0).
 *
 *  Return values:
 *	actual 'width' and 'height' of the decoded frame.
 */
{
   unsigned x = 0, y = 0;		/* maximum coordinates */
   unsigned state;			/* counter */
   
   for (state = wfa->basis_states; state < wfa->states; state++)
      if (isedge (wfa->into [state][0][0]) || isedge (wfa->into [state][1][0]))
      {
	 unsigned mult = state > luminance_root ? 2 : 1;
	 
	 x = max ((wfa->x [state][0]
		   + width_of_level (wfa->level_of_state [state])) * mult, x);
	 y = max ((wfa->y [state][0]
		   + height_of_level (wfa->level_of_state [state])) * mult, y);
      }

   if (x & 1)				/* ensure that image size is even */
      x++;
   if (y & 1)
      y++;
   *width  = x;
   *height = y;
}

static void
alloc_state_images (word_t ***images, u_word_t **offsets, const image_t *frame,
		    const unsigned *root_state, unsigned range_state,
		    unsigned max_level, format_e format, const wfa_t *wfa)
/*
 *  Generate list of 'wfa' state images which have to be computed for
 *  each level to obtain the decoded 'frame'. 'root_state[]' denotes the
 *  state images of the three color bands.
 *  'max_level' fives the max. level of a linear combination.
 *  Memory is allocated for every required state image.
 *  Use 4:2:0 subsampling or 4:4:4 'format' for color images.
 *  If 'range_state' > 0 then rather compute image of 'range_state' than 
 *  image of 'wfa->root_state'.
 *
 *  Return values:
 *	'*images'	Pointer to array of state image pointers
 *	'*offsets'	Pointer to array of state image offsets.
 *
 *  Side effects:
 *	The arrays given above are filled with useful values.
 */
{
   word_t   **simg;			/* ptr to list of state image ptr's */
   u_word_t  *offs;			/* ptr to list of offsets */
   unsigned   level;			/* counter */
   
   simg	= Calloc (wfa->states * (max_level + 1), sizeof (word_t *));
   offs	= Calloc (wfa->states * (max_level + 1), sizeof (u_word_t));

   /*
    *  Initialize buffers for those state images which are at 'max_level'.
    */
   if (range_state > 0)			/* a range is given */
   {
      simg [range_state + max_level * wfa->states] = frame->pixels [GRAY];
      offs [range_state + max_level * wfa->states] = frame->width;
   }
   else
   {
      unsigned state;

      for (state = wfa->basis_states; state <= root_state [Y]; state++)
	 if (wfa->level_of_state [state] == max_level)
	 {
	    simg [state + max_level * wfa->states]
	       = (frame->pixels [Y] + wfa->y [state][0] * frame->width
		  + wfa->x [state][0]);
	    offs [state + max_level * wfa->states] = frame->width;
	 }
      if (frame->color)
      {
	 unsigned width = format == FORMAT_4_2_0 ?
			  (frame->width >> 1) : frame->width;
	 for (; state < wfa->states; state++)
	    if (wfa->level_of_state [state] == max_level)
	    {
	       simg [state + max_level * wfa->states]
		  = (frame->pixels [state > root_state [Cb] ? Cr : Cb]
		     + wfa->y [state][0] * width + wfa->x [state][0]);
	       offs [state + max_level * wfa->states] = width;
	    }
      }
   }
   
   /*
    *  Generate list of state images which must be computed at each level
    */
   for (level = max_level; level > 0; level--)
   {
      int      child, domain;
      unsigned state, label, edge;
      
      /*
       *  Range approximation with child. 
       */
      for (state = 1; state < (range_state > 0 ?
			       range_state + 1 : wfa->states); state++)
	 if (simg [state + level * wfa->states])
	    for (label = 0; label < MAXLABELS; label++)
	       if (ischild (child = wfa->tree[state][label]))
	       {
		  if (isedge (wfa->into[state][label][0]))
		  {
		     /*
		      *  Allocate new image block.
		      */
		     simg [child + (level - 1) * wfa->states]
			= Calloc (size_of_level (level - 1), sizeof (word_t));
		     offs [child + (level - 1) * wfa->states]
			= width_of_level (level - 1);
		  }
		  else
		  {
		     /*
		      *  Use image block and offset of parent.
		      */
		     if (level & 1)	/* split vertically */
		     {
			simg [child + (level - 1) * wfa->states]
			   = (simg [state + level * wfa->states]
			      + label * (height_of_level (level - 1)
					 * offs [state
						+ level * wfa->states]));
		     }
		     else		/* split horizontally */
		     {
			simg [child + (level - 1) * wfa->states]
			   = (simg [state + level * wfa->states]
			      + label * width_of_level (level - 1));
		     }
		     offs [child + (level - 1) * wfa->states]
			= offs [state + level * wfa->states];
		  }
	       }
      /*
       *  Range approximation with linear combination 
       */
      for (state = 1; state < (range_state > 0 ?
			       range_state + 1 : wfa->states); state++)
	 if (simg [state + level * wfa->states])
	    for (label = 0; label < MAXLABELS; label++)
	       for (edge = 0; isedge (domain = wfa->into[state][label][edge]);
		    edge++)
	       {
		  if (domain > 0	/* don't allocate memory for state 0 */
		      && !simg [domain + (level - 1) * wfa->states])
		  {
		     simg [domain + (level - 1) * wfa->states]
			= Calloc (size_of_level (level - 1), sizeof (word_t));
		     offs [domain + (level - 1) * wfa->states]
			= width_of_level (level - 1);
		  }
	       }
      
   }

   *images  = simg;
   *offsets = offs;
}

static void
free_state_images (unsigned max_level, bool_t color, word_t **state_image,
		   u_word_t *offset, const unsigned *root_state,
		   unsigned range_state, format_e format, const wfa_t *wfa)
/*
 *  Free memory of state images.
 *  For more details refer to the inverse function 'alloc_state_images()'.
 *
 *  No return value.
 *
 *  Side effects:
 *	arrays 'state_image' and 'offset' are discarded.
 */
{
   word_t   marker;			/* ptr is required as a marker */
   unsigned level;

   if (range_state > 0)
   {
      state_image [range_state + max_level * wfa->states] = &marker;
   }
   else
   {
      unsigned state;
      
      /*
       *  Initialize state image array with states at 'max_level'
       */
      for (state = wfa->basis_states; state <= root_state [Y]; state++)
	 if (wfa->level_of_state [state] == max_level)
	    state_image [state + max_level * wfa->states] = &marker;

      if (color)
      {
	 if (format == FORMAT_4_2_0)
	    level = max_level - 2;
	 else
	    level = max_level;
      
	 for (; state < wfa->states; state++)
	    if (wfa->level_of_state [state] == level)
	       state_image [state + level * wfa->states] = &marker;
      }
   }
   
   for (level = max_level; level > 0; level--)
   {
      int      domain, child;
      unsigned state, label, edge;
      /*
       *  Range approximation with child. 
       */
      for (state = 1; state < (range_state > 0 ?
			       range_state + 1 : wfa->states); state++)
	 if (state_image [state + level * wfa->states])
	    for (label = 0; label < MAXLABELS; label++)
	       if (ischild (child = wfa->tree[state][label]))
	       {
		  if (isedge (wfa->into[state][label][0])
		      && (state_image [child + (level - 1) * wfa->states]
			  != &marker))
		     Free (state_image [child + (level - 1) * wfa->states]);
		  state_image [child + (level - 1) * wfa->states] = &marker;
	       }
      /*
       *  Range approximation with linear combination 
       */
      for (state = 1; state < (range_state > 0 ?
			       range_state + 1 : wfa->states);
	   state++)
	 if (state_image [state + level * wfa->states])
	    for (label = 0; label < MAXLABELS; label++)
	       for (edge = 0; isedge (domain = wfa->into[state][label][edge]);
		    edge++)
		  if (domain > 0	
		      && (state_image [domain + (level - 1) * wfa->states]
			  != NULL)
		      && (state_image [domain + (level - 1) * wfa->states]
			  != &marker))
		  {
		     Free (state_image [domain + (level - 1) * wfa->states]);
		     state_image [domain + (level - 1) * wfa->states]
			= &marker;
		  }
   }
   Free (state_image);
   Free (offset);
}

static void
compute_state_images (unsigned max_level, word_t **simg,
		      const u_word_t *offset, const wfa_t *wfa)
/*
 *  Compute all state images of the 'wfa' at level {1, ... , 'max_level'}
 *  which are marked in the array 'simg' (offsets of state images
 *  are given by 'offset').
 *
 *  Warning: Several optimizations are used in this function making 
 *  it difficult to understand.
 *
 *  No return value.
 *
 *  Side effects:
 *	state images (given by pointers in the array 'state_image')
 *	are computed.
 */
{
   unsigned level, state;
     
   /*
    *  Copy one-pixel images in case state_image pointer != &final distr.
    */

   for (state = 1; state < wfa->states; state++)
      if (simg [state] != NULL)		/* compute image at level 0 */
	 *simg [state] = (int) (wfa->final_distribution[state] * 8 + .5) * 2;

   /*
    *  Compute images of states
    *  Integer arithmetics are used rather than floating point operations.
    *  'weight' gives the weight in integer notation
    *  'src', 'dst', and 'idst' are pointers to the source and
    *  destination pixels (short or integer format), respectively.
    *  Short format : one operation per register (16 bit mode). 
    *  Integer format : two operations per register (32 bit mode). 
    *  'src_offset', 'dst_offset', and 'dst_offset' give the number of
    *  pixels which have to be omitted when jumping to the next image row.
    */
   for (level = 1; level <= max_level; level++) 
   {
      unsigned label;
      unsigned width  = width_of_level (level - 1);
      unsigned height = height_of_level (level - 1);
      
      for (state = 1; state < wfa->states; state++)
	 if (simg [state + level * wfa->states] != NULL)
	    for (label = 0; label < MAXLABELS; label++)
	       if (isedge (wfa->into [state][label][0]))
	       {
		  unsigned  edge;
		  int       domain;
		  word_t   *range;	/* address of current range */
		  bool_t    prediction_used; /* ND prediction found ? */

		  /*
		   *  Compute address of range image
		   */
		  if (level & 1)	/* split vertically */
		  {
		     range = simg [state + level * wfa->states]
			     + label * (height_of_level (level - 1)
					* offset [state
						 + level * wfa->states]);
		  }
		  else			/* split horizontally */
		  {
		     range = simg [state + level * wfa->states]
			     + label * width_of_level (level - 1);
		  }

		  /*
		   *  Generate the state images by adding the corresponding 
		   *  weighted state images:
		   *  subimage [label] =
		   *       weight_1 * image_1 + ... + weight_n * image_n
		   */
		  if (!ischild (domain = wfa->tree[state][label]))
		     prediction_used = NO;
		  else
		  {
		     unsigned  y;
		     word_t   *src;
		     word_t   *dst;
		     unsigned  src_offset;
		     unsigned  dst_offset;

		     prediction_used = YES;
		     /*
		      *  Copy child image
		      */
		     src        = simg [domain + (level - 1) * wfa->states];
		     src_offset = offset [domain + (level - 1) * wfa->states] ;
		     dst        = range;
		     dst_offset	= offset [state + level * wfa->states];
		     for (y = height; y; y--)
		     {
			memcpy (dst, src, width * sizeof (word_t));
			src += src_offset;
			dst += dst_offset;
		     }
		  }

		  if (!prediction_used
		      && isedge (domain = wfa->into[state][label][0]))
		  {
		     /*
		      *  If prediction is not used then the range is
		      *  filled with the first domain. No addition is needed.
		      */
		     edge = 0;
		     if (domain != 0)
		     {
			int	  weight;
			word_t 	 *src;
			unsigned  src_offset;

			src        = simg [domain + ((level - 1)
						     * wfa->states)];
			src_offset = offset [domain + ((level - 1)
						       * wfa->states)] - width;
			weight     = wfa->int_weight [state][label][edge];
			
			if (width == 1)	/* can't add two-pixels in a row */
			{
			   word_t   *dst;
			   unsigned  dst_offset;
			   
			   dst        = range;
			   dst_offset = offset [state + level * wfa->states]
					- width;
#ifdef HAVE_SIGNED_SHIFT
			   *dst++ = ((weight * (int) *src++) >> 10) << 1;
#else 					/* not HAVE_SIGNED_SHIFT */
			   *dst++ = ((weight * (int) *src++) / 1024) * 2;
#endif /* not HAVE_SIGNED_SHIFT */
			   if (height == 2) 
			   {
			      src += src_offset;
			      dst += dst_offset;
#ifdef HAVE_SIGNED_SHIFT
			      *dst++ = ((weight * (int) *src++) >> 10) << 1;
#else /* not HAVE_SIGNED_SHIFT */
			      *dst++ = ((weight * (int) *src++) / 1024) * 2;
#endif /* not HAVE_SIGNED_SHIFT */
			   }
			}
			else
			{
			   unsigned  y;
			   int 	    *idst;
			   unsigned  idst_offset;
			   
			   idst        = (int *) range;
			   idst_offset = (offset [state + level * wfa->states]
					  - width) / 2;
			   for (y = height; y; y--)
			   {
			      int *comp_dst = idst + (width >> 1);
			      
			      for (; idst != comp_dst; )
 			      {
				 int tmp; /* temp. value of adjacent pixels */
#ifdef HAVE_SIGNED_SHIFT
#	ifndef WORDS_BIGENDIAN
                                 tmp = (((weight * (int) src [1]) >> 10) << 17)
				       | (((weight * (int) src [0]) >> 9)
					  & 0xfffe);
#	else /* not WORDS_BIGENDIAN */
                                 tmp = (((weight * (int) src [0]) >> 10) << 17)
				       | (((weight * (int) src [1]) >> 9)
					  & 0xfffe);
#	endif /* not WORDS_BIGENDIAN */
#else /* not HAVE_SIGNED_SHIFT */
#	ifndef WORDS_BIGENDIAN
                                 tmp = (((weight * (int) src [1]) / 1024)
					* 131072)
				       | (((weight * (int) src [0])/ 512)
					  & 0xfffe);
#	else /* not WORDS_BIGENDIAN */
                                 tmp = (((weight * (int) src [0]) / 1024)
					* 131072)
				       | (((weight * (int) src [1]) / 512)
					  & 0xfffe);
#	endif /* not WORDS_BIGENDIAN */
#endif /* not HAVE_SIGNED_SHIFT */
				 src    +=  2;
				 *idst++ = tmp & 0xfffefffe;
			      }
			      src  += src_offset;
			      idst += idst_offset;
			   }
			}
		     }
		     else
		     {
			int weight = (int) (wfa->weight[state][label][edge]
					    * wfa->final_distribution[0]
					    * 8 + .5) * 2;
			/*
			 *  Range needs domain 0
			 *  (the constant function f(x, y) = 1),
			 *  hence a faster algorithm is used.
			 */
			if (width == 1)	/* can't add two-pixels in a row */
			{
			   word_t   *dst;
			   unsigned  dst_offset;
			   
			   dst        = range;
			   dst_offset = offset [state + level * wfa->states]
					- width;
			   
			   *dst++ = weight;
			   if (height == 2)
			   {
			      dst += dst_offset;
			      *dst++ = weight;
			   }
			}
			else
			{
			   unsigned  x, y;
			   int 	    *idst;
			   unsigned  idst_offset;
			   
			   weight      = (weight * 65536) | (weight & 0xffff);
			   idst	       = (int *) range;
			   idst_offset = offset [state + level * wfa->states]
					 / 2;
			   for (x = width >> 1; x; x--)
			      *idst++ = weight & 0xfffefffe;
			   idst += (offset [state + level * wfa->states]
				    - width) / 2;

			   for (y = height - 1; y; y--)
			   {
			      memcpy (idst, idst - idst_offset,
				      width * sizeof (word_t));
			      idst += idst_offset;
			   }
			}
		     }
		     edge = 1;
		  }
		  else
		     edge = 0;
		  
		  /*
		   *  Add remaining weighted domain images to current range
		   */
		  for (; isedge (domain = wfa->into[state][label][edge]);
		       edge++)
		  {
		     if (domain != 0)
		     {
			word_t 	 *src;
			unsigned  src_offset;
			int	  weight;

			src        = simg [domain + (level - 1) * wfa->states];
			src_offset = offset [domain + ((level - 1)
						       * wfa->states)] - width;
			weight     = wfa->int_weight [state][label][edge];
			
			if (width == 1)	/* can't add two-pixels in a row */
			{
			   word_t   *dst;
			   unsigned  dst_offset;
			   
			   dst        = range;
			   dst_offset = offset [state + level * wfa->states]
					- width;

#ifdef HAVE_SIGNED_SHIFT
			   *dst++ += ((weight * (int) *src++) >> 10) << 1;
#else /* not HAVE_SIGNED_SHIFT */
			   *dst++ += ((weight * (int) *src++) / 1024) * 2;
#endif /* not HAVE_SIGNED_SHIFT */
			   if (height == 2) 
			   {
			      src += src_offset;
			      dst += dst_offset;
#ifdef HAVE_SIGNED_SHIFT
			      *dst++ += ((weight * (int) *src++) >> 10) << 1;
#else /* not HAVE_SIGNED_SHIFT */
			      *dst++ += ((weight * (int) *src++) / 1024) * 2;
#endif /* not HAVE_SIGNED_SHIFT */
			   }
			}
			else
			{
			   int 	    *idst;
			   unsigned  idst_offset;
			   unsigned  y;
			   
			   idst        = (int *) range;
			   idst_offset = (offset [state + level * wfa->states]
					  - width) / 2;
			   
			   for (y = height; y; y--)
			   {
			      int *comp_dst = idst + (width >> 1);
			      
			      for (; idst != comp_dst;)
 			      {
				 int tmp; /* temp. value of adjacent pixels */
#ifdef HAVE_SIGNED_SHIFT
#	ifndef WORDS_BIGENDIAN
                                 tmp = (((weight * (int) src [1]) >> 10) << 17)
				       | (((weight * (int) src [0]) >> 9)
					  & 0xfffe);
#	else /* not WORDS_BIGENDIAN */
                                 tmp = (((weight * (int)src [0]) >> 10) << 17)
				       | (((weight * (int)src [1]) >> 9)
					  & 0xfffe);
#	endif /* not WORDS_BIGENDIAN */
#else /* not HAVE_SIGNED_SHIFT */
#	ifndef WORDS_BIGENDIAN
                                 tmp = (((weight * (int) src [1]) / 1024)
					* 131072)
				       | (((weight * (int) src [0])/ 512)
					  & 0xfffe);
#	else /* not WORDS_BIGENDIAN */
                                 tmp = (((weight * (int) src [0]) / 1024)
					* 131072)
				       | (((weight * (int) src [1])/ 512)
					  & 0xfffe);
#	endif /* not WORDS_BIGENDIAN */
#endif /* not HAVE_SIGNED_SHIFT */
				 src +=  2;
				 *idst = (*idst + tmp) & 0xfffefffe;
				 idst++;
			      }
			      src  += src_offset;
			      idst += idst_offset;
			   }
			}
		     }
		     else
		     {
			int weight = (int) (wfa->weight[state][label][edge]
					    * wfa->final_distribution[0]
					    * 8 + .5) * 2;
			/*
			 *  Range needs domain 0
			 *  (the constant function f(x, y) = 1),
			 *  hence a faster algorithm is used.
			 */
			if (width == 1)	/* can't add two-pixels in a row */
			{
			   word_t   *dst;
			   unsigned  dst_offset;
			   
			   dst        = range;
			   dst_offset = offset [state + level * wfa->states]
					- width;
			   
			   *dst++ += weight;
			   if (height == 2)
			   {
			      dst    += dst_offset;
			      *dst++ += weight;
			   }
			}
			else
			{
			   int 	    *idst;
			   unsigned  idst_offset;
			   unsigned  y;
			   
			   weight      = (weight * 65536) | (weight & 0xffff);
			   idst	       = (int *) range;
			   idst_offset = (offset [state + level * wfa->states]
					  - width) /2;
			   
			   for (y = height; y; y--)
			   {
			      int *comp_dst = idst + (width >> 1);
			      
			      for (; idst != comp_dst; )
			      {
				 *idst = (*idst + weight) & 0xfffefffe;
                                 idst++;
			      }
			      idst += idst_offset;
			   }
			}
		     }
		  }
	       } 
   }
}

static word_t *
duplicate_state_image (const word_t *domain, unsigned offset, unsigned level)
/*
 *  Allocate new memory block 'pixels' and copy pixel values of 'domain' 
 *  (size and pixel offset are given by 'level' and 'offset')
 *  to the lock 'pixels'.
 *
 *  Return value:
 *	pointer to the new domain block
 */
{
   word_t *dst, *pixels;
   int	   y, n;

   dst = pixels = Calloc (size_of_level (level), sizeof (word_t));

   if (domain)
      for (y = height_of_level (level); y; y--)
      {
	 memcpy (dst, domain, width_of_level (level) * sizeof (word_t));
	 dst    += width_of_level (level);
	 domain += offset;
      }
   else					/* state 0 */
      for (n = size_of_level (level); n; n--)
	 *dst++ = (int) (128 * 8 + .5) * 2;

   return pixels;
}
