/*
 *  xwfa.c:		FIASCO GTK+ analysis tool	
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
#include "dialog.h"
#include "callbacks.h"
#include "background.h"
#include "view.h"
#include "xwfa.h"
#include "icons.h"
#include "error.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
show_main_window (xwfa_display_t *display, char *wfa_name, char *image_name);
static GtkWidget *
menu_container (xwfa_display_t *display);
static GtkWidget *
status_container (xwfa_display_t *display);
static GtkWidget *
image_container (xwfa_display_t *display);
GtkWidget *
basis_states_container (xwfa_display_t *display);
GtkWidget *
lincomb_container (xwfa_display_t *display);
static void
about_xfiasco (GtkWidget *w, gpointer ptr);
static GtkWidget *
toolbar_container (xwfa_display_t *display);
static void
xmag (GtkWidget *widget, gpointer ptr);

/*****************************************************************************

				public code
  
*****************************************************************************/

int
main (int argc, char *argv[])
{
   char			*wfa_name   = NULL;
   char			*image_name = NULL;
   xwfa_display_t	*display;
   
   gtk_init (&argc, &argv);
   
   if (argc > 1)
      wfa_name = streq (argv [1], "-") ? NULL : argv [1];
   if (argc > 2)
      image_name = streq (argv [2], "-") ? NULL : argv [2];

   display = Calloc (1, sizeof (xwfa_display_t));
   show_main_window (display, wfa_name, image_name);
   
   gtk_main ();

   Free (display);

   return (0);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
about_xfiasco (GtkWidget *w, gpointer ptr)
/*
 *  Popup About XWFA window displaying some interesting information
 *  about the author of the package;-)
 *
 *  No return value.
 */
{
   GtkWidget *window;
   GtkWidget *button;
   GtkWidget *label;
   
   window = gtk_dialog_new ();
   gtk_window_set_title (GTK_WINDOW (window), "About xfiasco");

   gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		       GTK_SIGNAL_FUNC (gtk_false), NULL);
   gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
			      (GtkSignalFunc) gtk_widget_destroy,
			      GTK_OBJECT (window));

   button = gtk_button_new_with_label ("Close");
   gtk_widget_show (button);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button,
		       TRUE, TRUE, 0);
   gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			      (GtkSignalFunc) gtk_widget_destroy,
			      GTK_OBJECT (window));

   gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (window)->vbox), 5);
   label = gtk_label_new ("xfiasco, Version " VERSION);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox),
		       label, FALSE, FALSE, 5);
   gtk_widget_show (label);
   label = gtk_label_new ("Copyright (C) 1994-2000, Ullrich Hafner");
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox),
		       label, FALSE, FALSE, 5);
   gtk_widget_show (label);
   label = gtk_label_new ("<hafner@bifgoot.de>");
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox),
		       label, FALSE, FALSE, 5);
   gtk_widget_show (label);
   label = gtk_label_new ("http://ulli.linuxave.net/");
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox),
		       label, FALSE, FALSE, 5);
   gtk_widget_show (label);
   gtk_widget_show (window);
}

static void
show_main_window (xwfa_display_t *display, char *wfa_name, char *image_name)
/*
 *  Generate main window of XWFA.
 * 
 *  No return value.
 */
{
   GtkWidget *vbox;			/* all widgets are part of a vbox */
   
   /* Initialize display options */
   {
      int i;
      
      display->background     	= BG_WFA;   
      display->motion_display 	= MO_RANGE;
      display->smoothing_factor = 0;
      display->enlarge_factor	= 0;
      
      for (i = 0; i < 3; i++)
	 display->automatic_highlighting [i] = i == 0;
      for (i = 0; i < GRID_NULL; i++)
	 display->show_grid [i] = NO;
   }
   
   /* Generate a new toplevel window */
   display->root_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title (GTK_WINDOW (display->root_window),
			 "xfiasco " VERSION);
   gtk_container_border_width (GTK_CONTAINER (display->root_window), 0);
   gtk_window_set_policy (GTK_WINDOW (display->root_window),
			  TRUE, TRUE, FALSE);
   gtk_widget_realize (display->root_window);
   init_pixmaps (display->root_window);
   
   /* Connect delete/destroy events */
   gtk_signal_connect (GTK_OBJECT (display->root_window), "delete_event",
		       GTK_SIGNAL_FUNC (gtk_false), NULL);
   gtk_signal_connect (GTK_OBJECT (display->root_window), "destroy",
		       GTK_SIGNAL_FUNC (destroy_application), 0);

   /* All widgets are part of a vbox */
   vbox = gtk_vbox_new (FALSE, 5);
   gtk_widget_show (vbox);
   gtk_container_add (GTK_CONTAINER (display->root_window), vbox);

   gtk_box_pack_start (GTK_BOX (vbox), menu_container (display),
		       FALSE, FALSE, 0);
   gtk_box_pack_start (GTK_BOX(vbox),
		       display->toolbar = toolbar_container (display),
		       FALSE, TRUE, 0);

   gtk_box_pack_start (GTK_BOX (vbox),
		       display->status = status_container (display),
		       FALSE, FALSE, 0);
   
   /* Display area is composed of image partitioning and basis images */
   {
      GtkWidget    *hbox;

      hbox = gtk_hbox_new (FALSE, 5);
      gtk_widget_show (hbox);
      
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), image_container (display),
			  TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (hbox),
			  display->basis = basis_states_container (display),
			  FALSE, FALSE, 0);
   }
   
   gtk_box_pack_start (GTK_BOX (vbox),
		       display->lc = lincomb_container (display),
		       FALSE, FALSE, 0);
   
   gtk_widget_show (display->root_window);
   
   /* Load FIASCO file and image if specified on commandline */
   {
      char *env_path;			/* environment path */

      if (wfa_name)			/* WFA specified on command line? */
      {
	 display->wfa_path = strdup (wfa_name);
	 load_wfa (NULL, display);
      }
      else if ((env_path = getenv ("FIASCO_DATA")) != NULL)
      {
	 display->wfa_path = Calloc (strlen (env_path) + 2, sizeof (char));
	 strcpy (display->wfa_path, env_path);
	 strcat (display->wfa_path, "/");
      }
      if (image_name)
      {
	 display->image_path = strdup (image_name);
	 try {
	    generate_image_backgrounds (display->image_path,
					display->bg_image);
	    background_sensitive (display);
	    prediction_sensitive (display);
	 }
	 catch {
	    dialog_popup (DIALOG_ERROR,
			  "An error has been catched during image input.\n"
			  "Please check standard output for more details.",
			  NULL, NULL);
	 }
      }
      else if ((env_path = getenv ("FIASCO_IMAGES")) != NULL)
      {
	 display->image_path = Calloc (strlen (env_path) + 2, sizeof (char));
	 strcpy (display->image_path, env_path);
	 strcat (display->image_path, "/");
      }
   }
}

static GtkWidget *
menu_container (xwfa_display_t *display)
/*
 *  Generate widget containing the menu part of 'xfiasco'.
 *
 *  File->Load WFA
 *  File->Load image
 *  File->Next frame
 *  File->Quit
 *
 *  View->Clear display
 *  View->Display options ...
 *  View->WFA bintree ...
 *  View->Statistics ...
 *
 *  Help->About
 *
 *  Return value:
 *	container widget
 */
{
   GtkWidget *menu;
   GtkWidget *menu_bar;
   GtkWidget *menu_item;

   menu_bar = gtk_menu_bar_new();
   gtk_widget_show (menu_bar);

   /*
    *  FILE menu
    */
   menu      = gtk_menu_new();
   menu_item = gtk_menu_item_new_with_label ("File");
   gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
   gtk_menu_bar_append (GTK_MENU_BAR (menu_bar), menu_item);
   gtk_widget_show (menu_item);

   menu_item = gtk_menu_item_new_with_label ("Load FIASCO file ...");
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show(menu_item);
   gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
		       GTK_SIGNAL_FUNC (wfa_file_selection),
		       (gpointer) display);

   display->load_image_menu_item
      = menu_item = gtk_menu_item_new_with_label ("Load image ...");
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show(menu_item);
   gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
		       GTK_SIGNAL_FUNC (image_file_selection),
		       (gpointer) display);
   gtk_widget_set_sensitive (display->load_image_menu_item, NO);

   display->prev_frame_menu_item
      = menu_item = gtk_menu_item_new_with_label ("Previous frame");
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show(menu_item);
   gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
		       GTK_SIGNAL_FUNC (prev_frame), (gpointer) display);
   gtk_widget_set_sensitive (display->prev_frame_menu_item, NO);

   display->next_frame_menu_item
      = menu_item = gtk_menu_item_new_with_label ("Next frame");
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show(menu_item);
   gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
		       GTK_SIGNAL_FUNC (next_frame), (gpointer) display);
   gtk_widget_set_sensitive (display->next_frame_menu_item, NO);
   
   menu_item = gtk_menu_item_new ();
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show (menu_item);

   menu_item = gtk_menu_item_new_with_label ("Quit");
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show (menu_item);
   gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
		       GTK_SIGNAL_FUNC (destroy_application), NULL);

   /*
    *  VIEW menu
    */
   menu      = gtk_menu_new ();
   menu_item = gtk_menu_item_new_with_label ("View");
   gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
   gtk_menu_bar_append (GTK_MENU_BAR (menu_bar), menu_item);
   gtk_widget_show (menu_item);

   menu_item = gtk_menu_item_new_with_label ("Clear display");
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show (menu_item);
   gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
		       GTK_SIGNAL_FUNC (clear_display), (gpointer) display);

   menu_item = gtk_menu_item_new_with_label ("Display options ...");
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show (menu_item);
   gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
		       GTK_SIGNAL_FUNC (display_settings), (gpointer) display);

#ifdef XFIG   
   display->twfa_menu
      = menu_item = gtk_menu_item_new_with_label ("FIASCO bintree ...");
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show (menu_item);
   gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
		       GTK_SIGNAL_FUNC (twfa_settings), (gpointer) display);
   gtk_widget_set_sensitive (display->twfa_menu, NO);
#endif /* XFIG */
   
   /*
    *  HELP menu
    */
   menu      = gtk_menu_new();
   menu_item = gtk_menu_item_new_with_label ("Help");
   gtk_menu_item_right_justify (GTK_MENU_ITEM (menu_item));
   gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
   gtk_menu_bar_append (GTK_MENU_BAR (menu_bar), menu_item);
   gtk_widget_show (menu_item);

   menu_item = gtk_menu_item_new_with_label ("About ...");
   gtk_menu_append (GTK_MENU (menu), menu_item);
   gtk_widget_show (menu_item);
   gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
		       GTK_SIGNAL_FUNC (about_xfiasco), NULL);

   return menu_bar;
}

static GtkWidget *
status_container (xwfa_display_t *display)
/*
 *  Generate container widget with status information.
 *
 *  Return value:
 *	container widget
 */
{
   GtkWidget	*table;
   GtkWidget	*widget;
   int		n;

   display->status_widget = Calloc (STAT_NULL, sizeof (GtkWidget *));
   
   table = gtk_table_new (2, STAT_NULL, FALSE);
   gtk_table_set_row_spacings (GTK_TABLE (table), 0);
   gtk_table_set_col_spacings (GTK_TABLE (table), 5);
   gtk_container_border_width (GTK_CONTAINER (table), 5);

   for (n = 0; n < STAT_NULL; n++)
   {
      GtkWidget *label;
      char	*info_text [STAT_NULL] = {"Frame #", "Range state",
					  "Range label", "Position",
					  "Motion vector", "Frametype",
					  "Framerate", "States", "Frames"};
      bool_t    use_label [STAT_NULL] = {NO, NO, NO,
					 YES, YES, YES, YES, YES, YES};

      label = gtk_label_new (info_text [n]);
      gtk_widget_show (label);
      gtk_table_attach_defaults (GTK_TABLE (table), label, n, n + 1, 0, 1);

      if (use_label [n])
	 widget = gtk_label_new ("");
      else
      {
	 widget = gtk_entry_new ();
	 gtk_entry_set_text (GTK_ENTRY (widget), "");
	 if (n)
	    gtk_signal_connect (GTK_OBJECT(widget), "activate",
				(GtkSignalFunc) goto_range,
				(gpointer) display);
	 else
	    gtk_signal_connect (GTK_OBJECT(widget), "activate",
				(GtkSignalFunc) goto_frame,
				(gpointer) display);
	 gtk_widget_set_usize (widget, 50, -1);
      }

      gtk_table_attach (GTK_TABLE (table), widget, n, n + 1, 1, 2, 0, 0, 0, 0);
      gtk_widget_show (widget);
      display->status_widget [n] = widget;
   }
   
   gtk_widget_show (table);

   return table;
}

static GtkWidget *
image_container (xwfa_display_t *display)
/*
 *  Generate container for the decoded images.
 *
 *  Return value:
 *	pointer to the box containing the clickable areas.
 */
{
   GtkWidget    *frame;

   frame = gtk_frame_new ("Image Partitioning");
   gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_border_width (GTK_CONTAINER (frame), 5);
   gtk_widget_show (frame);

   display->image_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (display->image_window),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_container_border_width (GTK_CONTAINER (display->image_window), 5);
   gtk_container_add (GTK_CONTAINER (frame), display->image_window);
   gtk_widget_show (display->image_window);
   
   return frame;
}

GtkWidget *
basis_states_container (xwfa_display_t *display)
/*
 *  Generate basis images display container.
 *
 *  Return value:
 *	pointer to the box containing the basis images
 */
{
   GtkWidget *frame;

   frame = gtk_frame_new ("Initial Basis");
   gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_border_width (GTK_CONTAINER (frame), 5);
   gtk_widget_show (frame);

   display->basis_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (display->basis_window),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_container_border_width (GTK_CONTAINER (display->basis_window), 5);
   gtk_container_add (GTK_CONTAINER (frame), display->basis_window);
   gtk_widget_set_usize (display->basis_window, 115, -1);
   gtk_widget_show (display->basis_window);

   return frame;
}

GtkWidget *
lincomb_container (xwfa_display_t *display)
/*
 *  Generate linear combination display window.
 *
 *  Return value:
 *	pointer to the box containing the linear combination
 */
{
   GtkWidget *table, *frame;
   int	      n, p;

   display->lc_image = Calloc (MAXEDGES + 1, sizeof (GtkWidget *));
   display->lc_image [MAXEDGES] = NULL;

   display->lc_label = Calloc (MAXEDGES + 1, sizeof (GtkWidget *));
   display->lc_label [MAXEDGES] = NULL;

   frame = gtk_frame_new ("Linear Combination");
   gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_border_width (GTK_CONTAINER (frame), 5);
   gtk_widget_show (frame);

   table = gtk_table_new (2, MAXEDGES + 1 + 1, FALSE);
   gtk_table_set_row_spacings (GTK_TABLE (table), 0);
   gtk_table_set_col_spacings (GTK_TABLE (table), 5);
   gtk_container_border_width (GTK_CONTAINER (table), 5);
   gtk_container_add (GTK_CONTAINER (frame), table);
   gtk_widget_show (table);

   for (n = 0, p = 0; p < MAXEDGES + 1; n++, p++)
   {
      GtkWidget *frame;
	 
      if (p == 1)			/* hrule */
      {
	 GtkWidget *vruler;

	 vruler = gtk_vseparator_new ();
	 gtk_widget_show (vruler);
	 gtk_table_attach (GTK_TABLE (table), vruler, 1, 2, 0, 2,
			   GTK_FILL, GTK_FILL, 0, 0);
	 n++;
      }
      
      frame = gtk_frame_new (NULL);
      gtk_widget_show (frame);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_border_width (GTK_CONTAINER (frame), 0);
      gtk_table_attach (GTK_TABLE (table), frame, n, n + 1, 1, 2,
			GTK_EXPAND, GTK_EXPAND, 0, 0);

      display->lc_image [p] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
      gtk_preview_size (GTK_PREVIEW (display->lc_image [p]), 64, 64);
      gtk_container_add (GTK_CONTAINER (frame), display->lc_image [p]);
      gtk_widget_show (display->lc_image [p]);
      
      display->lc_label [p] = gtk_label_new ("");
      gtk_table_attach (GTK_TABLE (table), display->lc_label [p],
			n, n + 1, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0);
      gtk_widget_show (display->lc_label [p]);
   }

   return frame;
}

static GtkWidget *
toolbar_container (xwfa_display_t *display)
/*
 *  Generate toolbar.
 *
 *  Return value:
 *	toolbar widget
 */
{
   GtkWidget *toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
					 GTK_TOOLBAR_ICONS);

   gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			    NULL, "Exit.",
			    "Toolbar/Exit",
			    gtk_pixmap_new ((p_array + P_EXIT)->pixmap,
					    (p_array + P_EXIT)->mask),
			    (GtkSignalFunc) gtk_exit, NULL);
   gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
   gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			    NULL, "Display options.", "Toolbar/Display",
			    gtk_pixmap_new ((p_array + P_DISPLAY)->pixmap,
					    (p_array + P_DISPLAY)->mask),
			    (GtkSignalFunc) display_settings, display);
#ifdef XMAG   
   gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			    NULL, "Zoom.", "Toolbar/Zoom",
			    gtk_pixmap_new ((p_array + P_ZOOM)->pixmap,
					    (p_array + P_ZOOM)->mask),
			    (GtkSignalFunc) xmag, display);
#endif /* XMAG */
   
   gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
   display->prev_frame_button
      = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
				 NULL, "Previous frame.", "Toolbar/Previous",
				 gtk_pixmap_new ((p_array + P_PREV)->pixmap,
						 (p_array + P_PREV)->mask),
				 (GtkSignalFunc) prev_frame, display);
   gtk_widget_set_sensitive (display->prev_frame_button, NO);
   display->next_frame_button
      = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
				 NULL, "Next frame.", "Toolbar/Previous",
				 gtk_pixmap_new ((p_array + P_NEXT)->pixmap,
						 (p_array + P_NEXT)->mask),
				 (GtkSignalFunc) next_frame, display);
   gtk_widget_set_sensitive (display->next_frame_button, NO);

   gtk_widget_show_all (toolbar);
   return toolbar;
}

static void
xmag (GtkWidget *widget, gpointer ptr)
{
#ifdef XMAG   
   system (XMAG "&");
#endif /* XMAG */
   ;
}
