/*
 *  dialog.h
 *		
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/04 16:27:44 $
 *  $Author: hafner $
 *  $Revision: 4.1 $
 *  $State: Exp $
 */

#ifndef _DIALOG_H
#define _DIALOG_H

#include <gtk/gtk.h>

typedef enum dialog {DIALOG_INFO, DIALOG_QUESTION, DIALOG_WARNING,
		    DIALOG_ERROR} dialog_e;

void
dialog_popup (dialog_e type, const char *text,
	      void (*ok_function) (GtkWidget *, gpointer),
	      gpointer dataptr_ok_function);
gint
delete_window (const GtkWidget *widget, gpointer data);
void
destroy_window (GtkWidget  *widget, GtkWidget **window);
void
destroy_application (const GtkWidget *widget, gpointer data);
gint
hide_window (GtkWidget *widget);

#endif /* not _DIALOG_H */

