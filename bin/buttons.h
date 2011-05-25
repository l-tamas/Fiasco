/*
 *  buttons.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:51:17 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#ifndef _BUTTONS_H
#define _BUTTONS_H

#ifndef X_DISPLAY_MISSING

typedef enum grayscale_e {BLACK, NGRAY, LGRAY, DGRAY, RED,
			  THICKBLACK, NO_GC} grayscale_t;
typedef enum buttons_e {STOP_BUTTON, PLAY_BUTTON, PAUSE_BUTTON, RECORD_BUTTON,
			QUIT_BUTTON, NO_BUTTON} buttons_t;

typedef struct buttoninfo
{
   Window   window;
   bool_t   pressed [NO_BUTTON];
   GC	    gc [NO_GC];
   unsigned width;
   unsigned height;
   unsigned progbar_height;
   bool_t   record_is_rewind;
} binfo_t;

void
check_events (x11_info_t *xinfo, binfo_t *binfo, unsigned n,
	      unsigned n_frames);
void
wait_for_input (x11_info_t *xinfo);
binfo_t * 
init_buttons (x11_info_t *xinfo, unsigned n, unsigned n_frames,
	      unsigned buttons_height, unsigned progbar_height);

#endif /* not X_DISPLAY_MISSING */

#endif /* not _BUTTONS_H */

