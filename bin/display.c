/*
 *  display.c:		X11 display of frames
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 *		
 *  Based on mpeg2decode, (C) 1994, MPEG Software Simulation Group
 *  and      mpeg2play,   (C) 1994 Stefan Eckart
 *                                 <stefan@lis.e-technik.tu-muenchen.de>
 *  and      tmndec       (C) 1995, 1996  Telenor R&D, Norway
 */

/*
 *  $Date: 2000/07/03 19:35:59 $
 *  $Author: hafner $
 *  $Revision: 5.2 $
 *  $State: Exp $
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#if STDC_HEADERS
#	include <stdlib.h>
#	include <string.h>
#else /* not STDC_HEADERS */
#	if HAVE_STRING_H
#		include <string.h>
#	else /* not HAVE_STRING_H */
#		include <strings.h>
#	endif /* not HAVE_STRING_H */
#endif /* not STDC_HEADERS */

#include "types.h"
#include "macros.h"
#include "display.h"
#include "binerror.h"

/*****************************************************************************

	       shared memory functions (if USE_SHM is defined)
  
*****************************************************************************/

#ifdef USE_SHM

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

int
XShmQueryExtension (Display *dpy);
int
XShmGetEventBase (Display *dpy);

static int
HandleXError (Display *dpy, XErrorEvent *event);
static void
InstallXErrorHandler (x11_info_t *xinfo);
static void
DeInstallXErrorHandler (x11_info_t *xinfo);

static int		shmem_flag;
static XShmSegmentInfo	shminfo1, shminfo2;
static int		gXErrorFlag;
static int		CompletionType = -1;

static int
HandleXError (Display *dpy, XErrorEvent *event)
{
   gXErrorFlag = 1;
   
   return 0;
}

static void
InstallXErrorHandler (x11_info_t *xinfo)
{
   XSetErrorHandler (HandleXError);
   XFlush (xinfo->display);
}

static void
DeInstallXErrorHandler (x11_info_t *xinfo)
{
   XSetErrorHandler (NULL);
   XFlush (xinfo->display);
}

#endif /* USE_SHM */

/*****************************************************************************

				public code
  
*****************************************************************************/

void
display_image (unsigned x0, unsigned y0, x11_info_t *xinfo)
/*
 *  Display 'image' at pos ('x0', 'y0') in the current window
 *  (given by 'xinfo->window'). 
 *
 *  No return value.
 */
{
   int byte_order_check = 1;
   
   /*
    *  Always work in native bit and byte order. This tells Xlib to
    *  reverse bit and byte order if necessary when crossing a
    *  network. Frankly, this part of XImages is somewhat
    *  underdocumented, so this may not be exactly correct.
    */
   if (*(char *) & byte_order_check == 1)
   {
      xinfo->ximage->byte_order       = LSBFirst;
      xinfo->ximage->bitmap_bit_order = LSBFirst;
   }
   else
   {
      xinfo->ximage->byte_order       = MSBFirst;
      xinfo->ximage->bitmap_bit_order = MSBFirst;
   }    

   /* Display dithered image */
#ifdef USE_SHM
   if (shmem_flag)
   {
      XEvent xev;
      
      XShmPutImage (xinfo->display, xinfo->window, xinfo->gc, xinfo->ximage,
		    0, 0, x0, y0, xinfo->ximage->width, xinfo->ximage->height,
		    True);
      XFlush (xinfo->display);
      
      while (!XCheckTypedEvent (xinfo->display, CompletionType, &xev))
	 ;
   }
   else 
#endif /* USE_SHM */
   {
      xinfo->ximage->data = (char *) xinfo->pixels; 
      XPutImage (xinfo->display, xinfo->window, xinfo->gc, xinfo->ximage, 0, 0,
		 x0, y0, xinfo->ximage->width, xinfo->ximage->height);
   }
}

void
close_window (x11_info_t *xinfo)
{
#ifdef USE_SHM
   if (shmem_flag && xinfo->ximage)
   {
      XShmDetach (xinfo->display, &shminfo1);
      XDestroyImage (xinfo->ximage);
      xinfo->ximage = NULL;
      shmdt (shminfo1.shmaddr);
   }
   else
#endif /* USE_SHM */
   if (xinfo->ximage)
   {
      XDestroyImage (xinfo->ximage);
      xinfo->ximage = NULL;
   }
   if (xinfo->display)
   {
      XCloseDisplay (xinfo->display);
      xinfo->display = NULL;
   }
}

x11_info_t *
open_window (const char *titlename, const char *iconname,
	     unsigned width, unsigned height)
/*
 *  Open a X11 window of size 'width'x'height'.
 *  If 'color' is false then allocate a colormap with grayscales.
 *  Window and icon titles are given by 'titlename' and 'iconname',
 *  respectively.
 *
 *  Return value:
 *	X11 info struct containing display, gc, window ID and colormap.
 */
{
   XVisualInfo		visual_template; /* template for XGetVisualInfo() */
   XVisualInfo		visual_info;	/* return value of XGetVisualInfo() */
   int			visual_n;	/* # of matches of XGetVisualInfo */
   XEvent		xev;		
   XSizeHints		hint;		
   XSetWindowAttributes xswa;		
   unsigned int		fg, bg;		/* foreground and background color */
   unsigned int		mask;		/* event mask */
   x11_info_t		*xinfo = calloc (1, sizeof (x11_info_t));
   long                 visual_mask;
   
   if (!xinfo)
      error ("Out of memory");
   /*
    *  Initialization of display
    */
   xinfo->display = XOpenDisplay (NULL);
   if (xinfo->display == NULL)
      error ("Can't open display.\n"
	     "Make sure that your environment variable DISPLAY "
	     "is set correctly.");

   xinfo->screen = DefaultScreen (xinfo->display);
   xinfo->gc     = DefaultGC (xinfo->display, xinfo->screen);

   {
      unsigned   depth [] 	= {32, 24, 16};
      int        class [] 	= {TrueColor, PseudoColor};
      const char *class_text [] = {"TrueColor", "PseudoColor"};
      Status     found 		= 0;
      unsigned   d, c;

      for (d = 0; !found && d < sizeof (depth) / sizeof (unsigned); d++)
	 for (c = 0; !found && c < sizeof (class) / sizeof (int); c++)
	 {
	    found = XMatchVisualInfo (xinfo->display, xinfo->screen,
				      depth [d], class [c], &visual_info);
	    if (found)
	       fprintf (stderr, "%s : %d bit colordepth.\n",
			class_text [c], depth [d]);
	 }
      if (!found && fiasco_get_verbosity ())
	 error ("Can't find a 16/24/32 bit TrueColor/DirectColor display");
   }
   
   /* Width and height of the display window */
   hint.x = hint.y = 0;
   hint.min_width  = hint.max_width  = hint.width  = width;
   hint.min_height = hint.max_height = hint.height = height;
   hint.flags = PSize | PMinSize | PMaxSize;

   /* Get some colors */
   bg = WhitePixel (xinfo->display, xinfo->screen);
   fg = BlackPixel (xinfo->display, xinfo->screen);

   /* Make the window */
   mask = CWBackPixel | CWBorderPixel;
   if (visual_info.depth >= 16)
   {
      mask |= CWColormap;
      xswa.colormap = XCreateColormap (xinfo->display,
				       DefaultRootWindow (xinfo->display),
				       visual_info.visual, AllocNone);
   }
   xswa.background_pixel = bg;
   xswa.border_pixel     = fg;
   xinfo->window = XCreateWindow (xinfo->display,
				  DefaultRootWindow (xinfo->display), 0, 0,
				  width, height, 1, visual_info.depth,
				  InputOutput, visual_info.visual,
				  mask, &xswa);

   XSelectInput (xinfo->display, xinfo->window, StructureNotifyMask);

   /* Tell other applications about this window */
   XSetStandardProperties (xinfo->display, xinfo->window, titlename, iconname,
			   None, NULL, 0, &hint);

   /* Map window. */
   XMapWindow (xinfo->display, xinfo->window);

   /* Wait for map. */
   do
   {
      XNextEvent (xinfo->display, &xev);
   }
   while (xev.type != MapNotify || xev.xmap.event != xinfo->window);

   /* Allocate colors */

   return xinfo;
}

void
alloc_ximage (x11_info_t *xinfo, unsigned width, unsigned height)
/*
 *  Allocate ximage of size 'width'x'height'.
 *  If USE_SHM is defined then use shared memory extensions.
 *
 *  No return value.
 *
 *  Side effects:
 *	'ximage->ximage' and 'ximage->pixels' are set to useful values.
 */
{
   char dummy;
   
#ifdef USE_SHM
   if (XShmQueryExtension(xinfo->display))
   {
      if (fiasco_get_verbosity ())
	 fprintf (stderr, "Trying shared memory.\n");
      shmem_flag = 1;
   }
   else
   {
      shmem_flag = 0;
      if (fiasco_get_verbosity ())
	 fprintf (stderr,
		  "Shared memory not supported\nReverting to normal Xlib.\n");
   }

   if (shmem_flag)
      CompletionType = XShmGetEventBase (xinfo->display) + ShmCompletion;

   InstallXErrorHandler (xinfo);

   if (shmem_flag)
   {
      xinfo->ximage = XShmCreateImage (xinfo->display,
				       DefaultVisual (xinfo->display,
						      xinfo->screen),
				       DefaultDepth (xinfo->display,
						     xinfo->screen), ZPixmap,
				       NULL, &shminfo1, width, height);

      /* If no go, then revert to normal Xlib calls. */

      if (xinfo->ximage == NULL)
      {
	 if (fiasco_get_verbosity ())
	    fprintf (stderr,
		     "Shared memory error, disabling (Ximage error).\n");
	 goto shmemerror;
      }

      /* Success here, continue. */

      shminfo1.shmid = shmget (IPC_PRIVATE, xinfo->ximage->bytes_per_line
			       * xinfo->ximage->height, IPC_CREAT | 0777);

      if (shminfo1.shmid < 0)
      {
	 XDestroyImage (xinfo->ximage);
	 if (fiasco_get_verbosity ())
	    fprintf (stderr,
		     "Shared memory error, disabling (seg id error).\n");
	 goto shmemerror;
      }

      shminfo1.shmaddr = (char *) shmat (shminfo1.shmid, 0, 0);
      shminfo2.shmaddr = (char *) shmat (shminfo2.shmid, 0, 0);

      if (shminfo1.shmaddr == ((char *) -1))
      {
	 XDestroyImage (xinfo->ximage);
	 if (shminfo1.shmaddr != ((char *) -1))
	    shmdt (shminfo1.shmaddr);
	 if (fiasco_get_verbosity ())
	    fprintf (stderr,
		     "Shared memory error, disabling (address error).\n");
	 goto shmemerror;
      }

      xinfo->ximage->data = shminfo1.shmaddr;
      xinfo->pixels       = (byte_t *) xinfo->ximage->data;
      shminfo1.readOnly   = False;

      XShmAttach (xinfo->display, &shminfo1);
      XSync (xinfo->display, False);

      if (gXErrorFlag)
      {
	 /* Ultimate failure here. */
	 XDestroyImage (xinfo->ximage);
	 shmdt (shminfo1.shmaddr);
	 if (fiasco_get_verbosity ())
	    fprintf (stderr, "Shared memory error, disabling.\n");
	 gXErrorFlag = 0;
	 goto shmemerror;
      }
      else
	 shmctl (shminfo1.shmid, IPC_RMID, 0);
      if (fiasco_get_verbosity ())
	 fprintf (stderr, "Sharing memory.\n");
   }
   else
   {
     shmemerror:
      shmem_flag = 0;
#endif /* USE_SHM */

      xinfo->ximage = XCreateImage (xinfo->display,
				    DefaultVisual (xinfo->display,
						   xinfo->screen),
				    DefaultDepth (xinfo->display,
						  xinfo->screen),
				    ZPixmap, 0, &dummy, width, height, 8, 0);
      xinfo->pixels = calloc (width * height,
			      xinfo->ximage->depth <= 8
			      ? sizeof (byte_t)
			      : (xinfo->ximage->depth <= 16
				 ? sizeof (u_word_t) : sizeof (unsigned int)));
      if (!xinfo->pixels)
	 error ("Out of memory.");
    
#ifdef USE_SHM
   }

   DeInstallXErrorHandler (xinfo);
#endif /* USE_SHM */
}

#endif /* not X_DISPLAY_MISSING */
