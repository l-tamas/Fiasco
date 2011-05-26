/*
 *  icons.h	
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/04 16:27:46 $
 *  $Author: hafner $
 *  $Revision: 4.1 $
 *  $State: Exp $
 */

#ifndef _ICONS_H
#define _ICONS_H

#include <stdio.h>
#include <gtk/gtk.h>

typedef struct pixmap
{
   GdkPixmap *pixmap;
   GdkBitmap *mask;
} pixmap_t;

enum xpms {P_EXIT, P_DISPLAY, P_PREV, P_NEXT, P_ZOOM, P_TWFA, P_LAST};
pixmap_t *p_array;

void
init_pixmaps (GtkWidget *window);

#endif /* not _ICONS_H */
