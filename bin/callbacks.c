/*
 *  callbacks.c:	Callback functions for XWFA	
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/15 17:22:30 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#include "config.h"

#include "types.h"
#include "macros.h"

#if STDC_HEADERS
#	include <stdlib.h>
#endif /* not STDC_HEADERS */

#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */

#include <ctype.h>

#include <gtk/gtk.h>

#include "fiasco.h"
#include "misc.h"
#include "decoder.h"
#include "wfalib.h"
#include "read.h"
#include "xwfa.h"
#include "dialog.h"
#include "background.h"
#include "drawing.h"
#include "view.h"
#include "callbacks.h"
#include "error.h"

/*****************************************************************************

			     local variables
  
*****************************************************************************/

static int ask_load_image = 0;		/*  0: ask user whether to pop up
					       image file browser
					    1: pop up image file browser
					   -1: don't bother user */

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
image_ok (GtkWidget *widget, gpointer ptr);
static void
wfa_ok (GtkWidget *widget, gpointer ptr);
static void
image_contents (xwfa_display_t *display, bool_t color, int width, int height);
static gint
preview_expose_event (GtkWidget *widget, GdkEvent  *event, gpointer ptr);
static gint
preview_button_event (GtkWidget *widget, GdkEvent  *event, gpointer ptr);
static gint
preview_motion_event (GtkWidget *widget, GdkEvent  *event, gpointer ptr);
static gint
preview_release_event (GtkWidget *widget, GdkEvent  *event, gpointer ptr);
static GtkWidget *
pulldown_menu (xwfa_display_t *display);
static void
toggle_highlighting (GtkWidget *widget, gpointer data);
static void
show_approximation (GtkWidget *widget, gpointer data);

/*****************************************************************************

				public code
  
*****************************************************************************/

void
wfa_file_selection (GtkWidget *widget, gpointer ptr)
/*
 *  Show WFA file selection popup window.
 *  If user selects a file then load_wfa will be called.
 *
 *  No return value.
 */
{
   static GtkWidget *filesel = NULL;
   xwfa_display_t   *display = (xwfa_display_t *) ptr;

   if (filesel)				/* file selection already activated */
   {
      gtk_widget_show (filesel);
      return;
   }

   filesel = gtk_file_selection_new ("Load WFA");
   gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (filesel));
   if (display->wfa_path != NULL)
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				       display->wfa_path);
   gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
   gtk_signal_connect_object (GTK_OBJECT (filesel), "delete_event",
			      GTK_SIGNAL_FUNC (hide_window),
			      GTK_OBJECT (filesel));
   gtk_signal_connect (GTK_OBJECT (filesel), "destroy",
		       GTK_SIGNAL_FUNC (destroy_window), &filesel);
   gtk_object_set_user_data (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
			     (gpointer) filesel);
   gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		       "clicked", GTK_SIGNAL_FUNC (wfa_ok), (gpointer) display);
   gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			      "clicked", GTK_SIGNAL_FUNC (hide_window),
			      GTK_OBJECT (filesel));
   gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
			      "clicked", GTK_SIGNAL_FUNC (hide_window),
			      GTK_OBJECT (filesel));
   gtk_widget_show (filesel);
}

void
image_file_selection (GtkWidget *widget, gpointer ptr)
/*
 *  Show WFA file selection popup window.
 *  If user selects a file then backgrounds will be updated.
 *
 *  No return value.
 */
{
   static GtkWidget *filesel = NULL;
   xwfa_display_t   *display = (xwfa_display_t *) ptr;
   
   if (display->video == NULL)
   {
      dialog_popup (DIALOG_INFO, "Please load a FIASCO file first.",
		    NULL, NULL);
      return;
   }
   if (filesel)				/* file selection already activated */
   {
      gtk_widget_show (filesel);
      return;
   }

   filesel = gtk_file_selection_new ("Load original image");
   gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (filesel));
   if (display->image_path != NULL)
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				       display->image_path);
   gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
   gtk_signal_connect_object (GTK_OBJECT (filesel), "delete_event",
			      GTK_SIGNAL_FUNC (hide_window),
			      GTK_OBJECT (filesel));
   gtk_signal_connect (GTK_OBJECT (filesel), "destroy",
		       GTK_SIGNAL_FUNC (destroy_window), &filesel);

   gtk_object_set_user_data (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
			     (gpointer) filesel);
   gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		       "clicked", GTK_SIGNAL_FUNC (image_ok), (gpointer) display);
   
   gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			      "clicked", GTK_SIGNAL_FUNC (hide_window),
			      GTK_OBJECT (filesel));
   gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
			      "clicked", GTK_SIGNAL_FUNC (hide_window),
			      GTK_OBJECT (filesel));
   gtk_widget_show (filesel);
}

void
load_next_frame (GtkWidget *widget, gpointer ptr)
/*
 *  Load next frame of WFA. Update status bar, compute backgrounds, ...
 *
 *  No return value.
 */
{
   xwfa_display_t *display = (xwfa_display_t *) ptr;
   int		   frames  = 1;

   if (widget)
      frames = (int) gtk_object_get_user_data (GTK_OBJECT (widget));
   
   while (frames-- && display->frame_n++ < display->wfa->wfainfo->frames)
   {
      clear_current_range ();
      try {
	 get_next_frame (YES, display->enlarge_factor,
			 display->smoothing_factor,
			 NULL, FORMAT_4_4_4, display->video, NULL,
			 display->wfa, display->input);
      }
      catch {
	 dialog_popup (DIALOG_ERROR, fiasco_get_error_message (), NULL, NULL);
	 return;
      }
   }
   {
      static int bits;
      int	 n;
      
      if (display->frame_n == 1)	/* first frame */
	 bits = 0;

      /*
       *  Check if lc prediction has been used
       */
      {
	 bool_t	nd_prediction = NO;
	 bool_t	dc_prediction = NO;
	 int	state, label;
	 wfa_t	*wfa = display->video->wfa;
	 
	 for (state = wfa->basis_states; state < wfa->states; state++)
	    for (label = 0; label < MAXLABELS; label++)
	       if (isedge (wfa->into [state][label][0])
		   && ischild (wfa->tree [state][label]))
	       {
		  nd_prediction = YES;
	       }
	       else if (wfa->prediction [state][label])
		  dc_prediction = YES;
	 
	 display->lc_prediction = nd_prediction;
      }
     

      /*
       *  Update of status bar
       */
      {
	 char	text [MAXSTRLEN];

	 if (display->video->wfa->frame_type == I_FRAME)
	    gtk_label_set (GTK_LABEL (display->status_widget [STAT_MVEC]),
			   "None");
	 sprintf (text, "%d", display->frame_n);
	 gtk_entry_set_text (GTK_ENTRY (display->status_widget [STAT_FNO]),
			     text);
	 sprintf (text, "%d", display->video->wfa->wfainfo->frames);
	 gtk_label_set (GTK_LABEL (display->status_widget [STAT_POS]),
			"(0, 0)");
	 gtk_label_set (GTK_LABEL (display->status_widget [STAT_FRAMES]),
			text);
	 gtk_label_set (GTK_LABEL (display->status_widget [STAT_TYPE]),
			display->video->wfa->frame_type == I_FRAME ? "Intra"
			: (display->video->wfa->frame_type == P_FRAME
			   ? "Predicted" : "Bidirectional"));
	 display->bpp = (display->input->bits_processed - bits)
			/ (display->video->wfa->wfainfo->width
			   * (real_t) display->video->wfa->wfainfo->height);
	 sprintf (text, "%5.3fbpp", (double) display->bpp);
	 gtk_label_set (GTK_LABEL (display->status_widget [STAT_RATE]), text);
	 sprintf (text, "%d", display->video->wfa->states);
	 gtk_label_set (GTK_LABEL (display->status_widget [STAT_STATES]),
			text);
      }
      bits = display->input->bits_processed;

      generate_wfa_backgrounds (display);
      /*
       *  If last original frame was loaded then also load new original frame
       */
      if (display->bg_image [BG_ORIGINAL])
      {
	 char	  *name;
	 long int  number;
	 
	 name = strrchr (display->image_path, '/');
	 if (!name)
	    name = display->image_path;

	 while (*name && !isdigit (*name))
	    name++;

	 if (*name)
	 {
	    number = strtol (name, NULL, 10);
	    if (number == LONG_MIN || number == LONG_MAX)
	       warning ("Can't load next image.");
	    else
	    {
	       char text [MAXSTRLEN];
	       
	       number++;
	       if (number + 1 != display->frame_n
		   && number != display->frame_n)
		  dialog_popup (DIALOG_WARNING, "WFA frame number doesn't\n"
				"match image frame number.", NULL, NULL);
	       for (n = 0; *name && isdigit (*name); n++, name++)
		  ;
	       sprintf (text, "%0*ld", n, number);
	       while (n)
		  *--name = text [--n];
	    }
	 }
	 try {
	    generate_image_backgrounds (display->image_path,
					display->bg_image);
	 }
	 catch {
	    dialog_popup (DIALOG_ERROR, fiasco_get_error_message (),
			  NULL, NULL);
	 }
      }
      background_sensitive (display);
      prediction_sensitive (display);

      for (n = 0; n < (display->video->wfa->wfainfo->color ? 4 : 1); n++)
      {
	 if (n < 3)
	    gdk_window_set_cursor (display->click_areas [n]->window,
				   gdk_cursor_new (GDK_HAND1));
	 draw_background (display->background, display->bg_image, n,
			  display->click_areas [n]);
      }
      gtk_widget_draw (display->root_window, NULL);
      gtk_widget_set_sensitive (display->next_frame_menu_item,
				display->frame_n
				< display->wfa->wfainfo->frames
				? YES : NO);
      gtk_widget_set_sensitive (display->prev_frame_menu_item,
				display->frame_n > 1 ? YES : NO);
      gtk_widget_set_sensitive (display->next_frame_button,
				display->frame_n
				< display->wfa->wfainfo->frames
				? YES : NO);
      gtk_widget_set_sensitive (display->prev_frame_button,
				display->frame_n > 1 ? YES : NO);
      if (display->hl_button [2])
	 gtk_widget_set_sensitive (display->hl_button [2],
				   display->lc_prediction
				   || display->video->wfa->frame_type != I_FRAME);
   }
}

void
clear_display (GtkWidget *widget, gpointer ptr)
/*
 *  Clear highlighted ranges and domain in the clickable areas
 *
 *  No return value.
 */
{
   int		   n;
   xwfa_display_t *display = (xwfa_display_t *) ptr;
   
   clear_current_range ();

   if (display->video)
      force_basis_redraw (display->video->wfa->basis_states, display);

   for (n = 0; n < 4; n++)
      if (display->click_areas [n])
	 gtk_widget_draw (display->click_areas [n], NULL);
}

gint
goto_range (GtkWidget *widget, gpointer ptr)
/*
 *  Highlight range given by the entry widget 'range' and 'label'
 *  and show linear combination (i.e. simulate a mouse click).
 *
 *  No return value.
 */
{
   xwfa_display_t *display 	= (xwfa_display_t *) ptr;
   int		   state, label;
   bool_t	   mark [] = {YES, YES, YES};
      
   state = strtol (gtk_entry_get_text (GTK_ENTRY (display->status_widget [STAT_STATE])), NULL, 10);
   label = strtol (gtk_entry_get_text (GTK_ENTRY (display->status_widget [STAT_LABEL])), NULL, 10);

   if (state < display->video->wfa->basis_states)
      state = display->video->wfa->basis_states;
   else if (state >= display->video->wfa->states)
      state = display->video->wfa->states - 1;
   if (label < 0)
      label = 0;
   else if (label > MAXLABELS - 1)
      label = MAXLABELS - 1;
   {
      char text [MAXSTRLEN];
	 
      sprintf (text, "%d", state);
      gtk_entry_set_text (GTK_ENTRY (display->status_widget [STAT_STATE]),
			  text);
      sprintf (text, "%d", label);
      gtk_entry_set_text (GTK_ENTRY (display->status_widget [STAT_LABEL]),
			  text);
   }
   draw_lc_components (state, label, YES, display->video->wfa, display);
   highlight (state, label, mark, display->video->wfa, display);

   return FALSE;
}

void
load_wfa (GtkWidget *w, gpointer ptr)
/*
 *  Initialize decoding of new WFA stream.
 *  Change root window title, compute basis images and
 *  allocate clickable areas.
 *
 *  No return value.
 */
{
   xwfa_display_t *display = (xwfa_display_t *) ptr;
   background_e	   bg;
   
   if (display->video)
      free_video (display->video);
   display->video = alloc_video (YES);

   if (display->wfa)
      free_wfa (display->wfa);
   display->wfa = NULL;
   
   for (bg = 0; bg < BG_NULL; bg++)
      switch (bg)
      {
	 case BG_DELTA:
	 case BG_PREDICTION:
	 case BG_WFA:
	 case BG_ORIGINAL:
	 case BG_DIFF:
	    if (display->bg_image [bg])
	    {
	       free_image (display->bg_image [bg]);
	       display->bg_image [bg] = NULL;
	    }
	    break;
	 default:
	    break;
      }
   display->wfa = alloc_wfa (NO);

   try {
      display->input = open_wfa (display->wfa_path, display->wfa->wfainfo);
      read_basis (display->wfa->wfainfo->basis_name, display->wfa);
   }
   catch {
      free_wfa (display->wfa);
      Free (display->video);
      display->wfa           = NULL;
      display->video         = NULL;
      display->input         = NULL;
      dialog_popup (DIALOG_ERROR, fiasco_get_error_message (), NULL, NULL);
   }

   /*
    *  Change title of root window
    */
   if (display->wfa)
   {
      char *title;
      char *text;
      
      text  = strrchr (display->wfa->wfainfo->wfa_name, '/');
      title = Calloc (strlen ("xwfa : ") + strlen (VERSION) + 5 +
		      + strlen (display->wfa->wfainfo->wfa_name), sizeof (char));
      sprintf (title, "%s %s: %s", "xfiasco", VERSION,
	       text ? text + 1 : display->wfa->wfainfo->wfa_name);
      gtk_window_set_title (GTK_WINDOW (display->root_window), title);
      Free (title);
   }

   draw_basis_images (display->wfa, display);

   if (display->wfa)
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

      image_contents (display, display->wfa->wfainfo->color, width, height);
   
      display->frame_n = 0;

      init_colors (display->click_areas [0]);
      gtk_widget_set_sensitive (display->load_image_menu_item, YES);
#ifdef XFIG
      gtk_widget_set_sensitive (display->twfa_menu, YES);
#endif /* XFIG */

      load_next_frame (NULL, display);
      clear_display (NULL, display);
   }
   else
   {
      gtk_widget_set_sensitive (display->load_image_menu_item, FALSE);
#ifdef XFIG
      gtk_widget_set_sensitive (display->twfa_menu, FALSE);
#endif /* XFIG */

      image_contents (NULL, NO, -1, -1);
   }
}

void
goto_frame (GtkWidget *widget, gpointer ptr)
/*
 *  Goto frame number given in entry STAT_FNO.
 */
{
   xwfa_display_t *display = (xwfa_display_t *) ptr;
   gpointer 	  *tmp;
   int 		   frame   = strtol (gtk_entry_get_text (GTK_ENTRY (display->status_widget [STAT_FNO])), NULL, 10);

   if (!display->wfa)
      return;
   if (frame < 1)
      frame = 1;
   if (frame > display->wfa->wfainfo->frames)
      frame = display->wfa->wfainfo->frames;
   {
      char text [MAXSTRLEN];
      sprintf (text, "%d", frame);
      gtk_entry_set_text (GTK_ENTRY (display->status_widget [STAT_FNO]), text);
   }
   
   if (frame == display->frame_n)
      return;
   
   tmp = gtk_object_get_user_data (GTK_OBJECT (widget));

   if (frame < display->frame_n)
   {
      load_wfa (widget, ptr);
      frame--;
   }
   else
      frame = frame - display->frame_n;
   gtk_object_set_user_data (GTK_OBJECT (widget), (gpointer) frame);
   load_next_frame (widget, ptr);

   gtk_object_set_user_data (GTK_OBJECT (widget), tmp);
}

void
next_frame (GtkWidget *widget, gpointer ptr)
{
   xwfa_display_t *display = (xwfa_display_t *) ptr;
   gpointer *tmp;

   if (!display->wfa)
      return;
   if (display->frame_n >= display->wfa->wfainfo->frames)
      return;
   
   tmp = gtk_object_get_user_data (GTK_OBJECT (widget));

   gtk_object_set_user_data (GTK_OBJECT (widget), (gpointer) 1);
   load_next_frame (widget, ptr);

   gtk_object_set_user_data (GTK_OBJECT (widget), tmp);
}

void
prev_frame (GtkWidget *widget, gpointer ptr)
{
   xwfa_display_t *display = (xwfa_display_t *) ptr;
   gpointer *tmp;
   int		frame;
   
   if (!display->wfa)
      return;
   if (display->frame_n <= 1)
      return;
   
   tmp 	 = gtk_object_get_user_data (GTK_OBJECT (widget));
   frame = display->frame_n;
   load_wfa (widget, ptr);
   if (frame - 2)
   {
      gtk_object_set_user_data (GTK_OBJECT (widget), (gpointer) (frame - 2));
      load_next_frame (widget, ptr);
   }

   gtk_object_set_user_data (GTK_OBJECT (widget), tmp);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
wfa_ok (GtkWidget *widget, gpointer ptr)
/*
 *  Called after WFA has been selected.
 *  Load first frame of selected WFA stream.
 *  Ask user whether to load corresponding original image_ok.
 *
 *  No return value.
 */
{
   xwfa_display_t   *display = (xwfa_display_t *) ptr;
   GtkFileSelection *fs      = gtk_object_get_user_data (GTK_OBJECT (widget));
   
   if (display->wfa_path)
      Free (display->wfa_path);
   display->wfa_path = strdup (gtk_file_selection_get_filename (fs));

   load_wfa (NULL, display);

   if (!display->wfa)
      return;				/* nothing to do */
   
   if (ask_load_image == 0)
   {
      ask_load_image = -1;
      dialog_popup (DIALOG_QUESTION, "Load corresponding original image?",
		    image_file_selection, (gpointer) display);
   }
   else if (ask_load_image > 0)
      image_file_selection (NULL, display);
}

static void
image_ok (GtkWidget *widget, gpointer ptr)
/*
 *  Called after image has been selected. Load original image and compute
 *  new background images 'original' and 'difference'.
 *
 *  No return value.
 */
{
   xwfa_display_t   *display = (xwfa_display_t *) ptr;
   GtkFileSelection *fs      = gtk_object_get_user_data (GTK_OBJECT (widget));
   
   ask_load_image = 1;
   
   if (display->image_path)
      Free (display->image_path);
   display->image_path = strdup (gtk_file_selection_get_filename (fs));

   try {
      generate_image_backgrounds (display->image_path, display->bg_image);
   }
   catch {
      dialog_popup (DIALOG_ERROR, fiasco_get_error_message (), NULL, NULL);
   }
   background_sensitive (display);
   prediction_sensitive (display);
}

static void
image_contents (xwfa_display_t *display, bool_t color, int width, int height)
/*
 *  Generate a new table to display the clickable areas and allocate
 *  preview widgets to hold the color bands.
 *
 *  No return value.
 */
{
   static GtkWidget *table = NULL;	/* container for clickable areas */
   int		     n;
   
   if (table)
      gtk_widget_destroy (table);

   if (!display)
      return;				/* nothing to do */

   if (color)
      table = gtk_table_new (2, 2, TRUE);
   else
      table = gtk_table_new (1, 1, TRUE);

   gtk_table_set_row_spacings (GTK_TABLE (table), 5);
   gtk_table_set_col_spacings (GTK_TABLE (table), 5);
   gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (display->image_window),
					  table);
   for (n = 0; n < (color ? 4 : 1); n++)
   {
      display->click_areas [n]
	 = gtk_preview_new (n < 3 ? GTK_PREVIEW_GRAYSCALE : GTK_PREVIEW_COLOR);
      if (n < 3)
      {
	 gint old_mask;
      
	 gtk_signal_connect_after (GTK_OBJECT (display->click_areas [n]),
				   "expose_event",
				   GTK_SIGNAL_FUNC (preview_expose_event),
				   (gpointer) display);
	 gtk_signal_connect (GTK_OBJECT (display->click_areas [n]),
			     "button_press_event",
			     GTK_SIGNAL_FUNC (preview_button_event),
			     (gpointer) display);
	 gtk_signal_connect (GTK_OBJECT (display->click_areas [n]),
			     "button_release_event",
			     GTK_SIGNAL_FUNC (preview_release_event),
			     (gpointer) display);
	 gtk_signal_connect (GTK_OBJECT (display->click_areas [n]),
			     "motion_notify_event",
			     GTK_SIGNAL_FUNC (preview_motion_event),
			     (gpointer) display);
	 old_mask = gtk_widget_get_events (display->click_areas [n]);
	 gtk_widget_set_events (display->click_areas [n], old_mask
				| GDK_BUTTON_PRESS_MASK
				| GDK_BUTTON_RELEASE_MASK
				| GDK_POINTER_MOTION_MASK
				| GDK_POINTER_MOTION_HINT_MASK);
      }
      gtk_preview_size (GTK_PREVIEW (display->click_areas [n]), width, height);
      gtk_table_attach (GTK_TABLE (table), display->click_areas [n],
			n < 2 ? 0 : 1, n < 2 ? 1 : 2,
			n & 1 ? 1 : 0, n & 1 ? 2 : 1,
			GTK_FILL | GTK_EXPAND | GTK_SHRINK,
			GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0);
      gtk_widget_show (display->click_areas [n]);
   }
   for (; n < 4; n++)
      display->click_areas [n] = NULL;
   
   {
      gint max_width;
      gint max_height;

      max_width = gdk_screen_width ()
		  - display->basis_window->requisition.width;
      max_height = gdk_screen_height ()
		   - display->root_window->requisition.height
		   + display->image_window->requisition.height;
      gtk_widget_set_usize (display->image_window,
			    min (max_width - 100,
				 (width << (color ? 1 : 0))) + 40,
			    min (max_height - 80,
				 (height << (color ? 1 : 0))) + 40);
   }
   
   gtk_widget_show (table);
}

static gint
preview_expose_event (GtkWidget *widget, GdkEvent  *event, gpointer ptr)
/*
 *  Redraw partitioning and refresh range highlighting if clickable areas
 *  receive a expose event.
 *
 *  Return value:
 *	FALSE
 */
{
   xwfa_display_t *display = (xwfa_display_t *) ptr;
   int		   n;			/* clicked area */
   
   for (n = 0; n < 4; n++)
      if (widget == display->click_areas [n])
	 break;

   if (n == 4)
      return FALSE;			/* area not found */
   
   draw_ranges (display->show_grid [GRID_RANGE], n, display);
   draw_nd_prediction (display->show_grid [GRID_ND],
		       (display->show_grid [GRID_FORWARD] << FORWARD) |
		       (display->show_grid [GRID_BACKWARD] << BACKWARD)
		       | (display->show_grid [GRID_INTERPOLATED] << INTERPOLATED),
		       n, display);

   if (!display->region_active && !display->region_set)
      refresh_highlighting (n, display->video->wfa, display);
   else if (display->root_state > 0 && display->region_band == n)
   {
      draw_state_child (display->root_state, 0, display->video->wfa, GC_RANGE,
			display->click_areas [n], display);
      draw_state_child (display->root_state, 1, display->video->wfa, GC_RANGE,
			display->click_areas [n], display);
      preview_draw_rec (display->click_areas [n], GC_MARKER,
			min (display->mx1, display->mx0),
			min (display->my1, display->my0),
			abs (display->mx1 - display->mx0),
			abs (display->my1 - display->my0));
   }
   
   return FALSE;
}

static int click_state = -1;
static int click_label = -1;

static gint
preview_button_event (GtkWidget *widget, GdkEvent  *event, gpointer ptr)
/*
 *  Check for button events.
 *	left button:  range highlighting and display of linear combination.
 *	middle button: mark rectangular region
 *	right button: popup menu
 *
 *  Return value:
 *	FALSE	if event is out of range
 *	TRUE	on success
 */
{
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   GdkEventButton	*bevent = (GdkEventButton *) event;
   gint			x, y;
   int			n;		/* clicked area */
   int			 width, height;

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

   for (n = 0; n < 4; n++)
      if (widget == display->click_areas [n])
	 break;

   if (n >= 3)
      return FALSE;			/* area not found */

   gdk_window_get_pointer (display->click_areas [n]->window, &x, &y, NULL);
   x -= (display->click_areas [n]->allocation.width
	 - GTK_PREVIEW (display->click_areas [n])->buffer_width) / 2;
   y -= (display->click_areas [n]->allocation.height
	 - GTK_PREVIEW (display->click_areas [n])->buffer_height) / 2;
   if (x >= 0 && x < width && y >=0   && y < height)
   {
      unsigned state, label;

#ifdef XFIG
      if (bevent->button == 2)
      {
	 display->region_active = YES;
	 display->mx0           = x;
	 display->my0           = y;
	 display->mx1           = x;
	 display->my1           = y;
	 display->region_band	= n;
	 display->root_state	= -1;
	 for (n = 0; n < GRID_NULL; n++)
	 {
	    display->show_grid [n] = NO;
	    if (display->pred_button [n])
	       gtk_widget_set_sensitive (display->pred_button [n], FALSE);
	 }
	 
	 clear_current_range ();
	 clear_display (NULL, display);
      }
      else
#endif /* XFIG */
	 if (find_range (x, y, n, display->video->wfa, &state, &label))
      {
#ifdef XFIG
	 if (display->region_set || display->region_active)
	    twfa_flush (NULL, display);
#endif /* XFIG */
	 
	 if (bevent->button == 1)
	 {
	    bool_t	mark[] = {YES, YES, YES};
	 
	    draw_lc_components (state, label, bevent->button == 1,
				display->video->wfa, display);
	    {
	       int	n;
	       bool_t	refresh = YES;
	       
	       for (n = 0; n < GRID_NULL; n++)
		  if (display->show_grid [n])
		  {
		     refresh = NO;
		  }
	       
	       if (refresh)
		  highlight (state, label, mark, display->video->wfa, display);
	    }
	    
	    return TRUE;
	 }
	 else if (bevent->button == 3)
	 {
	    click_state = state;
	    click_label = label;
	    gtk_menu_popup (GTK_MENU (pulldown_menu (display)), NULL, NULL,
			    NULL, NULL, 3, bevent->time);
	    return FALSE;
	 }
      }
   }
#ifdef XFIG
   else
      twfa_flush (NULL, display);
#endif /* XFIG */
   
   return TRUE;
}

static gint
preview_release_event (GtkWidget *widget, GdkEvent  *event, gpointer ptr)
/*
 *  Check for button release events.
 *
 *  Return value:
 *	FALSE	if event is out of range
 *	TRUE	on success
 */
{
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   GdkEventButton	*bevent = (GdkEventButton *) event;
   gint			x, y;
   int			n;		/* clicked area */
   int			 width, height;

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

   for (n = 0; n < 4; n++)
      if (widget == display->click_areas [n])
	 break;

   if (n >= 3)
      return FALSE;			/* area not found */

   gdk_window_get_pointer (display->click_areas [n]->window, &x, &y, NULL);
   x -= (display->click_areas [n]->allocation.width
	 - GTK_PREVIEW (display->click_areas [n])->buffer_width) / 2;
   y -= (display->click_areas [n]->allocation.height
	 - GTK_PREVIEW (display->click_areas [n])->buffer_height) / 2;

#ifdef XFIG
   if (bevent->button == 2)
   {
      display->region_active = NO;
      if ((display->mx0 == display->mx1 && display->my0 == display->my1)
	  || n != display->region_band)
      {
	 twfa_flush (NULL, display);
      }
      else
      {
	 int	state;
	 wfa_t	*wfa = display->video->wfa;
	 int	start [4];		/* start states of each color band */

	 start [0] = wfa->basis_states;
	 if (wfa->wfainfo->color)
	 {
	    start [1] = wfa->tree[wfa->tree[wfa->root_state][0]][0] + 1;
	    start [2] = wfa->tree[wfa->tree[wfa->root_state][0]][1] + 1;
	    start [3] = wfa->states;
	 }
	 else
	    start [1] = wfa->states;
	 display->region_set = YES;
	 
	 for (state = start [1 + display->region_band];
	      state >= start [display->region_band]; state--)
	    if (min (display->mx0, display->mx1) >= wfa->x [state][0]
		&& min (display->my0, display->my1) >= wfa->y [state][0]
		&& max  (display->mx0, display->mx1) < wfa->x [state][0] + width_of_level (wfa->level_of_state [state])
		&& max  (display->my0, display->my1) < wfa->y [state][0] + height_of_level (wfa->level_of_state [state]))
	    {
	       display->root_state = state;
	    }
	 
	 twfa_settings (NULL, display);
	 {
	    char tmp [MAXSTRLEN];

	    sprintf (tmp, "%d", display->root_state);
	    gtk_entry_set_text (GTK_ENTRY (display->twfa [TWFA_ROOT]), tmp);
	 }
	 
	 draw_state_child (display->root_state, 0, wfa, GC_RANGE,
			   display->click_areas [n], display);
	 draw_state_child (display->root_state, 1, wfa, GC_RANGE,
			   display->click_areas [n], display);
      }
   }
#endif /* XFIG */
   
   return TRUE;
}

static gint
preview_motion_event (GtkWidget *widget, GdkEvent  *event, gpointer ptr)
/*
 *  Check for mouse motion events. Update highlighting if neccessary.
 *
 *  Return value:
 *	FALSE	if event is out of range
 *	TRUE	on success
 */
{
   xwfa_display_t *display = (xwfa_display_t *) ptr;
   gint		   x, y;
   unsigned	   state, label;
   int		   n;			/* clicked area */
   int 		   width, height;

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
   
   for (n = 0; n < 4; n++)
      if (widget == display->click_areas [n])
	 break;

   if (n == 4)
      return FALSE;			/* area not found */

   gdk_window_get_pointer (display->click_areas [n]->window, &x, &y, NULL);
   x -= (display->click_areas [n]->allocation.width
	 - GTK_PREVIEW (display->click_areas [n])->buffer_width) / 2;
   y -= (display->click_areas [n]->allocation.height
	 - GTK_PREVIEW (display->click_areas [n])->buffer_height) / 2;

   if (x >= 0 && x < width && y >= 0  && y < height)
   {
      char text [MAXSTRLEN];

      sprintf (text, "(%d, %d)", x, y);
      gtk_label_set (GTK_LABEL (display->status_widget [STAT_POS]), text);
      if (display->region_active)
      {
	 if (n == display->region_band)
	 {
	    preview_restore_area (display->click_areas [n],
				  min (display->mx1, display->mx0),
				  min (display->my0, display->my1),
				  abs (display->mx1 - display->mx0) + 1,
				  abs (display->my1 - display->my0) + 1);
	    preview_draw_rec (display->click_areas [n], GC_MARKER,
			      min (x, display->mx0), min (y, display->my0),
			      abs (x - display->mx0), abs (y - display->my0));
	    display->mx1 = x;
	    display->my1 = y;
	 }
#ifdef XFIG
	 else
	    twfa_flush (NULL, display);
#endif /* XFIG */
      }
      else if (!display->region_set
	       && find_range (x, y, n, display->video->wfa, &state, &label))
	 highlight (state, label, display->automatic_highlighting,
		    display->video->wfa, display);
   }
   
   return TRUE;
}

static GtkWidget *
pulldown_menu (xwfa_display_t *display)
/*
 *  Generate a popup menu with shortcuts.
 *
 *  Return value:
 *	pointer to popup menu widget (still needs to get mapped)
 */
{
   char	*text[] = {"Show approximation (weighted domains)",
		   "Show approximation (original domains)",
		   "Show prediction (weighted domains)",
		   "Show prediction (original domains)", "-",
		   "Previous frame", "Next frame", "Toggle highlighting",
		   "Clear display",
		   "-", "Display options",
#ifdef XFIG  
		   "WFA bintree",
#endif /* XFIG */		   
		   NULL};
   void	(*callback []) (GtkWidget *widget, gpointer data)
      = {show_approximation, show_approximation, show_approximation,
	 show_approximation, NULL, prev_frame, next_frame, toggle_highlighting,
	 clear_display, NULL, display_settings,
#ifdef XFIG  
	 twfa_settings,
#endif /* XFIG */		   
	 NULL};
   GtkWidget	*menu;
   int		n;
   bool_t	hl = YES, nd;
   int		pstate, plabel;
   
   get_prediction (click_state, click_label, display->video->wfa,
		   &pstate, &plabel);

   nd = pstate != -1 && plabel != -1 && display->lc_prediction;
   for (n = 0; n < GRID_NULL; n++)
      if (display->show_grid [n])
	 hl = NO;
   
   menu = gtk_menu_new ();

   for (n = 0; text [n]; n++)
   {
      GtkWidget *menu_item;

      if ((strneq (text [n], "Next frame")
	   || display->frame_n < display->video->wfa->wfainfo->frames)
	  &&
	  (strneq (text [n], "Previous frame") || display->frame_n > 1)
	  &&
	  (strneq (text [n], "Toggle highlighting") || hl)
	  &&
	  (strneq (text [n], "Show prediction (weighted domains)") || nd)
	  &&
	  (strneq (text [n], "Show prediction (original domains)") || nd))
      {
	 if (streq (text [n], "-"))	/* separator */
	    menu_item = gtk_menu_item_new ();
	 else 
	    menu_item = gtk_menu_item_new_with_label (text [n]);
	 gtk_menu_append (GTK_MENU (menu), menu_item);
	 gtk_widget_show (menu_item);
	 if (callback [n])
	 {
	    gtk_object_set_user_data (GTK_OBJECT(menu_item), (gpointer) n);
	    gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
				GTK_SIGNAL_FUNC (callback [n]),
				(gpointer) display);
	 }
      } 
   }

   return menu;
}

static void
show_approximation (GtkWidget *widget, gpointer ptr)
/*
 *  Display the linear combination components of the clicked range.
 *  According to the data associated with 'widget' show lc/prediction
 *  with or without weighting.
 *
 *  No return value.
 */
{
   bool_t		mark [] = {YES, YES, YES};
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   int data = (int) gtk_object_get_user_data (GTK_OBJECT (widget));
   
   if (data < 2)
      draw_lc_components (click_state, click_label, data % 2 ? NO : YES,
			  display->video->wfa, display);
   else
   {
      int pstate, plabel;
      
      get_prediction (click_state, click_label, display->video->wfa, &pstate,
		      &plabel);
      if (pstate != -1 && plabel != -1)
	 draw_lc_components (pstate, plabel, data % 2 ? NO : YES,
			     display->video->wfa, display);
   }
   
   {
      int	n;
      bool_t	refresh = YES;
	       
      for (n = 0; n < GRID_NULL; n++)
	 if (display->show_grid [n])
	    refresh = NO;
      if (refresh)
	 highlight (click_state, click_label, mark, display->video->wfa, display);
   }
}

static void
toggle_highlighting (GtkWidget *widget, gpointer ptr)
/*
 *  Toggle automatic highlighting of ranges and domains.
 *
 *  No return value.
 */
{
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   
   if (display->automatic_highlighting [0])
      display->automatic_highlighting [0] = display->automatic_highlighting [1]
					  = display->automatic_highlighting [2]
					  = NO;
   else
      display->automatic_highlighting [0] = display->automatic_highlighting [1]
					  = display->automatic_highlighting [2]
					  = YES;

   if (display->hl_button [0])
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->hl_button [0]),
				   display->automatic_highlighting [0]);
   if (display->hl_button [1])
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->hl_button [1]),
				   display->automatic_highlighting [1]);
   if (display->hl_button [2])
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->hl_button [2]),
				   display->automatic_highlighting [2]);
   
   clear_display (NULL, display);
}

