/*
 *  drawing.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/04 16:27:45 $
 *  $Author: hafner $
 *  $Revision: 4.1 $
 *  $State: Exp $
 */

#ifndef _DRAWING_H
#define _DRAWING_H

#include "types.h"
#include "wfa.h"
#include "xwfa.h"

typedef enum colors {GC_RANGE, GC_DOMAIN, GC_ND, GC_FORWARD,
		     GC_BACKWARD, GC_INTERPOLATED,
		     GC_MARKER, GC_NULL} color_type_e;

void
draw_lc_components (int state, int label, bool_t use_factor,
		    const wfa_t *orig_wfa, const xwfa_display_t *display);
void
draw_ranges (bool_t draw, int band, xwfa_display_t *display);
void
draw_nd_prediction (bool_t draw, int motion, int band, xwfa_display_t *display);
void
draw_basis_images (const wfa_t *orig_wfa, xwfa_display_t *display);
void
force_basis_redraw (int basis_states, xwfa_display_t *display);
void
init_colors (GtkWidget *preview);
void
refresh_highlighting (int band, const wfa_t *wfa, xwfa_display_t *display);
void
highlight (int state, int label, const bool_t *draw, const wfa_t *wfa,
	   xwfa_display_t *display);
void
get_prediction (int state, int label, const wfa_t *wfa, int *pstate, int *plabel);
void
clear_current_range (void);
void
preview_draw_rec (GtkWidget *preview, color_type_e color, int x0, int y0,
		  int width, int height);
void
draw_state_child (int state, int label, const wfa_t *wfa, color_type_e color,
		  GtkWidget *preview, xwfa_display_t *display);
void
preview_restore_area (GtkWidget *preview, int x0, int y0, int width, int height);

#endif /* not _DRAWING_H */

