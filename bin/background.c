/*
 *  background.c:	Generate different XWFA backgrounds
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/10/22 10:44:47 $
 *  $Author: hafner $
 *  $Revision: 4.2 $
 *  $State: Exp $
 */

#include "config.h"

#include <gtk/gtk.h>

#include "types.h"
#include "macros.h"

#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */

#include "wfa.h"
#include "decoder.h"
#include "image.h"
#include "misc.h"
#include "decoder.h"
#include "motion.h"
#include "wfalib.h"
#include "xwfa.h"
#include "dialog.h"
#include "background.h"
#include "error.h"

/******************************************************************************

				prototypes
  
******************************************************************************/

static void
draw_color (GtkPreview *preview, image_t *image);

/*****************************************************************************

				public code
  
*****************************************************************************/

void
generate_wfa_backgrounds (xwfa_display_t *display)
{
   unsigned  band;
   image_t  *delta 	= NULL;
   image_t  *prediction = NULL;
   image_t  *src, *dst;
   int	     width, height;

   if (display->enlarge_factor > 0)
   {
      width  = display->wfa->wfainfo->width  << display->enlarge_factor;
      height = display->wfa->wfainfo->height << display->enlarge_factor; 
   }
   else
   { 
      width  = display->wfa->wfainfo->width  >> - display->enlarge_factor;
      height = display->wfa->wfainfo->height >> - display->enlarge_factor;
      if (width & 1)
	 width++;
      if (height & 1)
	 height++;
   }

   /*
    *  Discard old images
    */
   {
      background_e	bg;
      
      for (bg = 0; bg < BG_NULL; bg++)
	 switch (bg)
	 {
	    case BG_DELTA:
	    case BG_PREDICTION:
	    case BG_WFA:
	       if (display->bg_image [bg])
	       {
		  free_image (display->bg_image [bg]);
		  display->bg_image [bg] = NULL;
	       }
	       break;
	    default:
	       break;
	 }
   }

   display->bg_image [BG_WFA] = decode_image (width, height, FORMAT_4_4_4,
					      NULL, display->video->wfa);
   smooth_image (display->smoothing_factor,
		 display->video->wfa, display->bg_image [BG_WFA]);

   if (display->video->wfa->frame_type != I_FRAME) /* motion images */
   {
      restore_mc (display->enlarge_factor, display->bg_image [BG_WFA],
		  display->video->past, display->video->future,
		  display->video->wfa);
      src = delta = decode_image (width, height, FORMAT_4_4_4, NULL,
				  display->video->wfa);
      dst = prediction = alloc_image (delta->width, delta->height, delta->color, 
				      delta->format);
   }
   else if (display->lc_prediction)	/* non-deterministic prediction */
   {
      word_t	*range;
      int	x, y;
      wfa_t	*wfa = display->video->wfa;
      unsigned	state, label;
      
      src = prediction = alloc_image (width, height,
				      display->video->frame->color,
				      display->video->frame->format);
      for (state = wfa->basis_states; state < wfa->states; state++)
	 for (label = 0; label < MAXLABELS; label++)
	    if (isedge (wfa->into [state][label][0])
		&& ischild (wfa->tree [state][label]))
	    {
	       word_t	*domains [MAXEDGES + 2]; /* domain blocks */
	       int	level;
	       int	edge;
		  
	       level = wfa->level_of_state [state] - 1;
	       range = decode_range (state, label, level, domains, wfa);

	       for (edge = 1; domains [edge]; edge++)
	       {
		  word_t	*dst;
		  word_t	*src;
		  int	weight;
		     
		  dst = prediction->pixels [GRAY] + wfa->x [state][label]
			+ (wfa->y [state][label] * prediction->width);
		  src = domains [edge];
		  weight = wfa->int_weight [state][label][edge - 1];
		  for (y = height_of_level (level); y; y--)
		  {
		     for (x = width_of_level (level); x; x--)
#ifdef HAVE_SIGNED_SHIFT
			*dst++ += ((weight * (int) *src++) >> 10) << 1;
#else /* not HAVE_SIGNED_SHIFT */
		        *dst++ += ((weight * (int) *src++) / 1024) * 2;
#endif /* not HAVE_SIGNED_SHIFT */
		     dst += prediction->width - width_of_level (level);
		  }
		  Free (domains [edge]);
	       }
	       Free (domains [0]);
	       Free (range);
	    }
      dst = delta = alloc_image (width, height, prediction->color,
				 prediction->format);
   }
   else
      src = dst = NULL;

   if (src != NULL && dst != NULL)
   {
      for (band = first_band (delta->color); band <= last_band (delta->color);
	   band++)
      {
	 int		n;		/* pixel counter */
	 word_t	*o, *d, *s;		/* pointer to current pixel */
	 
	 o = display->video->sframe
	     ? display->video->sframe->pixels [band]
	     : display->video->frame->pixels [band];
	 d = dst->pixels [band];
	 s = src->pixels [band];
	 
	 for (n = delta->width * delta->height; n; n--)
	    *d++ = *o++ - *s++;
      }
      display->bg_image [BG_PREDICTION] = prediction;
      display->bg_image [BG_DELTA]      = delta;
   }
}

void
generate_image_backgrounds (char *image_name, image_t **bg_image)
{
   int		band;

   if (bg_image [BG_ORIGINAL])
      free_image (bg_image [BG_ORIGINAL]);
   bg_image [BG_ORIGINAL] = read_image (image_name);
   
   if (bg_image [BG_DIFF])
      free_image (bg_image [BG_DIFF]);
   bg_image [BG_DIFF] = NULL;
   
   if (!bg_image [BG_WFA])
      return;

   if (!same_image_type (bg_image [BG_ORIGINAL], bg_image [BG_WFA]))
   {
      free_image (bg_image [BG_ORIGINAL]);
      bg_image [BG_ORIGINAL] = NULL;
      dialog_popup (DIALOG_ERROR, "Original image and decoded WFA image\n"
		    "must be of same size and format", NULL, NULL);
      return;
   }

   bg_image [BG_DIFF] = alloc_image (bg_image [BG_ORIGINAL]->width,
				     bg_image [BG_ORIGINAL]->height,
				     bg_image [BG_ORIGINAL]->color,
				     bg_image [BG_ORIGINAL]->format);
   
   for (band = first_band (bg_image [BG_ORIGINAL]->color);
	band <= last_band (bg_image [BG_ORIGINAL]->color);
	band++)
   {
      int		n;		/* pixel counter */
      word_t	*o, *d, *s;		/* pointer to current pixel */
      
      o = bg_image [BG_ORIGINAL]->pixels [band];
      d = bg_image [BG_DIFF]->pixels [band];
      s = bg_image [BG_WFA]->pixels [band];
	 
      for (n = bg_image [BG_ORIGINAL]->width * bg_image [BG_ORIGINAL]->height;
	   n; n--)
	 *d++ = *o++ - *s++;
   }
}

void
draw_background (background_e bg_type, image_t **bg_image, int number,
		 GtkWidget *preview)
/*
 *  Fill background of 'click_area' with 'bg' images.
 *
 *  No return value.
 *
 *  Side effects:
 *	'click_area' background is changed.
 */
{
   if (number < 3)
      draw_grayscale (GTK_PREVIEW (preview), bg_image [bg_type], number);
   else
      draw_color (GTK_PREVIEW (preview),
		  bg_image [bg_type] != NULL
		  ? bg_image [bg_type] : bg_image [BG_WFA]);
}

void
draw_grayscale (GtkPreview *preview, image_t *image, int band)
/*
 *  Copy color 'band' of 'image' into preview widget '*preview'.
 *  If '*preview' == NULL then generate new widget.
 *
 *  No return value.
 *
 *  Side effects:
 *	'*preview' is set to the new or changed preview widget
 */
{
   if (GTK_WIDGET (preview)->requisition.width)
   {
      unsigned	x, y;
      byte_t   *row;

      row = Calloc (GTK_WIDGET (preview)->requisition.width, sizeof (gchar));

      if (image == NULL)
      {
	 for (y = 0;
	      y < (unsigned) GTK_WIDGET (preview)->requisition.height; y++)
	 {
	    for (x = 0;
		 x < (unsigned) GTK_WIDGET (preview)->requisition.width; x++)
	       row [x] = 60;

	    gtk_preview_draw_row (preview, row, 0, y,
				  GTK_WIDGET (preview)->requisition.width);
	 }
      }
      else
      {
	 word_t	  *pixels;
	 unsigned *gray_clip;
      
	 gtk_preview_size (preview, image->width, image->height);
	 gray_clip = init_clipping () + 128;
	 gtk_widget_push_visual (gtk_preview_get_visual ());
	 gtk_widget_push_colormap (gtk_preview_get_cmap ());

	 pixels = image->pixels [band];
   
	 for (y = 0; y < image->height; y++)
	 {
	    for (x = 0; x < image->width; x++)
	       row [x] = gray_clip [*pixels++ / 16];

	    gtk_preview_draw_row (preview, row, 0, y, image->width);
	 }
	 gtk_widget_pop_colormap ();
	 gtk_widget_pop_visual ();
      }
      Free (row);
   }
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
draw_color (GtkPreview *preview, image_t *image)
/*
 *  Copy 'image' into preview widget '*preview'.
 *  If '*preview' == NULL then generate new widget.
 *
 *  No return value.
 *
 *  Side effects:
 *	'*preview' is set to the new or changed preview widget
 */
{
   if (GTK_WIDGET (preview)->requisition.width)
   {
      int	x, y;
      byte_t	*row;

      row = Calloc (GTK_WIDGET (preview)->requisition.width,
		    sizeof (gchar) * 3);
      if (image == NULL)
      {
	 for (y = 0; y < GTK_WIDGET (preview)->requisition.height; y++)
	 {
	    byte_t	*ptr;

	    ptr = row;
	 
	    for (x = 0; x < GTK_WIDGET (preview)->requisition.width; x++)
	    {
	       *ptr++ = 60;
	       *ptr++ = 60;
	       *ptr++ = 60;
	    }
	 
	    gtk_preview_draw_row (preview, row, 0, y,
				  GTK_WIDGET (preview)->requisition.width);
	 }
      }
      else
      {
	 word_t	  *yptr, *cbptr, *crptr;
	 unsigned *gray_clip;
      
	 gtk_preview_size (preview, image->width, image->height);
	 gray_clip = init_clipping ();
   
	 gtk_widget_push_visual (gtk_preview_get_visual ());
	 gtk_widget_push_colormap (gtk_preview_get_cmap ());

	 yptr  = image->pixels [Y];
	 cbptr = image->pixels [Cb];
	 crptr = image->pixels [Cr];
   
	 for (y = 0; y < image->height; y++)
	 {
	    byte_t	*ptr;
	    int		lu, cb, cr;
      
	    ptr = row;
	    for (x = 0; x < image->width; x++)
	    {
	       lu  = *yptr++  / 16 + 128;
	       cb = *cbptr++ / 16;
	       cr = *crptr++ / 16;

	       *ptr++ = gray_clip [(int) (lu + 1.4022 * cr + 0.5)];
	       *ptr++ = gray_clip [(int) (lu - 0.7145 * cr -
					  0.3456 * cb + 0.5)];
	       *ptr++ = gray_clip [(int) (lu + 1.7710 * cb + 0.5)];
	    }

	    gtk_preview_draw_row (preview, row, 0, y, image->width);
	 }

	 gtk_widget_pop_colormap ();
	 gtk_widget_pop_visual ();
      }
      Free (row);
   }
}

