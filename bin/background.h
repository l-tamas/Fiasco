/*
 *  background.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/04 16:27:41 $
 *  $Author: hafner $
 *  $Revision: 4.1 $
 *  $State: Exp $
 */

#ifndef _BACKGROUND_H
#define _BACKGROUND_H

#include "types.h"
#include "xwfa.h"
#include <gtk/gtk.h>

void
generate_wfa_backgrounds (xwfa_display_t *display);
void
generate_image_backgrounds (char *image_name, image_t **bg_image);
bool_t
generate_backgrounds (video_t *video, background_e *bg);
void
draw_background (background_e bg_type, image_t **bg_image, int number,
		 GtkWidget *preview);
void
draw_grayscale (GtkPreview *preview, image_t *image, int band);

#endif /* not _BACKGROUND_H */

