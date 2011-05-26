/*
 *  callbacks.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/15 17:23:42 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#ifndef _CALLBACKS_H
#define _CALLBACKS_H

#include <gtk/gtk.h>
#include "xwfa.h"


void
image_file_selection (GtkWidget *widget, gpointer ptr);
void
wfa_file_selection (GtkWidget *widget, gpointer ptr);
void
load_next_frame (GtkWidget *widget, gpointer ptr);
void
clear_display (GtkWidget *widget, gpointer data);
gint
goto_range (GtkWidget *widget, gpointer dummy);
void
load_wfa (GtkWidget *w, gpointer ptr);
void
goto_frame (GtkWidget *widget, gpointer ptr);
void
next_frame (GtkWidget *widget, gpointer ptr);
void
prev_frame (GtkWidget *widget, gpointer ptr);

#endif /* not _CALLBACKS_H */

