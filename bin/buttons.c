/*
 *  buttons.c:		Draw MWFA player buttons in X11 window	
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/15 17:23:11 $
 *  $Author: hafner $
 *  $Revision: 5.2 $
 *  $State: Exp $
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#if STDC_HEADERS
#	include <stdlib.h>
#endif /* not STDC_HEADERS */

#include "types.h"
#include "macros.h"

#include "display.h"
#include "binerror.h"
#include "buttons.h"

/*****************************************************************************

			     local variables
  
*****************************************************************************/

static const int EVENT_MASK = (KeyPressMask | ButtonPressMask |
			       ButtonReleaseMask | ExposureMask);

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
draw_progress_bar (x11_info_t *xinfo, binfo_t *binfo, unsigned n,
		   unsigned n_frames);
static void
draw_button (x11_info_t *xinfo, binfo_t *binfo,
	     buttons_t button, bool_t pressed);
static void
draw_control_panel (x11_info_t *xinfo, binfo_t *binfo,
		    unsigned n, unsigned n_frames);

/*****************************************************************************

				public code
  
*****************************************************************************/

binfo_t * 
init_buttons (x11_info_t *xinfo, unsigned n, unsigned n_frames,
	      unsigned buttons_height, unsigned progbar_height)
/*
 *  Initialize a toolbar with the typical collection of video player
 *  buttons (pause, play, record, next, etc.) in the window given by 'xinfo'.
 *  'n' gives the current frame, 'whereas' n_frames is the total number of
 *  frames of the video stream.
 *  The size of the button toolbar is given by 'buttons_height',
 *  the size of the progressbar is given by 'progbar_height'.
 *
 *  Return value:
 *	struct managing the toolbar and progressbar information
 */
{
   XGCValues  values;
   XEvent     event;
   Colormap   cmap;
   XColor     gray, dgray, lgray, red;
   XColor     graye, dgraye, lgraye, rede;
   buttons_t  button;			/* counter */
   binfo_t   *binfo = calloc (1, sizeof (binfo_t));

   if (!binfo)
      error ("Out of memory.");
   
   binfo->width            = xinfo->ximage->width;
   binfo->height           = buttons_height;
   binfo->progbar_height   = progbar_height;
   binfo->record_is_rewind = NO;

   /*
    *  Generate sub-window for control panel
    */
   binfo->window = XCreateSimpleWindow (xinfo->display, xinfo->window,
					0, xinfo->ximage->height,
					binfo->width, binfo->height, 0,
					BlackPixel (xinfo->display,
						    xinfo->screen),
					WhitePixel (xinfo->display,
						    xinfo->screen));
   XSelectInput(xinfo->display, binfo->window, StructureNotifyMask);
   XMapWindow (xinfo->display, binfo->window);
   do
   {
      XNextEvent (xinfo->display, &event);
   }
   while (event.type != MapNotify || event.xmap.event != binfo->window);
   XSelectInput (xinfo->display, binfo->window, EVENT_MASK);

   /*
    *  Generate graphic contexts for different colors.
    */
   cmap = DefaultColormap (xinfo->display, xinfo->screen);
   XAllocNamedColor (xinfo->display, cmap, "#404040", &dgray, &dgraye);
   XAllocNamedColor (xinfo->display, cmap, "white", &lgray, &lgraye);
   XAllocNamedColor (xinfo->display, cmap, "#a8a8a8", &gray, &graye);
   XAllocNamedColor (xinfo->display, cmap, "red", &red, &rede);
   
   values.foreground = BlackPixel (xinfo->display, xinfo->screen);
   values.background = WhitePixel (xinfo->display, xinfo->screen);
   binfo->gc [BLACK] = XCreateGC (xinfo->display,
				  RootWindow (xinfo->display, xinfo->screen),
				  (GCForeground | GCBackground), &values);
   values.foreground = BlackPixel (xinfo->display, xinfo->screen);
   values.background = WhitePixel (xinfo->display, xinfo->screen);
   values.line_width = 3;
   values.join_style = JoinRound;
   binfo->gc [THICKBLACK] = XCreateGC (xinfo->display,
				       RootWindow (xinfo->display,
						   xinfo->screen),
				       (GCForeground | GCBackground
					| GCLineWidth | GCJoinStyle), &values);
   values.foreground = gray.pixel;
   values.background = WhitePixel (xinfo->display, xinfo->screen);
   binfo->gc [NGRAY] = XCreateGC (xinfo->display,
				  RootWindow (xinfo->display, xinfo->screen),
				  (GCForeground | GCBackground), &values);
   values.foreground = lgray.pixel;
   values.background = WhitePixel (xinfo->display, xinfo->screen);
   binfo->gc [LGRAY] = XCreateGC (xinfo->display,
				  RootWindow (xinfo->display, xinfo->screen),
				  (GCForeground | GCBackground), &values);
   values.foreground = dgray.pixel;
   values.background = WhitePixel (xinfo->display, xinfo->screen);
   binfo->gc [DGRAY] = XCreateGC (xinfo->display,
				  RootWindow (xinfo->display, xinfo->screen),
				  (GCForeground | GCBackground), &values);
   values.foreground = red.pixel;
   values.background = WhitePixel (xinfo->display, xinfo->screen);
   binfo->gc [RED]   = XCreateGC (xinfo->display,
				  RootWindow (xinfo->display, xinfo->screen),
				  (GCForeground | GCBackground), &values);

   for (button = 0; button < NO_BUTTON; button++)
      binfo->pressed [button] = NO;

   draw_control_panel (xinfo, binfo, n, n_frames); 
   
   return binfo;
}

void
wait_for_input (x11_info_t *xinfo)
/*
 *  Wait for key press or mouse click in window 'xinfo'.
 *  Redraw 'image' if event other then ButtonPress or KeyPress occurs.
 *  Enlarge or reduce size of image by factor 2^'enlarge_factor'.
 *
 *  No return value.
 *
 *  Side effect:
 *	program is terminated after key press or mouse click.
 */
{
   bool_t leave_loop = NO;
   
   XSelectInput (xinfo->display, xinfo->window, EVENT_MASK);

   while (!leave_loop)
   {
      XEvent event;

      XMaskEvent (xinfo->display, EVENT_MASK, &event);
      switch (event.type)
      {
	 case ButtonPress:
	 case KeyPress:
	    leave_loop = YES;
	    break;
	 default:
	    display_image (0, 0, xinfo);
	    break;
      }
   }
}

void
check_events (x11_info_t *xinfo, binfo_t *binfo, unsigned n, unsigned n_frames)
/*
 *  Check the X11 event loop. If the PAUSE buttonin the of panel 'binfo'
 *  is activated wait until next event occurs.
 *  Redraw 'image' if event other then ButtonPress or ButtonRelease occurs.
 *  Enlarge or reduce size of image by factor 2^'enlarge_factor'.
 *  'n' gives the current frame, 'whereas' n_frames is the total number of
 *  frames of the video stream.
 *
 *  No return values.
 *
 *  Side effects:
 *	status of buttons (binfo->pressed [button]) is changed accordingly.
 */
{
   bool_t leave_eventloop;

   leave_eventloop = (!binfo->pressed [PAUSE_BUTTON]
		      && binfo->pressed [PLAY_BUTTON])
		     || (!binfo->pressed [PAUSE_BUTTON]
			 && binfo->record_is_rewind
			 && binfo->pressed [RECORD_BUTTON])
		     || binfo->pressed [RECORD_BUTTON];
   draw_progress_bar (xinfo, binfo, n, n_frames);

   if (binfo->pressed [PAUSE_BUTTON] && binfo->pressed [PLAY_BUTTON])
   {
      XFlush (xinfo->display);
      draw_button (xinfo, binfo, PLAY_BUTTON, NO); /* clear PLAY mode */
      XFlush (xinfo->display);
   }
   if (binfo->pressed [PAUSE_BUTTON]
       && binfo->record_is_rewind && binfo->pressed [RECORD_BUTTON])
   {
      XFlush (xinfo->display);
      draw_button (xinfo, binfo, RECORD_BUTTON, NO); /* clear PLAY mode */
      XFlush (xinfo->display);
   }

   if (binfo->pressed [STOP_BUTTON])
   {
      XFlush (xinfo->display);
      draw_button (xinfo, binfo, STOP_BUTTON, NO); /* clear STOP button */
      XFlush (xinfo->display);
   }

   do
   {
      XEvent event;
      int    button;
      bool_t wait_release = NO;
	 
      
      if (XCheckMaskEvent (xinfo->display, EVENT_MASK, &event))
      {
	 switch (event.type)
	 {
	    case ButtonPress:
	       wait_release = NO;
	       if (!(binfo->pressed [RECORD_BUTTON] &&
		     !binfo->record_is_rewind))
		  for (button = 0; button < NO_BUTTON; button++)
		  {
		     int x0, y0, x1, y1; /* button coordinates */
		  
		     x0 = button * (binfo->width / NO_BUTTON);
		     y0 = binfo->progbar_height;
		     x1 = x0 + binfo->width / NO_BUTTON;
		     y1 = y0 + binfo->height - binfo->progbar_height - 1;
		     if (event.xbutton.x > x0 && event.xbutton.x < x1
			 && event.xbutton.y > y0 && event.xbutton.y < y1) 
		     {
			draw_button (xinfo, binfo, button,
				     !binfo->pressed [button]);
			wait_release = YES;
			break;
		     }
		  }
	       break;
	    case ButtonRelease:
	       wait_release = NO;
	       break;
	    default:
	       wait_release = NO;
	       draw_control_panel (xinfo, binfo, n, n_frames);
	       display_image (0, 0, xinfo);
	       break;
	 }
	 leave_eventloop = !wait_release
			   && (binfo->pressed [PLAY_BUTTON]
			       || binfo->pressed [STOP_BUTTON]
			       || binfo->pressed [RECORD_BUTTON]
			       || binfo->pressed [QUIT_BUTTON]);
      }
   } while (!leave_eventloop);

   if ((binfo->pressed [RECORD_BUTTON] && !binfo->record_is_rewind)
       && n == n_frames - 1)
   {
      binfo->record_is_rewind = YES;
      draw_button (xinfo, binfo, RECORD_BUTTON, NO);
   }
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
draw_control_panel (x11_info_t *xinfo, binfo_t *binfo,
		    unsigned n, unsigned n_frames)
/*
 *  Draw control panel 'binfo' with all buttons and progressbar in
 *  the given 'window'.
 *  'n' gives the current frame, 'whereas' n_frames is the total number of
 *  frames of the video stream.
 *
 *  No return value.
 */
{
   buttons_t button;
   
   XFillRectangle (xinfo->display, binfo->window, binfo->gc [NGRAY],
		   0, 0, binfo->width, binfo->height);
   draw_progress_bar (xinfo, binfo, n, n_frames);
   for (button = 0; button < NO_BUTTON; button++)
      draw_button (xinfo, binfo, button, binfo->pressed [button]);
}

static void
draw_progress_bar (x11_info_t *xinfo, binfo_t *binfo, unsigned n,
		   unsigned n_frames)
/*
 *  Draw progressbar of control panel 'binfo' in the given 'window'.
 *  'n' gives the current frame, whereas 'n_frames' is the total number of
 *  frames of the video stream.
 *
 *  No return value.
 */
{
   unsigned x, y, width, height;

   x 	  = 2;
   y 	  = 1;
   width  = binfo->width - 5;
   height = binfo->progbar_height - 3;
   
   XDrawLine (xinfo->display, binfo->window, binfo->gc [DGRAY],
	      x, y, x + width, y);
   XDrawLine (xinfo->display, binfo->window, binfo->gc [DGRAY],
	      x, y, x, y + height - 1);
   XDrawLine (xinfo->display, binfo->window, binfo->gc [LGRAY],
	      x + width, y + 1, x + width, y + height);
   XDrawLine (xinfo->display, binfo->window, binfo->gc [LGRAY],
	      x, y + height, x + width, y + height);

   x++; y++; width  -= 2; height -= 2;
   XFillRectangle (xinfo->display, binfo->window, binfo->gc [NGRAY],
		   x, y, width, height);

   XFillRectangle (xinfo->display, binfo->window, binfo->gc [BLACK],
		   x + n * max (1, width / n_frames), y,
		   max (1, width / n_frames), height);
}

static void
draw_button (x11_info_t *xinfo, binfo_t *binfo,
	     buttons_t button, bool_t pressed)
/*
 *  Draw 'button' of control panel 'binfo' in the given 'window'.
 *  'pressed' indicates whether the button is pressed or not.
 *
 *  No return value.
 */
{
   grayscale_t top, bottom;		/* index of GC */
   unsigned    x, y, width, height;	/* coordinates of button */
   
   x 	  = button * (binfo->width / NO_BUTTON);
   y 	  = binfo->progbar_height;
   width  = binfo->width / NO_BUTTON;
   height = binfo->height - binfo->progbar_height - 1;
   
   if (width < 4 || height < 4)
      return;
   
   if (pressed)
   {
      top    = DGRAY;
      bottom = LGRAY;
   }
   else
   {
      top    = LGRAY;
      bottom = DGRAY;
   }

   x 	 += 2;
   width -= 4;
   
   XDrawLine (xinfo->display, binfo->window, binfo->gc [top],
	      x, y, x + width, y);
   XDrawLine (xinfo->display, binfo->window, binfo->gc [top],
	      x, y, x, y + height - 1);
   XDrawLine (xinfo->display, binfo->window, binfo->gc [bottom],
	      x + width, y + 1, x + width, y + height);
   XDrawLine (xinfo->display, binfo->window, binfo->gc [bottom],
	      x, y + height, x + width, y + height);

   x++; y++; width  -= 2; height -= 2;
   XFillRectangle (xinfo->display, binfo->window, binfo->gc [NGRAY],
		   x, y, width, height);

   switch (button)
   {
      case STOP_BUTTON:
	 XFillRectangle (xinfo->display, binfo->window, binfo->gc [BLACK],
			 x + width / 2 - 6, y + height / 2 - 4, 11, 11);
	 if (pressed && !binfo->pressed [STOP_BUTTON])
	 {
	    draw_button (xinfo, binfo, PLAY_BUTTON, NO);
	    draw_button (xinfo, binfo, PAUSE_BUTTON, NO); 
	    draw_button (xinfo, binfo, RECORD_BUTTON, NO); 
	 }
	 break;
      case PAUSE_BUTTON:
	 XFillRectangle (xinfo->display, binfo->window, binfo->gc [BLACK],
			 x + width / 2 - 6, y + height / 2 - 4, 5, 11);
	 XFillRectangle (xinfo->display, binfo->window, binfo->gc [BLACK],
			 x + width / 2 + 1, y + height / 2 - 4, 5, 11);
	 break;
      case PLAY_BUTTON:
	 {
	    XPoint triangle [3];

	    triangle [0].x = x + width / 2 - 5;
	    triangle [0].y = y + height / 2 - 5;
	    triangle [1].x = 10;
	    triangle [1].y = 6;
	    triangle [2].x = -10;
	    triangle [2].y = 6;

	    XFillPolygon (xinfo->display, binfo->window, binfo->gc [BLACK],
			  triangle, 3, Convex, CoordModePrevious);
	    if (pressed && !binfo->pressed [PLAY_BUTTON]
		&& binfo->pressed [RECORD_BUTTON])
	       draw_button (xinfo, binfo, RECORD_BUTTON, NO);
	 }
	 break;
      case RECORD_BUTTON:
	 if (!binfo->record_is_rewind)
	 {
	    XFillArc (xinfo->display, binfo->window, binfo->gc [RED],
		      x + width / 2 - 5, y + height / 2 - 5, 11, 11, 0,
		      360 * 64);
	    if (pressed && !binfo->pressed [RECORD_BUTTON])
	    {
	       draw_button (xinfo, binfo, STOP_BUTTON, YES);
	       draw_button (xinfo, binfo, PLAY_BUTTON, NO);
	       draw_button (xinfo, binfo, PAUSE_BUTTON, NO); 
	    }
	 }
	 else
	 {
	    XPoint triangle [3];

	    triangle [0].x = x + width / 2 + 5;
	    triangle [0].y = y + height / 2 - 5;
	    triangle [1].x = -10;
	    triangle [1].y = 6;
	    triangle [2].x = 10;
	    triangle [2].y = 6;

	    XFillPolygon (xinfo->display, binfo->window, binfo->gc [BLACK],
			  triangle, 3, Convex, CoordModePrevious);
	    if (pressed && !binfo->pressed [RECORD_BUTTON]
		&& binfo->pressed [PLAY_BUTTON])
	       draw_button (xinfo, binfo, PLAY_BUTTON, NO);
	 }
	 break;
      case QUIT_BUTTON:
	 {
	    XPoint triangle [3];

	    triangle [0].x = x + width / 2 - 6;
	    triangle [0].y = y + height / 2 + 2;
	    triangle [1].x = 6;
	    triangle [1].y = -7;
	    triangle [2].x = 6;
	    triangle [2].y = 7;

	    XFillPolygon (xinfo->display, binfo->window, binfo->gc [BLACK],
			  triangle, 3, Convex, CoordModePrevious);
	    XFillRectangle (xinfo->display, binfo->window, binfo->gc [BLACK],
			    x + width / 2 - 5, y + height / 2 + 4, 11, 3);
	 }
	 break;
      default:
	 break;
   }
   binfo->pressed [button] = pressed;
}

#endif /* not X_DISPLAY_MISSING */
