/*
 *  xwfa.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/04 17:14:18 $
 *  $Author: hafner $
 *  $Revision: 4.3 $
 *  $State: Exp $
 */

#ifndef _XWFA_H
#define _XWFA_H

#include <gtk/gtk.h>
#include "types.h"
#include "image.h"
#include "bit-io.h"
#include "decoder.h"

typedef enum background {BG_NONE, BG_WFA, BG_ORIGINAL, BG_DIFF,
			 BG_PREDICTION, BG_DELTA, BG_NULL} background_e;
typedef enum motion {MO_RANGE, MO_BOTH, MO_REFERENCE, MO_VECTOR} motion_e;
typedef enum grid {GRID_RANGE, GRID_ND, GRID_FORWARD, GRID_BACKWARD,
		   GRID_INTERPOLATED, GRID_NULL} grid_e;
typedef enum tree_opt {TWFA_ROOT, TWFA_DEPTH, TWFA_GRID, TWFA_CGRID,
		       TWFA_STATES,
		       TWFA_STATE_NUM, TWFA_PRUNE, TWFA_BASIS, TWFA_LC,
		       TWFA_LC_LABEL, TWFA_LC_LIST, TWFA_BLC, TWFA_BLC_LABEL,
		       TWFA_BLC_LIST, TWFA_SHADOWS, TWFA_KEY, TWFA_LEVELS,
		       TWFA_NULL} tree_opt_e;
typedef enum status {STAT_FNO, STAT_STATE, STAT_LABEL, STAT_POS, STAT_MVEC,
		     STAT_TYPE, STAT_RATE, STAT_STATES, STAT_FRAMES,
		     STAT_NULL} status_e;

typedef struct xwfa_display
{
   GtkWidget	 *root_window;		/* root window widget */
   GtkWidget	 *basis_window;		/* window displaying all basis images */
   GtkWidget	**basis_image;		/* array of previews for basis display */
   GtkWidget    **lc_image;		/* array of previews for lc display */
   GtkWidget    **lc_label;		/* array of labels for lc display */
   GtkWidget	 *click_areas [4];	/* on clickable area for each band */
   GtkWidget	**status_widget;	/* status information */
#if 0   
   GtkWidget	 *statistics;		/* View->Statistics */
#endif
   GtkWidget	 *twfa_menu;		/* View->WFA bintree */
   GtkWidget	 *load_image_menu_item;	/* File->Load image */
   GtkWidget	 *prev_frame_menu_item;	/* File->Previous frame */
   GtkWidget	 *next_frame_menu_item;	/* File->Next frame */
   GtkWidget	 *prev_frame_button;	/* Previous frame toolbar button */
   GtkWidget	 *next_frame_button;	/* Next frame toolbar button */
   GtkWidget	 *image_window;		/* container for clickable areas */
   GtkWidget	 *pred_button [GRID_NULL + 1]; /* array of toggle buttons
					   for predicition settings */
   GtkWidget	 *twfa [TWFA_NULL];	/* array of toggle buttons
					   for twfa settings */
   GtkWidget	 *hl_button [3];		/* array of toggle buttons
					   for highlighting */
   GtkWidget	 *bg_button [BG_NULL];	/* array of radio buttons
					   for background settings */
   GtkWidget	 *basis;		/* Basis states container */
   GtkWidget	 *lc;			/* LC container */
   GtkWidget	 *toolbar;		/* Toolbar container */
   GtkWidget	 *status;		/* Status container */
   image_t	 *bg_image [BG_NULL];	/* array of background images */
   char		 *image_path;		/* path of original image */
   char		 *wfa_path;		/* path of WFA */
   video_t	 *video;		/* WFA decoder struct */
   wfa_t	 *wfa;			/* WFA struct */
#if 0   
   wfa_t	 *statistic_wfa;	/* WFA used for statistics */
#endif
   bitfile_t	 *input;		/* bitfile of current WFA */
   background_e	  background;		/* current background */
   motion_e	  motion_display;	/* type of motion vactor display */
   bool_t	  show_grid [GRID_NULL]; /* display grid 'i' ? */
   bool_t	  automatic_highlighting [3]; /* highlighting of ranges, domains? */
   real_t	  smoothing_factor;	/* smoothing factor in WFA decoding */
   int		  enlarge_factor;	/* enlarge image 2^n times */
   bool_t	  lc_prediction;	/* WFA coded with lc prediction */
   int		  frame_n;		/* current frame number */
   real_t	  bpp;			/* rate of current WFA */
   int		  mx0, mx1, my0, my1;	/* coordinates of region */
   bool_t	  region_active;	/* region drawing mode active ? */
   bool_t	  region_set;		/* region has been set ? */
   int		  root_state;		/* root state of region */
   int		  region_band;		/* color band of region */
} xwfa_display_t;

#endif /* not _XWFA_H */

