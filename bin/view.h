/*
 *  view.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/04 16:27:47 $
 *  $Author: hafner $
 *  $Revision: 4.2 $
 *  $State: Exp $
 */

#ifndef _VIEW_H
#define _VIEW_H

#include "xwfa.h"

void
display_settings (GtkWidget *widget, gpointer ptr);
void
prediction_sensitive (xwfa_display_t *display);
void
background_sensitive (xwfa_display_t *display);
void
twfa_settings (GtkWidget *widget, gpointer ptr);
void
twfa_flush (GtkWidget *widget, gpointer ptr);

#endif /* not _VIEW_H */

