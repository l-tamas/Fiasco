/*
 *  drawing.c:		Drawing of various objects for XWFA
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/04 16:27:44 $
 *  $Author: hafner $
 *  $Revision: 4.2 $
 *  $State: Exp $
 */

#include "config.h"

#include "types.h"
#include "macros.h"

#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */

#include <gtk/gtk.h>

#include "wfa.h"
#include "xwfa.h"
#include "misc.h"
#include "wfalib.h"
#include "background.h"
#include "drawing.h"
#include "error.h"
 
/******************************************************************************

			      local variables
  
*****************************************************************************/

static GdkGC	*gc_color [GC_NULL];

static int	range_x [MAXEDGES + 2];
static int	range_y [MAXEDGES + 2];
static int	range_l [MAXEDGES + 2];
static int	range_n [MAXEDGES + 2];
static int	range_band       = -1;
static int	range_state      = -1;
static int	range_label      = -1;
static int	prediction_state = -1;
static int	prediction_label = -1;
static word_t	*domains         = NULL;

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
show_mc_coord (int prediction_state, int prediction_label, const wfa_t *wfa,
	       xwfa_display_t *display);
static void
highlight_basis_image (int state, GdkGC *gc, xwfa_display_t *display);
static gint
basis_event (GtkWidget *widget, GdkEvent  *event, gpointer ptr);
static void
draw_mc (int state, int label, motion_e motion_display, const wfa_t *wfa,
	 GtkWidget *preview, xwfa_display_t *display);
static void
draw_motion_vector (int state, int label, const wfa_t *wfa, GtkWidget *preview);

/*****************************************************************************

				public code
  
*****************************************************************************/

void
draw_lc_components (int state, int label, bool_t use_factor,
		    const wfa_t *orig_wfa, const xwfa_display_t *display)
/*
 *  Show factors and domains of the range approximation (given by 'state'
 *  and 'label'). If 'use_factor' is TRUE the show weighted domains else
 *  domains are shown as such.
 *
 *  No return value.
 */
{
   wfa_t	*wfa;
   int		display_level;
   image_t	range;			/* range block */
   word_t	*domains [MAXEDGES + 2]; /* domain blocks */
   int		n, p, edge;
   char		text [MAXSTRLEN];

   wfa = alloc_wfa (NO);
   copy_wfa (wfa, orig_wfa);
		     
   display_level = 12 - (wfa->level_of_state [state] % 2 ? 0 : 1);
   range.color   = NO;
   range.width   = width_of_level (display_level);
   range.height  = height_of_level (display_level);
   range.format  = FORMAT_4_4_4;
   range.pixels [GRAY] = decode_range (state, label, display_level, domains, wfa);
   if (display_level % 2)		/* rectangle */
   {
      word_t    *dst, *src, *square;
      int	y;
		     
      square = Calloc (width_of_level (display_level + 1)
		       * height_of_level (display_level), sizeof (word_t));
      src = range.pixels [GRAY];
      dst = square;
      for (y = range.height; y; y--)
      {
	 int x;
	 
	 memcpy (dst, src, width_of_level (display_level) * sizeof (word_t));
	 src += range.width;
	 dst += range.width;
	 for (x = range.width; x; x--)
	    *dst++ = 86 * 16;
      }
      Free (range.pixels [GRAY]);
      range.pixels [GRAY] = square;
      range.width         = width_of_level (display_level + 1);
   }
   draw_grayscale (GTK_PREVIEW (display->lc_image [0]), &range, GRAY);
   gtk_widget_draw (display->lc_image [0], NULL);
   sprintf (text, "%d, %d = ", state, label);
   gtk_label_set (GTK_LABEL (display->lc_label [0]), text);
   gtk_widget_draw (display->lc_label [0], NULL);
   for (n = 0, edge = 0; domains [n]; n++, edge++)
   {
      word_t	*src, *dst;
      int	y;

      if (n == 0 && ischild (wfa->tree [state][label]))
      {
	 sprintf (text, "Child: %d", wfa->tree [state][label]);
	 gtk_label_set (GTK_LABEL (display->lc_label [n + 1]), text);
	 gtk_widget_draw (display->lc_label [n + 1], NULL);
	 edge--;
      }
      else
      {
	 sprintf (text, "%+.3fx%d", (double) wfa->weight [state][label][edge],
		  wfa->into [state][label][edge]);
	 gtk_label_set (GTK_LABEL (display->lc_label [n + 1]), text);
	 gtk_widget_draw (display->lc_label [n + 1], NULL);
	 if (use_factor)
	    for (p = width_of_level (display_level)
		     * height_of_level (display_level); p; p--)
#ifdef HAVE_SIGNED_SHIFT
	       domains [n][p - 1]
		  = ((wfa->int_weight [state][label][edge]
		      * (int) domains [n][p - 1]) >> 10) << 1;
#else /* not HAVE_SIGNED_SHIFT */
	       domains [n][p - 1]
		  = ((wfa->int_weight [state][label][edge]
		      * (int) domains [n][p - 1]) / 1024) * 2;
#endif /* not HAVE_SIGNED_SHIFT */
      }
      src = domains [n];
      dst = range.pixels [GRAY];
      for (y = range.height; y; y--)
      {
	 memcpy (dst, src, width_of_level (display_level) * sizeof (word_t));
	 src += width_of_level (display_level);
	 dst += range.width;
      }
      draw_grayscale (GTK_PREVIEW (display->lc_image [n + 1]), &range, GRAY);
      gtk_widget_draw (display->lc_image [n + 1], NULL);
      Free (domains [n]);
      domains [n] = NULL;
   }
   for (p = range.width * range.height; p; p--)
      range.pixels [GRAY][p - 1] = (0xd6 - 128) * 16;
   for (; n < MAXEDGES; n++)
   {
      draw_grayscale (GTK_PREVIEW (display->lc_image [n + 1]),
		      &range, GRAY);
      gtk_widget_draw (display->lc_image [n + 1], NULL);
      gtk_label_set (GTK_LABEL (display->lc_label [n + 1]), " ");
      gtk_widget_draw (display->lc_label [n + 1], NULL);
   }

   Free (range.pixels [GRAY]);
   free_wfa (wfa);
}

void
draw_ranges (bool_t draw, int band, xwfa_display_t *display)
/*
 *  Draw grid of ranges of color 'band' if 'draw' is TRUE.
 *
 *  No return value.
 */
{
   if (draw && band < 3)
   {
      int	start [4];		/* start states of each color band */
      int	state;			/* counter */
      int	label;			/* counter */
      wfa_t	*wfa = display->video->wfa; /* abbrev */
	 
      start [0] = wfa->basis_states;
      if (wfa->wfainfo->color)
      {
	 start [1] = wfa->tree [wfa->tree[wfa->root_state][0]][0] + 1;
	 start [2] = wfa->tree [wfa->tree[wfa->root_state][0]][1] + 1;
	 start [3] = wfa->states;
      }
      else
	 start [1] = wfa->states;

      for (state = start [band]; state < start [1 + band]; state++)
	 for (label = 0; label < MAXLABELS; label++)
	    if (isedge (wfa->into [state][label][0]))
	       draw_state_child (state, label, wfa, GC_RANGE,
				 display->click_areas [band], display);
   }
}

void
draw_nd_prediction (bool_t draw, int motion, int band, xwfa_display_t *display)
/*
 *  Draw grid of predicted ranges of color 'band' if 'draw' is TRUE.
 *  'motion' is a mask defining which types of precition should be displayed. 
 *
 *  No return value.
 */
{
   if (band < 3)
   {
      int	start [4];
      int	state, label;		/* counter */
      wfa_t	*wfa = display->video->wfa; /* abbrev */

      start [0] = wfa->basis_states;
      if (wfa->wfainfo->color)
      {
	 start [1] = wfa->tree [wfa->tree[wfa->root_state][0]][0] + 1;
	 start [2] = wfa->tree [wfa->tree[wfa->root_state][0]][1] + 1;
	 start [3] = wfa->states;
      }
      else
	 start [1] = wfa->states;

      for (state = start [band]; state < start [1 + band]; state++)
	 for (label = 0; label < MAXLABELS; label++)
	    if (isedge (wfa->into [state][label][0])
		&& ischild (wfa->tree [state][label]))
	    {
	       if (draw)
		  draw_state_child (state, label, wfa, GC_ND,
				    display->click_areas [band], display);
	    }
	    else
	    {
	       int type;
	       
	       type = wfa->mv_tree [state][label].type;
	       if ((type == FORWARD && (motion & (1 << FORWARD)))
		   || (type == BACKWARD && (motion & (1 << BACKWARD)))
		   || (type == INTERPOLATED && (motion & (1 << INTERPOLATED))))
		  draw_mc (state, label, display->motion_display, wfa,
			   display->click_areas [band], display);
	    }
   }
}

void
draw_basis_images (const wfa_t *orig_wfa, xwfa_display_t *display)
/*
 *  Draw basis state images defined by the given WFA 'orig_wfa'.
 *
 *  No return value.
 */
{
   static GtkWidget	*table = NULL;	/* table for labels and images */
   int			state;		/* counter */
   wfa_t		*wfa;		/* copy of analysed WFA */
   
   if (table != NULL)
      gtk_widget_destroy (table);	/* discard old table */

   if (!orig_wfa)
      return;				/* nothing to do */

   wfa = alloc_wfa (NO);
   copy_wfa (wfa, orig_wfa);

   if (display->basis_image)
      Free (display->basis_image);
   display->basis_image = Calloc (wfa->basis_states, sizeof (GtkWidget *));

   /*
    *  Allocate table for images and labels
    */
   table = gtk_table_new (wfa->basis_states * 3, 1, FALSE);
   gtk_table_set_row_spacings (GTK_TABLE (table), 0);
   gtk_table_set_col_spacings (GTK_TABLE (table), 5);
   gtk_container_border_width (GTK_CONTAINER (table), 5);
   gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (display->basis_window),
					  table);
   gtk_widget_show (table);
   
   for (state = 0; state < wfa->basis_states; state++)
   {
      GtkWidget *border, *label, *preview, *sep;
      char	label_text [MAXSTRLEN];

      sprintf (label_text, "%d", state);
      label = gtk_label_new (label_text);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1,
			state * 3, state * 3 + 1, 0, GTK_EXPAND, 0, 0);
      gtk_widget_show (label);

      border = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (border), GTK_SHADOW_IN);
      gtk_container_border_width (GTK_CONTAINER (border), 0);
      gtk_table_attach (GTK_TABLE (table), border, 0, 1,
			state * 3 + 1, state * 3 + 2, 0, GTK_EXPAND, 0, 0);
      gtk_widget_show (border);

      /*
       *  Compute basis image
       */
      {
	 const int	display_level = 12; /* level of basis image */
	 image_t	basis_block;	/* basis image */
	 word_t		*domains [2];	/* domain blocks */

	 display->basis_image [state] = preview
				      = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
	 gtk_preview_size (GTK_PREVIEW (preview), 64, 64);
	 gtk_container_add (GTK_CONTAINER (border), preview);
	 gtk_widget_show (preview);
	 gtk_object_set_user_data (GTK_OBJECT (preview), (gpointer) state);
	 gtk_signal_connect_after (GTK_OBJECT (preview), "expose_event",
				   GTK_SIGNAL_FUNC (basis_event),
				   (gpointer) display);

	 remove_states (wfa->basis_states, wfa);
	 append_edge (wfa->basis_states, state, 1.0, 0, wfa);
	 wfa->states = wfa->basis_states + 1;
      
	 basis_block.color         = NO;
	 basis_block.width         = width_of_level (display_level);
	 basis_block.height        = height_of_level (display_level);
	 basis_block.format        = FORMAT_4_4_4;
	 basis_block.pixels [GRAY] = decode_range (wfa->basis_states, 0,
						   display_level, domains, wfa);
	 {
	    word_t	*src, *dst;
	    int		y;
	    
	    src = domains [0];
	    dst = basis_block.pixels [GRAY];
	    for (y = basis_block.height; y; y--)
	    {
	       memcpy (dst, src,
		       width_of_level (display_level) * sizeof (word_t));
	       src += width_of_level (display_level);
	       dst += basis_block.width;
	    }
	 }
	 draw_grayscale (GTK_PREVIEW (preview), &basis_block, GRAY);
	 Free (basis_block.pixels [GRAY]);
	 Free (domains [0]);
      }

      sep = gtk_hseparator_new ();
      gtk_widget_show (sep);
      gtk_table_attach (GTK_TABLE (table), sep, 0, 1,
			state * 3 + 2, state * 3 + 3,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 5);
      
   }

   free_wfa (wfa);
}

void
force_basis_redraw (int basis_states, xwfa_display_t *display)
/*
 *  Redraw basis image widgets of states 0, ... , 'basis_states'
 *
 *  No return value.
 */
{
   int state;

   for (state = 0; state < basis_states; state++)
      gtk_widget_draw (display->basis_image [state], NULL);
}

void
init_colors (GtkWidget *preview)
/*
 *  Initialize array of graphics contexts with different colors.
 *  Use 'preview' to obtain the color information.
 *
 *  No return value.
 *
 *  Side effects:
 *	local array 'gc_color []' is changed.
 */
{
   GdkColormap	*colormap;

   int color_values [GC_NULL][3] =
   {{255, 255,   0},			/* yellow */
    {255, 165,   0},			/* orange */
    {255,   0,   0},			/* red */
    {  0, 255, 255},			/* cyan */
    {  0, 255,   0},			/* green */
    {255, 192, 203},			/* pink */
    {100, 149, 237}			/* CornflowerBlue */
    };
   color_type_e gc;
   
   colormap = gdk_window_get_colormap (preview->window);

   for (gc = 0; gc < GC_NULL; gc++)
   {
      GdkColor	color;		

      color.red   = color_values [gc][0] << 8;
      color.green = color_values [gc][1] << 8;
      color.blue  = color_values [gc][2] << 8;

      gc_color [gc] = gdk_gc_new (preview->window);
      if (!gdk_color_alloc (colormap, &color))
	 gdk_color_white (colormap, &color);
      gdk_gc_set_foreground (gc_color [gc], &color);
   }
}

void
refresh_highlighting (int band, const wfa_t *wfa, xwfa_display_t *display)
/*
 *  Refresh highlighting of ranges and domains in given 'band' if required.
 *
 *  No return value.
 */
{
   int edge;

   if (band == range_band && range_state != -1 && range_label != -1)
   {
      draw_state_child (range_state, range_label, wfa, GC_RANGE,
			display->click_areas [band], display);

      range_x [0] = wfa->x [range_state][range_label];
      range_y [0] = wfa->y [range_state][range_label];
      range_l [0] = wfa->level_of_state [range_state] - 1;
      range_n [0] = band;
   }
   
   if (band == Y && domains)
      for (edge = 0; isedge (domains [edge]); edge++)
	 if (domains [edge] >= wfa->basis_states)
	 {
	    draw_state_child (domains [edge], -1, wfa, GC_DOMAIN,
			      display->click_areas [band], display);
	    
	    range_x [2 + edge] = wfa->x [domains [edge]][0];
	    range_y [2 + edge] = wfa->y [domains [edge]][0];
	    range_l [2 + edge] = wfa->level_of_state [domains [edge]];
	 }
   
   if (band == Y && prediction_state != -1 && prediction_label != -1)
   {
      if (ischild (wfa->tree [prediction_state][prediction_label])
	  && isedge (wfa->into [prediction_state][prediction_label][0]))
      {
	 draw_state_child (prediction_state, prediction_label, wfa, GC_ND,
			   display->click_areas [band], display);

	 range_x [1] = wfa->x [prediction_state][prediction_label];
	 range_y [1] = wfa->y [prediction_state][prediction_label];
	 range_l [1] = wfa->level_of_state [prediction_state] - 1;
      }
      else
      {
	 draw_mc (prediction_state, prediction_label, MO_RANGE, wfa,
		  display->click_areas [0], display);

	 range_x [1] = max (0, wfa->x [prediction_state][prediction_label] - 16);
	 range_y [1] = max (0, wfa->y [prediction_state][prediction_label] - 16);
	 range_l [1] = max (wfa->level_of_state [prediction_state], 12) ;
      }
   }
}

void
highlight (int state, int label, const bool_t *draw, const wfa_t *wfa,
	   xwfa_display_t *display)
/*
 *  Highlight range given by 'state' and 'label'.
 *  Array of draw [] flags indicate whether ranges, domains, and prediction
 *  should be highlighted.
 *
 *  No return value.
 */
{
   bool_t	preview_event = NO, basis_event = NO;
   char		text [MAXSTRLEN];
   bool_t	mc_changed = NO;
   
   /*
    *  Update entry widgets
    */
   sprintf (text, "%d", state);
   gtk_entry_set_text (GTK_ENTRY (display->status_widget [STAT_STATE]), text);
   sprintf (text, "%d", label);
   gtk_entry_set_text (GTK_ENTRY (display->status_widget [STAT_LABEL]), text);

   {
      int    pstate, plabel;
	 
      get_prediction (state, label, wfa, &pstate, &plabel);

      if (pstate != -1 && plabel != -1) /* found a prediction */
      {
	 if (prediction_state != pstate || prediction_label != plabel)
	 {
	    prediction_state = pstate;
	    prediction_label = plabel;
	    mc_changed	= YES;
	 }
      }
      else
      {
	 prediction_state = -1;
	 prediction_label = -1;
	 mc_changed	= YES;
      }
      
      if (mc_changed)
	 show_mc_coord (prediction_state, prediction_label, wfa, display);
   }
   
   if (draw [0] || draw [1] || draw [2])
   {
      if ((state != range_state || label != range_label)
	  && (range_state >= 0 && range_label >= 0))
      {
	 range_state = range_label = range_band = -1;
	 prediction_state = prediction_label = -1;
	 domains =  NULL;
      }
      /*
       *  Compute corresponding color band
       */
      {
	 int band;
	 
	 if (wfa->wfainfo->color)
	 {
	    if (state <= wfa->tree [wfa->tree [wfa->root_state][0]][0])
	       band = 0;
	    else if (state <= wfa->tree [wfa->tree [wfa->root_state][0]][1])
	       band = 1;
	    else
	       band = 2;
	 }
	 else
	    band = 0;
	 
	 /*
	  *  Force range image to be drawn
	  */
	 if (draw [0] && (state != range_state || label != range_label))
	 {
	    range_state   = state;
	    range_label   = label;
	    range_band    = band;
	    preview_event = YES;
	    basis_event   = YES;
	 }
      }
      /*
       *  Force domain images to be drawn
       */
      if (draw [1] && domains != wfa->into [state][label])
      {
	 domains       = wfa->into [state][label];
	 preview_event = YES;
	 basis_event   = YES;
      }
      /*
       *  Force prediction to be drawn
       */
      if (draw [2] && mc_changed)
	 preview_event = YES;
   }

   if (preview_event)
   {
      int n;

      for (n = 0; n < MAXEDGES + 2; n++)
	 if (range_x [n] != -1 && range_y [n] != -1 && range_l [n] != -1)
	    preview_restore_area (display->click_areas [n == 0 ? range_n [n] : 0],
				  range_x [n], range_y [n],
				  width_of_level (range_l [n]),
				  height_of_level (range_l [n]));
      refresh_highlighting (0, display->video->wfa, display);
      if (wfa->wfainfo->color)
      {
	 refresh_highlighting (1, display->video->wfa, display);
	 refresh_highlighting (2, display->video->wfa, display);
      }
   }
   if (basis_event)
      force_basis_redraw (wfa->basis_states, display);
}

void
get_prediction (int state, int label, const wfa_t *wfa, int *pstate, int *plabel)
/*
 *  Check if a parent of the given range (represented by 'state' and 'label')
 *  is predicted.
 *
 *  No return value.
 *
 *  Side effects:
 *	state and label of parent range will be copied to 'pstate' and 'plabel'
 *      on success, else they are filled with -1, -1.
 */
{
   int		max_state;
   int		x, y, s, l;

   *pstate = *plabel = -1;
   x = wfa->x [state][label];
   y = wfa->y [state][label];
   max_state = wfa->wfainfo->color
	       ? wfa->tree[wfa->tree [wfa->root_state][0]][0] + 1
	       : wfa->states;
	 
   for (s = state; s < max_state; s++)
      for (l = 0; l < MAXLABELS; l++)
	 if (wfa->level_of_state [s] >= wfa->level_of_state [state])
	    if (x >= wfa->x [s][l]
		&& x < (wfa->x [s][l]
			+ width_of_level (wfa->level_of_state [s] - 1))
		&& y >= wfa->y [s][l]
		&& y < (wfa->y [s][l]
			+ height_of_level (wfa->level_of_state [s] - 1)))
	    {
	       /*
		*  Found a parent of the range (state, label)
		*  Now check if a prediction has been used.
		*/
	       if (wfa->mv_tree [s][l].type != NONE
		   || (ischild (wfa->tree [s][l])
		       && isedge (wfa->into [s][l][0]))) /* prediction */
	       {
		  *pstate = s;
		  *plabel = l;
		  s = max_state;
		  break;
	       }
	    }
}

void
clear_current_range (void)
/*
 *  Clear information about current range.
 *  I.e. force recalculaton of current range.
 *
 *  No return value.
 *
 *  Side effects:
 *	local variables 'range_band', 'range_state', 'range_label',
 *		        'prediction_state', 'prediction_label', 'domains'
 *	are cleared.
 */
{
   range_band       = -1;
   range_state      = -1;
   range_label      = -1;
   {
      int n;

      for (n = 0; n < MAXEDGES + 2; n++)
      {
	 range_x [n] = -1;
	 range_y [n] = -1;
	 range_l [n] = -1;
	 range_n [n] = -1;
      }
   }
   prediction_state = -1;
   prediction_label = -1;
   domains          = NULL;
}

/*******************************************************************************

				private code
  
*******************************************************************************/

static void
highlight_basis_image (int state, GdkGC *gc, xwfa_display_t *display)
/*
 *  Highlight basis image 'state' with graphics context 'gc'.
 *
 *  No return value.
 */
{
   int x0, y0;

   x0 = (display->basis_image [state]->allocation.width
	 - GTK_PREVIEW (display->basis_image [state])->buffer_width) / 2;
   y0 = (display->basis_image [state]->allocation.height
	 - GTK_PREVIEW (display->basis_image [state])->buffer_height) / 2;
   
   gdk_draw_rectangle (display->basis_image [state]->window, gc, FALSE,
		       x0, y0, 63, 63);
}

static gint
basis_event (GtkWidget *widget, GdkEvent  *event, gpointer ptr)
/*
 *  Basis images expose event: redraw highlighting of basis images if required.
 *
 *  Return value:
 *	FALSE
 */
{
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   int			edge;
   int		state = (int) gtk_object_get_user_data (GTK_OBJECT (widget));
   
   switch (event->type)
   {
      case GDK_MAP:
      case GDK_EXPOSE:
	 if (domains)
	    for (edge = 0; isedge (domains [edge]); edge++)
	       if (domains [edge] == state)
		  highlight_basis_image (state, gc_color [GC_DOMAIN], display);
	 break;
      default:
	 break;
   }
   
   return FALSE;
}

void
draw_state_child (int state, int label, const wfa_t *wfa, color_type_e color,
		  GtkWidget *preview, xwfa_display_t *display)
/*
 *  Highlight a range block of 'wfa' given by 'state' and 'label' with 'color'.
 *  If 'state' is a basis state then use the basis image container else use
 *  the clickable area 'preview'.
 *  If 'label' == -1 highlight both childs of 'state'.
 *
 *  No return value.
 */
{
   int width, height;

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

   if (state < wfa->basis_states)
      highlight_basis_image (state, gc_color [color], display);
   else
   {
      if (label != -1)
	 preview_draw_rec (preview, color,
			   wfa->x [state][label], wfa->y [state][label],
			   min (width_of_level (wfa->level_of_state [state] - 1),
				width - wfa->x [state][label]) - 1,
			   min (height_of_level (wfa->level_of_state [state] - 1),
				height - wfa->y [state][label]) - 1);
      else
	 preview_draw_rec (preview, color,
			   wfa->x [state][0], wfa->y [state][0],
			   min (width_of_level (wfa->level_of_state [state]),
				width - wfa->x [state][0]) - 1,
			   min (height_of_level (wfa->level_of_state [state]),
				height - wfa->y [state][0]) - 1);
   }
}

static void
draw_mc (int state, int label, motion_e motion_display, const wfa_t *wfa,
	 GtkWidget *preview, xwfa_display_t *display)
/*
 *  Highlight motion compensated range block of 'wfa' given
 *  by 'state' and 'label'.
 *  Use the clickable area 'preview' for output.
 *  According to 'motion_display' highlight the range block, the reference block,
 *  both blocks, or show the motion vector.
 *
 *  No return value.
 */
{
   int	type;				/* motion compensation type */
   int  owidth, oheight;

   if (display->enlarge_factor > 0)
   {
      owidth  = display->wfa->wfainfo->width  << display->enlarge_factor;
      oheight = display->wfa->wfainfo->height << display->enlarge_factor; 
   }
   else
   { 
      owidth  = display->wfa->wfainfo->width  >> - display->enlarge_factor;
      oheight = display->wfa->wfainfo->height >> - display->enlarge_factor;
      if (owidth & 1)
	 owidth++;
      if (oheight & 1)
	 oheight++;
   }
   
   type = wfa->mv_tree [state][label].type;
   if (type == NONE)
      return;				/* nothing to do */

   
   if (motion_display == MO_VECTOR)
      draw_motion_vector (state, label, wfa, preview);
   else
   {
      if (motion_display == MO_RANGE || motion_display == MO_BOTH)
	 draw_state_child (state, label, wfa, type == FORWARD ? GC_FORWARD :
			   (type == BACKWARD ? GC_BACKWARD : GC_INTERPOLATED),
			   preview, display);

      if (motion_display == MO_REFERENCE || motion_display == MO_BOTH)
      {
	 GdkGCValues	gcval;
	 int		x, y;
	 int		width, height;
      
	 if (type == FORWARD || type == INTERPOLATED)
	 {
	    gdk_gc_get_values (gc_color [GC_FORWARD], &gcval);
	    gdk_gc_set_line_attributes (gc_color [GC_FORWARD], gcval.line_width,
					GDK_LINE_ON_OFF_DASH, gcval.cap_style,
					gcval.join_style);
	    x = wfa->mv_tree [state][label].fx
		/ (wfa->wfainfo->half_pixel ? 2 : 1) + wfa->x [state][label];
	    y = wfa->mv_tree [state][label].fy
		/ (wfa->wfainfo->half_pixel ? 2 : 1) + wfa->y [state][label];
	    width  = width_of_level (wfa->level_of_state [state] - 1);
	    height = height_of_level (wfa->level_of_state [state] - 1);
	    preview_draw_rec (preview, GC_FORWARD, x, y,
			      min (width,  owidth - x) - 1,
			      min (height, oheight - y) - 1);
	    gdk_gc_set_line_attributes (gc_color [GC_FORWARD], gcval.line_width,
					gcval.line_style, gcval.cap_style,
					gcval.join_style);
	 }
	 if (type == BACKWARD || type == INTERPOLATED)
	 {
	    gdk_gc_get_values (gc_color [GC_BACKWARD], &gcval);
	    gdk_gc_set_line_attributes (gc_color [GC_BACKWARD], gcval.line_width,
					GDK_LINE_ON_OFF_DASH, gcval.cap_style,
					gcval.join_style);
	    x = wfa->mv_tree [state][label].bx
		/ (wfa->wfainfo->half_pixel ? 2 : 1) + wfa->x [state][label];
	    y = wfa->mv_tree [state][label].by
		/ (wfa->wfainfo->half_pixel ? 2 : 1) + wfa->y [state][label];
	    width  = width_of_level (wfa->level_of_state [state] - 1);
	    height = height_of_level (wfa->level_of_state [state] - 1);
	    preview_draw_rec (preview, GC_BACKWARD, x, y,
			      min (width,  owidth - x) - 1,
			      min (height, oheight - y) - 1);
	    gdk_gc_set_line_attributes (gc_color [GC_BACKWARD], gcval.line_width,
					gcval.line_style, gcval.cap_style,
					gcval.join_style);
	 }
      }
   }
}

static void
draw_motion_vector (int state, int label, const wfa_t *wfa, GtkWidget *preview)
/*
 *  Draw the motion vector that has been used to predict the range block of 'wfa'
 *  given by 'state' and 'label'.
 *  Use the clickable area 'preview' for output.
 *
 *  No return value.
 */
{
   mc_type_e type = wfa->mv_tree [state][label].type;
   
   if (type != NONE)
   {
      int x0, y0;

      x0 = (preview->allocation.width
	    - GTK_PREVIEW (preview)->buffer_width) / 2;
      y0 = (preview->allocation.height
	    - GTK_PREVIEW (preview)->buffer_height) / 2;

      x0 += wfa->x [state][label]
	    + width_of_level (wfa->level_of_state [state] - 1) / 2;
      y0 += wfa->y [state][label]
	    + height_of_level (wfa->level_of_state [state] - 1) / 2;

      
      if (type == FORWARD || type == INTERPOLATED)
      {
	 gdk_draw_line (preview->window, gc_color [GC_FORWARD],
			x0, y0,
			x0 + (wfa->mv_tree [state][label].fx
			      / (wfa->wfainfo->half_pixel ? 2 : 1)),
			y0 + (wfa->mv_tree [state][label].fy
			      / (wfa->wfainfo->half_pixel ? 2 : 1)));
      }
      if (type == BACKWARD || type == INTERPOLATED)
      {
	 gdk_draw_line (preview->window, gc_color [GC_BACKWARD],
			x0, y0,
			x0 + (wfa->mv_tree [state][label].bx
			      / (wfa->wfainfo->half_pixel ? 2 : 1)),
			y0 + (wfa->mv_tree [state][label].by
			      / (wfa->wfainfo->half_pixel ? 2 : 1)));
      }
   }
}

void
preview_draw_rec (GtkWidget *preview, color_type_e color, int x0, int y0,
		  int width, int height)
/*
 *  Draw a rectangle into the clickable area 'preview' with 'color'
 *  given by the coordinates ('x0', 'y0') and ('width', 'height').
 *
 *  No return value.
 */
{
   gdk_draw_rectangle (preview->window, gc_color [color], 0, x0, y0,
		       width, height);
}

void
preview_restore_area (GtkWidget *preview, int x0, int y0, int width, int height)
{
   gtk_preview_put (GTK_PREVIEW (preview),
		    preview->window, preview->style->black_gc,
		    x0, y0, x0, y0, width, height);
}

static void
show_mc_coord (int prediction_state, int prediction_label, const wfa_t *wfa,
	       xwfa_display_t *display)
{
   char prediction_label_text [MAXSTRLEN], dummy [MAXSTRLEN];

   prediction_label_text [0] = 0;
   if (wfa->mv_tree [prediction_state][prediction_label].type == FORWARD
       || (wfa->mv_tree [prediction_state][prediction_label].type
	   == INTERPOLATED))
   {
      if (wfa->wfainfo->half_pixel)
	 sprintf (dummy, "(%.1f:%.1f),",
		  (double) wfa->mv_tree [prediction_state][prediction_label].fx
		  / 2.0,
		  (double) wfa->mv_tree [prediction_state][prediction_label].fy
		  / 2.0);
      else
	 sprintf (dummy, "(%d:%d),",
		  wfa->mv_tree [prediction_state][prediction_label].fx,
		  wfa->mv_tree [prediction_state][prediction_label].fy);
      strcat (prediction_label_text, dummy);
   }
   else
      strcat (prediction_label_text, "None,");
		     
   if (wfa->mv_tree [prediction_state][prediction_label].type == BACKWARD
       || (wfa->mv_tree [prediction_state][prediction_label].type
	   == INTERPOLATED))
   {
      if (wfa->wfainfo->half_pixel)
	 sprintf (dummy, "(%.1f:%.1f)",
		  (double) wfa->mv_tree [prediction_state][prediction_label].bx
		  / 2.0,
		  (double) wfa->mv_tree [prediction_state][prediction_label].by
		  / 2.0);
      else
	 sprintf (dummy, "(%d:%d)",
		  wfa->mv_tree [prediction_state][prediction_label].bx,
		  wfa->mv_tree [prediction_state][prediction_label].by);
      strcat (prediction_label_text, dummy);
   }
   else
      strcat (prediction_label_text, "None");
   gtk_label_set (GTK_LABEL (display->status_widget [STAT_MVEC]),
		  prediction_label_text);
}

