/*
 *  view.c:		Menu view: Display options	
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/15 17:20:59 $
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

#include <gtk/gtk.h>

#include "misc.h"
#include "wfalib.h"
#include "xwfa.h"
#include "dialog.h"
#include "callbacks.h"
#include "background.h"
#include "drawing.h"
#include "view.h"
#include "error.h"

/*****************************************************************************

			     local variables
  
*****************************************************************************/


/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
hide_unhide_window (GtkWidget *widget, gpointer ptr);
static GtkWidget *
partitoning_settings (xwfa_display_t *display);
static void
smoothing_update (GtkWidget *widget, gpointer data);
static GtkWidget *
decoder_settings (xwfa_display_t *display);
static GtkWidget *
background_settings (xwfa_display_t *display);
static GtkWidget *
preferences_settings (xwfa_display_t *display);
static void
background_toggle (GtkWidget *widget, gpointer data);
static GtkWidget *
highlighting_settings (xwfa_display_t *display);
static void
highlight_toggle (GtkWidget *widget, gpointer data);
static void
motion_option_set (GtkWidget *widget, gpointer data);
static void
grid_toggle (GtkWidget *widget, gpointer data);
static void
magnification_update (GtkWidget *widget, gpointer ptr);
#ifdef XFIG
static void
twfa_toggle (GtkWidget *widget, gpointer ptr);
static void
twfa_call (GtkWidget *widget, gpointer ptr);
#endif /* XFIG */

/*****************************************************************************

				public code
  
*****************************************************************************/

void
display_settings (GtkWidget *widget, gpointer ptr)
/*
 *  Generate and display the dialog window 'Display options'.
 *
 *  No return value.
 */
{
   static GtkWidget *window = NULL;
   GtkWidget	    *notebook, *button;
   xwfa_display_t  *display = (xwfa_display_t *) ptr;
   
   if (!window)
   {
      window = gtk_dialog_new ();
      gtk_window_set_title (GTK_WINDOW (window), "Display options");

      gtk_signal_connect_object (GTK_OBJECT (window), "delete_event",
				 GTK_SIGNAL_FUNC (gtk_widget_hide),
				 GTK_OBJECT (window));

      button = gtk_button_new_with_label ("Close");
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button,
			  TRUE, TRUE, 0);
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 GTK_SIGNAL_FUNC (gtk_widget_hide),
				 GTK_OBJECT (window));

      notebook = gtk_notebook_new ();
      gtk_widget_show (notebook);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), notebook,
			  TRUE, TRUE, 0);

      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				background_settings (display),
				gtk_label_new ("Background"));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				decoder_settings (display),
				gtk_label_new ("Decoder"));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				partitoning_settings (display),
				gtk_label_new ("Partitioning"));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				highlighting_settings (display),
				gtk_label_new ("Highlighting"));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				preferences_settings (display),
				gtk_label_new ("Preferences"));
   }

   if (!GTK_WIDGET_VISIBLE (window))
      gtk_widget_show (window);
   else
      gtk_widget_hide (window);
}

void
background_sensitive (xwfa_display_t *display)
/*
 *  Traverse array of background radio buttons and set sensitiveness
 *  if corresponding background image is available.
 *
 *  No return value.
 */
{
   background_e bg;

   if (display->background != BG_NONE &&!display->bg_image [display->background])
   {
      display->background = display->bg_image [BG_WFA] ? BG_WFA : BG_NONE;

      for (bg = BG_WFA; bg < BG_NULL; bg++)
	 gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->bg_button [bg]), bg == display->background);
   }

   for (bg = BG_WFA; bg < BG_NULL; bg++)
      if (display->bg_button [bg])
	 gtk_widget_set_sensitive (display->bg_button [bg],
				   display->bg_image [bg] != NULL);
}

void
prediction_sensitive (xwfa_display_t *display)
/*
 *  Traverse array of prediction radio buttons and set sensitiveness
 *  if corresponding prediction is available.
 *
 *  No return value.
 */
{
   int n;
   
   if (!display->pred_button [GRID_RANGE])
      return;				/* nothing to do */

   for (n = 0; n < GRID_NULL; n++)
      display->show_grid [n]
	 = GTK_TOGGLE_BUTTON (display->pred_button [n])->active ? YES : NO;
      
   gtk_widget_set_sensitive (display->pred_button [GRID_RANGE], YES);
   gtk_widget_set_sensitive (display->pred_button [GRID_ND],
			     display->lc_prediction);
   gtk_widget_set_sensitive (display->pred_button [GRID_FORWARD], display->video
			     && (display->video->wfa->frame_type != I_FRAME));
   gtk_widget_set_sensitive (display->pred_button [GRID_BACKWARD], display->video
			     && (display->video->wfa->frame_type == B_FRAME));
   gtk_widget_set_sensitive (display->pred_button [GRID_INTERPOLATED],
			     display->video
			     && (display->video->wfa->frame_type == B_FRAME));
   gtk_widget_set_sensitive (display->pred_button [GRID_NULL], display->video
			     && (display->video->wfa->frame_type != I_FRAME));
}

#ifdef XFIG

void
twfa_settings (GtkWidget *widget, gpointer ptr)
/*
 *  Generate and display the dialog window 'WFA bintree options'.
 *
 *  No return value.
 */ 
{
   static GtkWidget *window = NULL;
   xwfa_display_t   *display = (xwfa_display_t *) ptr;

   if (!display->wfa)			/*  || streq (XFIG, "no") */
      return;
   
   if (!window)
   {
      GtkWidget *hbox, *label, *entry, *button, *table;
      GtkTooltips *tooltips;
      
      tooltips = gtk_tooltips_new ();
      {
	 GdkColor	*yellow, *black;		
	 GdkColormap	*colormap;

	 yellow = Calloc (1, sizeof (GdkColor));
	 black  = Calloc (1, sizeof (GdkColor));
	 
	 colormap = gdk_window_get_colormap (display->root_window->window);

	 yellow->red    = 255 << 8;
	 yellow->green  = 255 << 8;
	 yellow->blue   =   0 << 8;

	 if (!gdk_color_alloc (colormap, yellow))
	    gdk_color_white (colormap, yellow);
	 
	 black->red   = 0 << 8;
 	 black->green = 0 << 8;
	 black->blue  = 0 << 8;
	 
	 if (!gdk_color_alloc (colormap, black))
	    gdk_color_black (colormap, black);

	 gtk_tooltips_set_colors (tooltips, yellow, black);
      }
      
      window = gtk_dialog_new ();
      gtk_window_set_title (GTK_WINDOW (window), "WFA bintree options");

      gtk_signal_connect_object (GTK_OBJECT (window), "delete_event",
				 GTK_SIGNAL_FUNC (gtk_widget_hide),
				 GTK_OBJECT (window));

      /*
       *  Start computation of bintree
       */
      
      button = gtk_button_new_with_label ("Show WFA tree");
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button,
			  TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (twfa_call), (gpointer) display);
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 GTK_SIGNAL_FUNC (gtk_widget_hide),
				 GTK_OBJECT (window));
      gtk_tooltips_set_tip (tooltips, button,
			    "Start external computation of bintree with the "
			    "tool `bfiasco', and launch "
			    "`xfig' with the generated figure.", NULL);

      /*
       *  Close button
       */

      button = gtk_button_new_with_label ("Close");
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button,
			  TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (twfa_flush), (gpointer) display);
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 GTK_SIGNAL_FUNC (gtk_widget_hide),
				 GTK_OBJECT (window));

      table = gtk_table_new (9, 2, FALSE);
      gtk_table_set_row_spacings (GTK_TABLE (table), 5);
      gtk_table_set_col_spacings (GTK_TABLE (table), 5);
      gtk_container_border_width (GTK_CONTAINER (table), 5);
      gtk_widget_show (table);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), table,
			  TRUE, TRUE, 5);

      /*
       *  Subtree root state
       */
      
      label = gtk_label_new ("Subtree root-state");
      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (entry), "-1");
      gtk_widget_set_usize (entry, 50, -1);
      gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 0, 1);
      gtk_widget_show (entry);
      display->twfa [TWFA_ROOT] = entry;
      gtk_tooltips_set_tip (tooltips, entry,
			    "You can use mouse button 2 to mark a "
			    "rectangular region in the image partitioning "
			    "display. The root state will be automatically set "
			    "to the smallest range covering the marked region.",
			    NULL);

      /*
       *  Bintree max-depth
       */
      label = gtk_label_new ("Max. bintree depth");
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (entry), "-1");
      gtk_widget_set_usize (entry, 50, -1);
      gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 1, 2);
      gtk_widget_show (entry);
      display->twfa [TWFA_DEPTH] = entry;
      gtk_tooltips_set_tip (tooltips, entry,
			    "Restrict bintree to a maximum depth", NULL);

      /*
       *  WFA grid
       */
      button = gtk_check_button_new_with_label ("Partitioning");
      gtk_widget_show (button);
      gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 2, 3);
      display->twfa [TWFA_GRID] = button;
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (twfa_toggle), (gpointer) display);
      gtk_tooltips_set_tip (tooltips, button,
			    "Show image partitioning.", NULL);

      button = gtk_check_button_new_with_label ("Color partitioning");
      gtk_widget_show (button);
      gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 2, 3);
      display->twfa [TWFA_CGRID] = button;
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_tooltips_set_tip (tooltips, button,
			    "Use colors to distinguish states in different "
			    "regions of the image partitioning.", NULL);

      /*
       *  State symbols and text
       */
      button = gtk_check_button_new_with_label ("State symbols");
      gtk_widget_show (button);
      gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 3, 4);
      display->twfa [TWFA_STATES] = button;
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (twfa_toggle), (gpointer) display);
      gtk_tooltips_set_tip (tooltips, button,
			    "Use symbols to represent different WFA states: "
			    "triangles are multi-states, "
			    "squares are motion compensated states and "
			    "circles are normal states.", NULL);

      button = gtk_check_button_new_with_label ("State numbering");
      gtk_widget_show (button);
      gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 3, 4);
      display->twfa [TWFA_STATE_NUM] = button;
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_tooltips_set_tip (tooltips, button,
			    "Print state symbols with state numbers.", NULL);

      /*
       *  Prune tree
       */
      button = gtk_check_button_new_with_label ("Prune tree at first LC");
      gtk_widget_show (button);
      gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 4, 5);
      display->twfa [TWFA_PRUNE] = button;
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_tooltips_set_tip (tooltips, button,
			    "Traverse bintree recursively up to the level where"
			    " the first linear combination is found. "
			    "The union of missed states is represented by "
			    "one multi-state.", NULL);

      /*
       *  Basis
       */
      button = gtk_check_button_new_with_label ("Initial basis");
      gtk_widget_show (button);
      gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 4, 5);
      display->twfa [TWFA_BASIS] = button;
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (twfa_toggle), (gpointer) display);
      gtk_tooltips_set_tip (tooltips, button,
			    "Show initial basis states.", NULL);

      /*
       *  LC from states
       */
      
      button = gtk_check_button_new_with_label ("LC edges");
      gtk_widget_show (button);
      gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 5, 6);
      display->twfa [TWFA_LC] = button;
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (twfa_toggle), (gpointer) display);
      gtk_tooltips_set_tip (tooltips, button,
			    "Show linear combination edges into"
			    " non-basis states.", NULL);

      label = gtk_label_new ("States to consider:");
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_widget_show (label);
      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 6, 7);
      display->twfa [TWFA_LC_LABEL] = label;
      gtk_widget_set_sensitive (label, FALSE);

      entry = gtk_entry_new ();
      gtk_widget_show (entry);
      gtk_entry_set_text (GTK_ENTRY (entry), "");
      gtk_widget_set_usize (entry, 50, -1);
      gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 6, 7);
      display->twfa [TWFA_LC_LIST] = entry;
      gtk_widget_set_sensitive (entry, FALSE);
      gtk_tooltips_set_tip (tooltips, entry,
			    "Show only linear combination edges of "
			    "states defined by the regexp "
			    "(NUM[-NUM],)*[NUM[-NUM]].", NULL);

      /*
       *  LC from basis states
       */
      button = gtk_check_button_new_with_label ("LC edges into basis");
      gtk_widget_show (button);
      gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 7, 8);
      display->twfa [TWFA_BLC] = button;
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (twfa_toggle), (gpointer) display);
      gtk_tooltips_set_tip (tooltips, button,
			    "Show linear combination edges into basis states.",
			    NULL);

      label = gtk_label_new ("States to consider:");
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_widget_show (label);
      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 8, 9);
      display->twfa [TWFA_BLC_LABEL] = label;
      gtk_widget_set_sensitive (label, FALSE);

      entry = gtk_entry_new ();
      gtk_widget_show (entry);
      gtk_entry_set_text (GTK_ENTRY (entry), "");
      gtk_widget_set_usize (entry, 50, -1);
      gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 8, 9);
      display->twfa [TWFA_BLC_LIST] = entry;
      gtk_widget_set_sensitive (entry, FALSE);
      gtk_tooltips_set_tip (tooltips, entry,
			    "Show only linear combination edges of "
			    "states defined by the regexp "
			    "(NUM[-NUM],)*[NUM[-NUM]].", NULL);

      /*
       *  Shadow, key and levels
       */
      hbox = gtk_hbox_new (FALSE, 5);
      gtk_widget_show (hbox);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox,
			  TRUE, TRUE, 5);
      
      button = gtk_check_button_new_with_label ("Shadows");
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 5);
      gtk_widget_show (button);
      display->twfa [TWFA_SHADOWS] = button;
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);

      button = gtk_check_button_new_with_label ("Key");
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 5);
      gtk_widget_show (button);
      display->twfa [TWFA_KEY] = button;
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);

      button = gtk_check_button_new_with_label ("Levels");
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 5);
      gtk_widget_show (button);
      display->twfa [TWFA_LEVELS] = button;
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), FALSE);
   }
   if (!GTK_WIDGET_VISIBLE (window))
      gtk_widget_show (window);
   else
      gtk_widget_hide (window);
}

void
twfa_flush (GtkWidget *widget, gpointer ptr)
/*
 *  Remove marked 'twfa' region.
 *
 *  No return value.
 */
{
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   
   display->region_set    = NO;
   display->region_active = NO;
   display->root_state    = -1;
   prediction_sensitive (display);
   clear_display (NULL, display);
}

/****************************************************************************

				private code
  
*****************************************************************************/

static void
twfa_toggle (GtkWidget *widget, gpointer ptr)
/*
 *  Toggle the clicked toggle-button in the 'twfa dialog window'.
 *  Also, update other widgets if required.
 *
 *  No return value.
 */
{
   xwfa_display_t   *display = (xwfa_display_t *) ptr;
   tree_opt_e i;
 
   for (i = 0; i < TWFA_NULL; i++)
      if (widget == display->twfa [i])	/* matched */
      {
	 switch (i)
	 {
	    case TWFA_LC:
	       gtk_widget_set_sensitive (display->twfa [TWFA_LC_LIST],
					 GTK_TOGGLE_BUTTON (widget)->active);
	       gtk_widget_set_sensitive (display->twfa [TWFA_LC_LABEL],
					 GTK_TOGGLE_BUTTON (widget)->active);
	       if (GTK_TOGGLE_BUTTON (widget)->active)
	       {
		  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->twfa [TWFA_STATES]), TRUE);
	       }
	       break;
	    case TWFA_BLC:
	       gtk_widget_set_sensitive (display->twfa [TWFA_BLC_LIST],
					 GTK_TOGGLE_BUTTON (widget)->active);
	       gtk_widget_set_sensitive (display->twfa [TWFA_BLC_LABEL],
					 GTK_TOGGLE_BUTTON (widget)->active);
	       if (GTK_TOGGLE_BUTTON (widget)->active)
	       {
		  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->twfa [TWFA_BASIS]), TRUE);
	       }
	       break;
	    case TWFA_GRID:
	       gtk_widget_set_sensitive (display->twfa [TWFA_CGRID],
					 GTK_TOGGLE_BUTTON (widget)->active);
	       break;
	    case TWFA_STATES:
	       if (!GTK_TOGGLE_BUTTON (widget)->active)
	       {
		  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->twfa [TWFA_LC]), FALSE);
		  gtk_widget_set_sensitive (display->twfa [TWFA_LC_LIST], FALSE);
		  gtk_widget_set_sensitive (display->twfa [TWFA_LC_LABEL], FALSE);
	       }
	       else
	       {
		  if (!GTK_TOGGLE_BUTTON (display->twfa [TWFA_LC])->active)
		  {
		     gtk_widget_set_sensitive (display->twfa [TWFA_LC_LIST],
					       FALSE);
		     gtk_widget_set_sensitive (display->twfa [TWFA_LC_LABEL],
					       FALSE);
		  }
	       }
	       gtk_widget_set_sensitive (display->twfa [TWFA_STATE_NUM],
					 GTK_TOGGLE_BUTTON (widget)->active);
	       break;
	    case TWFA_BASIS:
	       if (!GTK_TOGGLE_BUTTON (widget)->active)
	       {
		  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->twfa [TWFA_BLC]), FALSE);
		  gtk_widget_set_sensitive (display->twfa [TWFA_BLC_LIST],
					    FALSE);
		  gtk_widget_set_sensitive (display->twfa [TWFA_BLC_LABEL],
					    FALSE);
	       }
	       else
	       {
		  if (!GTK_TOGGLE_BUTTON (display->twfa [TWFA_BLC])->active)
		  {
		     gtk_widget_set_sensitive (display->twfa [TWFA_BLC_LIST],
					       FALSE);
		     gtk_widget_set_sensitive (display->twfa [TWFA_BLC_LABEL],
					       FALSE);
		  }
	       }
	       break;
	    default:
	       break;
	 }
      }
}

static void
twfa_call (GtkWidget *widget, gpointer ptr)
/*
 *  Parse all 'twfa' toggle buttons and entries and generate a command to
 *  invoke the external programs 'twfa' and 'xfig'.
 *
 *  No return value.
 */
{
   char			cmd_line [MAXSTRLEN];
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   tree_opt_e		opt;
   
   sprintf (cmd_line, "bfiasco -f%d ", display->frame_n);

   for (opt = 0; opt < TWFA_NULL; opt++)
      switch (opt)
      {
	 case TWFA_SHADOWS:
	    if (GTK_TOGGLE_BUTTON (display->twfa [opt])->active)
	       strncat (cmd_line,  "--shadows ",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	    break;
	 case TWFA_KEY:
	    if (GTK_TOGGLE_BUTTON (display->twfa [opt])->active)
	       strncat (cmd_line,  "--key ",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	    break;
	 case TWFA_LEVELS:
	    if (GTK_TOGGLE_BUTTON (display->twfa [opt])->active)
	       strncat (cmd_line,  "--levels ",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	    break;
	 case TWFA_PRUNE:
	    if (GTK_TOGGLE_BUTTON (display->twfa [opt])->active)
	       strncat (cmd_line,  "-p ",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	    break;
	 case TWFA_GRID:
	    if (GTK_TOGGLE_BUTTON (display->twfa [opt])->active)
	    {
	       if (GTK_TOGGLE_BUTTON (display->twfa [TWFA_CGRID])->active)
		  strncat (cmd_line,  "-G ",
			   max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	       else
		  strncat (cmd_line,  "-g ",
			   max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	    }
	    break;
	 case TWFA_STATES:
	    if (GTK_TOGGLE_BUTTON (display->twfa [opt])->active)
	    {
	       if (GTK_TOGGLE_BUTTON (display->twfa [TWFA_STATE_NUM])->active)
		  strncat (cmd_line,  "-S ",
			   max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	       else
		  strncat (cmd_line,  "-s ",
			   max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	    }
	    break;
	 case TWFA_BASIS:
	    if (GTK_TOGGLE_BUTTON (display->twfa [opt])->active)
	       strncat (cmd_line,  "-b ",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	    break;
	 case TWFA_ROOT:
	    if (strtol (gtk_entry_get_text (GTK_ENTRY (display->twfa [opt])),
			NULL, 10) >= display->wfa->basis_states)
	    {
	       strncat (cmd_line,  "-r",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	       strncat (cmd_line,
			gtk_entry_get_text (GTK_ENTRY (display->twfa [opt])),
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	       strncat (cmd_line,  " ",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	    }
	    break;
	 case TWFA_DEPTH:
	    if (strtol (gtk_entry_get_text (GTK_ENTRY (display->twfa [opt])),
			NULL, 10) > 0)
	    {
	       strncat (cmd_line,  "-d",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	       strncat (cmd_line,
			gtk_entry_get_text (GTK_ENTRY (display->twfa [opt])),
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	       strncat (cmd_line,  " ",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	    }
	    break;
	 case TWFA_LC:
	    if (GTK_TOGGLE_BUTTON (display->twfa [opt])->active)
	    {
	       strncat (cmd_line,  "-l",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	       strncat (cmd_line,
			gtk_entry_get_text (GTK_ENTRY (display->twfa [TWFA_LC_LIST])),
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	       strncat (cmd_line,  " ",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	    }
	    break;
	 case TWFA_BLC:
	    if (GTK_TOGGLE_BUTTON (display->twfa [opt])->active)
	    {
	       strncat (cmd_line,  "-L",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	       strncat (cmd_line,
			gtk_entry_get_text (GTK_ENTRY (display->twfa [TWFA_BLC_LIST])),
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	       strncat (cmd_line,  " ",
			max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
	    }
	    break;
	 default:
	    break;
      }
   
   strncat (cmd_line,  display->wfa_path,
	    max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
   strncat (cmd_line,  " > ",
	    max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
   strncat (cmd_line,  display->wfa_path,
	    max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
   strncat (cmd_line,  ".fig",
	    max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
   if (system (cmd_line) != 0)
      dialog_popup (DIALOG_ERROR,
		    "An error has been catched during bintree computation.\n"
		    "Please check standard output for more details.",
		    NULL, NULL);
   sprintf (cmd_line, XFIG " ");
   strncat (cmd_line,  display->wfa_path,
	    max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
   strncat (cmd_line,  ".fig &",
	    max (MAXSTRLEN - 1 - (int) strlen (cmd_line), 0));
   system (cmd_line);

   twfa_flush (NULL, display);
}

#endif /* XFIG */

static GtkWidget *
background_settings (xwfa_display_t *display)
/*
 *  Generate and display the notebook page 'Background image'.
 *
 *  No return value.
 */
{
   GtkWidget	*frame, *vbox;
   GtkWidget    *button = NULL;
   char		*bg_label [BG_NULL];
   background_e	bg;
   
   bg_label [BG_NONE]            = "None";
   bg_label [BG_WFA]             = "Decoded frame";
   bg_label [BG_ORIGINAL]        = "Original frame";
   bg_label [BG_DIFF]            = "Difference";
   bg_label [BG_PREDICTION]      = "Prediction";
   bg_label [BG_DELTA]           = "Delta approximation";
   
   frame = gtk_frame_new ("Background image");
   gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_border_width (GTK_CONTAINER (frame), 5);

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (frame), vbox);
   gtk_widget_show (vbox);
   
   for (bg = 0; bg < BG_NULL; bg++)
   {
      GSList *group;

      group = button ? gtk_radio_button_group (GTK_RADIO_BUTTON (button)) : NULL;
      display->bg_button [bg]
	 = button
	 = gtk_radio_button_new_with_label (group, bg_label [bg]);
      gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);
      if (bg == display->background)
	 gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      else
	 gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) bg);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (background_toggle),
			  (gpointer) display);
   }

   background_sensitive (display);
   gtk_widget_show (frame);
   return frame;
}

static void
background_toggle (GtkWidget *widget, gpointer ptr)
/*
 *  Update the display of the clickable areas according to pressed radio button.
 *
 *  No return value.
 */
{
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   
   if (GTK_TOGGLE_BUTTON (widget)->active) 
   {
      int n;
      
      display->background = (int) gtk_object_get_user_data (GTK_OBJECT (widget));

      for (n = 0; n < 4; n++)
	 if (display->click_areas [n])
	 {
	    draw_background (display->background, display->bg_image, n,
			     display->click_areas [n]);
	    gtk_widget_draw (display->click_areas [n], NULL);
	 }
   } 
}

static GtkWidget *
decoder_settings (xwfa_display_t *display)
/*
 *  Generate and display the notebook page 'Decoder options'.
 *
 *  No return value.
 */
{
   GtkWidget *frame, *vbox;
   GtkWidget *label, *sep;
   GtkWidget *scale;
   GtkObject *adjustment;
   
   frame = gtk_frame_new ("Decoder options");
   gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_border_width (GTK_CONTAINER (frame), 5);

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (frame), vbox);
   gtk_container_border_width (GTK_CONTAINER (vbox), 5);
   gtk_widget_show (vbox);

   label = gtk_label_new ("Smoothing [%]");
   gtk_widget_show (label);
   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
   
   adjustment = gtk_adjustment_new (display->smoothing_factor, 0.0, 101.0,
				    10, 10, 1);
   
   scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
   gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
   gtk_scale_set_digits (GTK_SCALE (scale), 0);
   gtk_scale_set_draw_value (GTK_SCALE (scale), TRUE);
   gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
   gtk_widget_show (scale);
   gtk_object_set_user_data (GTK_OBJECT (adjustment), (gpointer) scale);
   gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		       GTK_SIGNAL_FUNC (smoothing_update), (gpointer) display);
   
   sep = gtk_hseparator_new ();
   gtk_widget_show (sep);
   gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 10);
   
   label = gtk_label_new ("Magnification factor");
   gtk_widget_show (label);
   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

   {
      GtkWidget    *button = NULL, *hbox;
      int	   mag;
      
      hbox = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_container_border_width (GTK_CONTAINER (hbox), 5);
      gtk_widget_show (hbox);
      
      for (mag = 0; mag < 4; mag++)
      {
	 GSList *group;
	 char   *label [] = {"x1/4", "x1", "x4", "x16"};
	 group = button
		 ? gtk_radio_button_group (GTK_RADIO_BUTTON (button))
		 : NULL;
	 button = gtk_radio_button_new_with_label (group, label [mag]);
	 gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
	 gtk_widget_show (button);
	 if (streq (label [mag], "x1"))
	    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
	 else
	    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), FALSE);
	 gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) mag);
	 gtk_signal_connect (GTK_OBJECT (button), "toggled",
			     GTK_SIGNAL_FUNC (magnification_update),
			     (gpointer) display);
      }
   }

   gtk_widget_show (frame);
   return frame;
}

static void
smoothing_update (GtkWidget *widget, gpointer ptr)
/*
 *  Decode current WFA frame using a new 'smoothing' value.
 *
 *  No return value.
 */
{
   GtkAdjustment	*adj;
   int			n;
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   GtkWidget *scale = gtk_object_get_user_data (GTK_OBJECT (widget));
   
   adj = gtk_range_get_adjustment (GTK_RANGE (GTK_SCALE (scale)));
   display->smoothing_factor = (gdouble) adj->value;

   if (display->video != NULL)
   {
      generate_wfa_backgrounds (display);
      for (n = 0; n < (display->video->wfa->wfainfo->color ? 4 : 1); n++)
      {
	 draw_background (display->background, display->bg_image, n,
			  display->click_areas [n]);
	 gtk_widget_draw (display->click_areas [n], NULL);
      }
   }
}

static void
magnification_update (GtkWidget *widget, gpointer ptr)
/*
 *  Decode current WFA frame using a new 'magnification' value.
 *
 *  No return value.
 */
{
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   
   if (GTK_TOGGLE_BUTTON (widget)->active) 
   {
      display->enlarge_factor
	 = (int) gtk_object_get_user_data (GTK_OBJECT (widget)) - 1;
      
      if (display->video != NULL)
	 load_wfa (NULL, display);
   }
}

static GtkWidget *
partitoning_settings (xwfa_display_t *display)
/*
 *  Generate and display the notebook page 'Partitioning display'.
 *
 *  No return value.
 */
{
   GtkWidget	*frame, *vbox, *hbox;
   GtkWidget	*menu, *menu_item;
   GtkWidget	*option_menu;
   GtkWidget	*widget;
   int		grid;
   
   frame = gtk_frame_new ("Partitioning display");
   gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_border_width (GTK_CONTAINER (frame), 5);

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (frame), vbox);
   gtk_widget_show (vbox);

   for (grid = 0; grid < GRID_NULL; grid++)
   {
      GtkWidget	*button;
      char	*text [] = {"Refining (delta) approximation",
			    "Linear combination",
			    "Forward motion compensation",
			    "Backward motion compensation",
			    "Interpolated motion compensation"};

      button = gtk_check_button_new_with_label (text [grid]);
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
				   display->show_grid [grid]);
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) grid);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (grid_toggle), (gpointer) display);
      display->pred_button [grid] = button;

      if (grid == 0)
      {
	 widget = gtk_hseparator_new ();
	 gtk_widget_show (widget);
	 gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);
   
	 widget = gtk_label_new ("Prediction");
	 gtk_widget_show (widget);
	 gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);
      }
   }

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (hbox), 5);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
   gtk_widget_show (hbox);

   widget = gtk_label_new ("Display of:");
   gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 5);
   gtk_widget_show (widget);

   option_menu = gtk_option_menu_new();

   menu = gtk_menu_new ();
   
   menu_item = gtk_menu_item_new_with_label("Range");
   gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		       GTK_SIGNAL_FUNC (motion_option_set), (gpointer) display);
   gtk_object_set_user_data (GTK_OBJECT (menu_item), (gpointer) MO_RANGE);
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show (menu_item);
   
   menu_item = gtk_menu_item_new_with_label("Range & Reference");
   gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		       GTK_SIGNAL_FUNC (motion_option_set), (gpointer) display);
   gtk_object_set_user_data (GTK_OBJECT (menu_item), (gpointer) MO_BOTH);
   gtk_menu_append (GTK_MENU(menu), menu_item);
   gtk_widget_show (menu_item);

   menu_item = gtk_menu_item_new_with_label("Reference");
   gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		       GTK_SIGNAL_FUNC (motion_option_set), (gpointer) display);
   gtk_object_set_user_data (GTK_OBJECT (menu_item), (gpointer) MO_REFERENCE);
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show (menu_item);

   menu_item = gtk_menu_item_new_with_label("Motion vector");
   gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		       GTK_SIGNAL_FUNC (motion_option_set), (gpointer) display);
   gtk_object_set_user_data (GTK_OBJECT (menu_item), (gpointer) MO_VECTOR);
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show (menu_item);

   gtk_box_pack_end (GTK_BOX (hbox), option_menu, FALSE, FALSE, 5);
   gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
   gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu),
				display->motion_display);
   gtk_widget_show (option_menu);

   display->pred_button [grid] = option_menu;

   prediction_sensitive (display);
   
   gtk_widget_show (frame);
   return frame;
}

static void
motion_option_set (GtkWidget *widget, gpointer ptr)
/*
 *  Update clickable areas according to motion toggle buttons.
 *
 *  No return value.
 */
{
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   int			n;

   display->motion_display
      = (motion_e) gtk_object_get_user_data (GTK_OBJECT (widget));
   for (n = 0; n < 3; n++)
      if (display->click_areas [n])
	 gtk_widget_draw (display->click_areas [n], NULL);
}

static void
grid_toggle (GtkWidget *widget, gpointer ptr)
/*
 *  Update clickable areas according to motion toggle buttons.
 *
 *  No return value.
 */
{
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   int	n;
   int	data = (int) gtk_object_get_user_data (GTK_OBJECT (widget));
   bool_t hl = NO;
   
   display->show_grid [data] = GTK_TOGGLE_BUTTON (widget)->active;

   for (n = 0; n < GRID_NULL; n++)
      if (display->show_grid [n])
      {
	 hl = YES;
	 display->automatic_highlighting [0] = NO;
	 display->automatic_highlighting [1] = NO;
	 display->automatic_highlighting [2] = NO;
	 gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->hl_button [0]),
				      NO);
	 gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->hl_button [1]),
				      NO);
	 gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->hl_button [2]),
				      NO);
	 gtk_widget_set_sensitive (display->hl_button [0], NO);
	 gtk_widget_set_sensitive (display->hl_button [1], NO);
	 gtk_widget_set_sensitive (display->hl_button [2], NO);
      }

   if (!hl)
   {
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->hl_button [0]),
				   display->automatic_highlighting [0]);
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->hl_button [1]),
				   display->automatic_highlighting [1]);
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (display->hl_button [2]),
				   display->automatic_highlighting [2]);
      gtk_widget_set_sensitive (display->hl_button [0], YES);
      gtk_widget_set_sensitive (display->hl_button [1], YES);
      if (display->video == NULL)
	 gtk_widget_set_sensitive (display->hl_button [2],
				   display->lc_prediction);
      else
	 gtk_widget_set_sensitive (display->hl_button [2],
				   display->lc_prediction
				   || display->video->wfa->frame_type != I_FRAME);

   }

   clear_current_range ();
   for (n = 0; n < 3; n++)
      if (display->click_areas [n])
	 gtk_widget_draw (display->click_areas [n], NULL);
}

static GtkWidget *
highlighting_settings (xwfa_display_t *display)
/*
 *  Generate and display the notebook page 'Automatic highlighting'.
 *
 *  No return value.
 */
{
   GtkWidget	*frame, *vbox;
   int		highlight;
   
   frame = gtk_frame_new ("Automatic highlighting");
   gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_border_width (GTK_CONTAINER (frame), 5);

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (frame), vbox);
   gtk_widget_show (vbox);

   for (highlight = 0; highlight < 3; highlight++)
   {
      GtkWidget	*button;
      char	*text [3] = {"Range", "Domains", "Prediction"};

      button = gtk_check_button_new_with_label (text [highlight]);
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
				   display->automatic_highlighting [highlight]);
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) highlight);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (highlight_toggle),
			  (gpointer) display);

      if (display->video && highlight == 2)
	 gtk_widget_set_sensitive (button, display->lc_prediction
				   || display->video->wfa->frame_type != I_FRAME);

      display->hl_button [highlight] = button;
   }

   gtk_widget_show (frame);
   return frame;
}

static GtkWidget *
preferences_settings (xwfa_display_t *display)
/*
 *  Generate and display the notebook page 'Preferences'.
 *
 *  No return value.
 */
{
   GtkWidget	*frame, *vbox;
   GtkWidget	*button;
   
   frame = gtk_frame_new ("Preferences");
   gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_border_width (GTK_CONTAINER (frame), 5);

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (frame), vbox);
   gtk_widget_show (vbox);

   button = gtk_check_button_new_with_label ("Basis images");
   gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
   gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
   gtk_signal_connect (GTK_OBJECT (button), "toggled",
		       GTK_SIGNAL_FUNC (hide_unhide_window),
		       (gpointer) &display->basis);

   button = gtk_check_button_new_with_label ("Linear combination");
   gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
   gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
   gtk_signal_connect (GTK_OBJECT (button), "toggled",
		       GTK_SIGNAL_FUNC (hide_unhide_window),
		       (gpointer) &display->lc);

   button = gtk_check_button_new_with_label ("Toolbar");
   gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
   gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
   gtk_signal_connect (GTK_OBJECT (button), "toggled",
		       GTK_SIGNAL_FUNC (hide_unhide_window),
		       (gpointer) &display->toolbar);

   button = gtk_check_button_new_with_label ("Status window");
   gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
   gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
   gtk_signal_connect (GTK_OBJECT (button), "toggled",
		       GTK_SIGNAL_FUNC (hide_unhide_window),
		       (gpointer) &display->status);

   gtk_widget_show_all (frame);
   return frame;
}

static void
highlight_toggle (GtkWidget *widget, gpointer ptr)
/*
 *  Update automatic highlighting according to toggle buttons.
 *
 *  No return value.
 */
{
   xwfa_display_t	*display = (xwfa_display_t *) ptr;
   int	n;
   int	data = (int) gtk_object_get_user_data (GTK_OBJECT (widget));

   display->automatic_highlighting [data] = GTK_TOGGLE_BUTTON (widget)->active;

   for (n = 0; n < 3; n++)
      if (display->click_areas [n])
	 gtk_widget_draw (display->click_areas [n], NULL);
}

#if 0   
static void
call_statistics (GtkWidget *widget, gpointer ptr)
/*
 *  Call external plotting program 'plotmtv' with some WFA statistics.
 *
 *  No return value.
 */
{
   plot_t	(*(*statistics[]) (real_t bpp, plot_t *plot, const wfa_t *wfa))
      = {ranges_statistics, edges_statistics, edges_level_statistics,
	 weights_level_statistics, weights_edges_statistics,
	 weights_position_statistics, weights_state0_statistics,
	 final_distribution_statistics};
   int	data = (int) gtk_object_get_user_data (GTK_OBJECT (widget));
   xwfa_display_t *display = (xwfa_display_t *) ptr;
      
   if (display->video == NULL)
      return;

   if (display->video->wfa->wfainfo->frames > 1)
   {
      dialog_popup (DIALOG_ERROR,
		    "Statistical analysis of videos\nnot supported until now",
		    NULL, NULL);
      return;
   }
   else if (display->statistic_wfa == NULL) /* no log file */
      return;

   if (strneq (PLOTMTV, "no"))		/* plotmtv  installed */
   {
      plot_t *plot;

      plot = statistics [data] (display->bpp, NULL, display->statistic_wfa);
      plotmtv (NO, plot);
   }
}
#endif

static void
hide_unhide_window (GtkWidget *widget, gpointer ptr)
{
   GtkWidget *window = * (GtkWidget **) ptr;
   
   if (!GTK_WIDGET_VISIBLE (window))
      gtk_widget_show (window);
   else
      gtk_widget_hide (window);
}
