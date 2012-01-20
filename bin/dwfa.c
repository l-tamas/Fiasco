/*
 *  dwfa.c:		Decoding of WFA-files
 *
 *  Written by:		Ullrich Hafner
 *			Michael Unger
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */
 
/*
 *  $Date: 2000/10/28 17:39:29 $
 *  $Author: hafner $
 *  $Revision: 5.7 $
 *  $State: Exp $
 */

#include "config.h"

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
#include <math.h>

#include "types.h"
#include "macros.h"

#include <getopt.h>

#include "binerror.h"
#include "misc.h"
#include "params.h"
#include "fiasco.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static int 
checkargs (int argc, char **argv, bool_t *double_resolution, bool_t *panel,
	   int *fps, char **image_name, fiasco_d_options_t **options);
static void
video_decoder (const char *wfa_name, const char *image_name, bool_t panel,
	       bool_t double_resolution, int fps, fiasco_d_options_t *options);
static void
get_output_template (const char *image_name, const char *wfa_name,
		     bool_t color, char **basename, char **suffix);

/*****************************************************************************

				public code
  
*****************************************************************************/

int 
main (int argc, char **argv)
{
   char	    	      *image_name        = NULL; /* output filename */
   bool_t    	       double_resolution = NO; /* double resolution of image */
   bool_t    	       panel             = NO; /* control panel */
   int  	       fps               = -1; /* frame display rate */
   fiasco_d_options_t *options 	       	 = NULL; /* additional coder options */
   int	     	       last_arg;	/* last processed cmdline parameter */

   init_error_handling (argv[0]);

   last_arg = checkargs (argc, argv, &double_resolution, &panel, &fps,
			 &image_name, &options);
   
   if (last_arg >= argc)
      video_decoder ("-", image_name, panel, double_resolution, fps, options);
   else
      while (last_arg++ < argc)
	 video_decoder (argv [last_arg - 1], image_name, panel,
			double_resolution, fps, options);

   return 0;
}

/*****************************************************************************

				private code
  
*****************************************************************************/

param_t params [] =
{
  {"output", "FILE", 'o', PSTR, {0}, "-",
   "Write raw PNM frame(s) to `%s'."},
  {"double", NULL, 'd', PFLAG, {0}, "FALSE",
   "Interpolate images to double size before display."},
  {"fast", NULL, 'r', PFLAG, {0}, "FALSE",
   "Use 4:2:0 format for fast, low quality output."},
  {"panel", NULL, 'p', PFLAG, {0}, "FALSE",
   "Display control panel."},
  {"magnify", "NUM", 'm', PINT, {0}, "0",
   "Magnify/reduce image size by a factor of 4^`%s'."},
  {"framerate", "NUM", 'F', PINT, {0}, "-1",
   "Set display rate to `%s' frames per second."},
  {"smoothing", "NUM", 's', PINT, {0}, "-1",
   "Smooth image(s) by factor `%s' (0-100)"},
  {NULL, NULL, 0, 0, {0}, NULL, NULL }
};

static int 
checkargs (int argc, char **argv, bool_t *double_resolution, bool_t *panel,
	   int *fps, char **image_name, fiasco_d_options_t **options)
/*
 *  Check validness of command line parameters and of the parameter files.
 *
 *  Return value.
 *	index in argv of the first argv-element that is not an option.
 *
 *  Side effects:
 *	'double_resolution', 'panel', 'fps', 'image_name' and 'options'
 *      are modified.
 */
{
   int optind;				/* last processed commandline param */

   optind = parseargs (params, argc, argv,
		       "Decode FIASCO-FILEs and write frame(s) to disk.",
		       "With no FIASCO-FILE, or if FIASCO-FILE is -, "
		       "read standard input.\n"
		       "Environment:\n"
		       "FIASCO_DATA   Search path for automata files. "
		       "Default: ./\n"
		       "FIASCO_IMAGES Save path for image files. "
		       "Default: ./", " [FIASCO-FILE]...",
		       FIASCO_SHARE, "system.fiascorc", ".fiascorc");

   *image_name        =   (char *)   parameter_value (params, "output");
   *double_resolution = *((bool_t *) parameter_value (params, "double"));
   *panel             = *((bool_t *) parameter_value (params, "panel"));
   *fps		      = *((int *)    parameter_value (params, "framerate"));

   /*
    *  Additional options ... (have to be set with the fiasco_set_... methods)
    */
   *options = fiasco_d_options_new ();

   {
      int n = *((int *) parameter_value (params, "smoothing"));
      
      if (!fiasco_d_options_set_smoothing (*options, max (-1, n)))
	 error (fiasco_get_error_message ());
   }

   {
      int n = *((int *) parameter_value (params, "magnify"));
      
      if (!fiasco_d_options_set_magnification (*options, n))
	 error (fiasco_get_error_message ());
   }
   
   {
      bool_t n = *((bool_t *) parameter_value (params, "fast"));
      
      if (!fiasco_d_options_set_4_2_0_format (*options, n > 0 ? YES : NO))
	 error (fiasco_get_error_message ());
   }

   return optind;
}

static void
video_decoder (const char *wfa_name, const char *image_name, bool_t panel,
	       bool_t double_resolution, int fps, fiasco_d_options_t *options)
{
   do
   {
      unsigned  	width, height, frames, n;
      fiasco_decoder_t *decoder_state;
      char     	       *filename;
      char     	       *basename;	/* basename of decoded frame */
      char     	       *suffix;		/* suffix of decoded frame */
      unsigned 		frame_time;
      
      if (!(decoder_state = fiasco_decoder_new (wfa_name, options)))
	 error (fiasco_get_error_message ());
   
      if (fps <= 0)			/* then use value of FIASCO file */ 
	 fps = fiasco_decoder_get_rate (decoder_state);
      frame_time = fps ? (1000 / fps) : (1000 / 25);

      if (!(width = fiasco_decoder_get_width (decoder_state)))
	  error (fiasco_get_error_message ());
      
      if (!(height = fiasco_decoder_get_height (decoder_state)))
	  error (fiasco_get_error_message ());

      if (!(frames = fiasco_decoder_get_length (decoder_state)))
	 error (fiasco_get_error_message ());
      
      get_output_template (image_name, wfa_name,
			   fiasco_decoder_is_color (decoder_state),
			   &basename, &suffix);

      filename = fiasco_calloc (strlen (basename) + strlen (suffix) + 2
			 + 10 + (int) (log10 (frames) + 1), sizeof (char));

      for (n = 0; n < frames; n++)
      {
	 clock_t fps_timer;		/* frames per second timer struct */
	 
	 prg_timer (&fps_timer, START);
	 
	 if (image_name)		/* just write frame to disk */
	 {
	    if (frames == 1)		/* just one image */
	    {
	       if (streq (image_name, "-"))
		  strcpy (filename, "-");
	       else
		  sprintf (filename, "%s.%s", basename, suffix);
	    }
	    else
	    {
	       fprintf (stderr, "Decoding frame %d to file `%s.%0*d.%s\n",
			n, basename, (int) (log10 (frames - 1) + 1),
			n, suffix);
	       sprintf (filename, "%s.%0*d.%s", basename,
			(int) (log10 (frames - 1) + 1), n, suffix);
	    }

	    if (!fiasco_decoder_write_frame (decoder_state, filename))
	       error (fiasco_get_error_message ());
	 }
      }
      free (filename);
      fiasco_decoder_delete (decoder_state);
   } while (panel);
}

static void
get_output_template (const char *image_name, const char *wfa_name,
		     bool_t color, char **basename, char **suffix)
/*
 *  Generate image filename template for output of image sequences.
 *  'wfa_name' is the filename of the WFA stream.
 *  Images are either saved with filename 'basename'.'suffix' (still images)
 *  or 'basename'.%03d.'suffix' (videos).
 *
 *  No return value.
 *
 *  Side effects:
 *	'*basename' and '*suffix' is set.
 */
{
   if (!wfa_name || streq (wfa_name, "-"))
      wfa_name = "stdin";		
   /*
    *  Generate filename template
    */
   if (!image_name || streq (image_name, "") || streq (image_name, "-"))
   {
      *basename = strdup (wfa_name);
      *suffix   = NULL;
   }
   else
   {
      *basename = strdup (image_name);
      *suffix   = strrchr (*basename, '.');
   }
    
   if (*suffix)			/* found name 'basename.suffix' */
   {
      **suffix = 0;			/* remove dot */
      (*suffix)++;
      if (**suffix == 0)
	 *suffix = strdup (color ? "ppm" : "pgm");
   }
   else				/* no suffix found, generate one */
      *suffix = strdup (color ? "ppm" : "pgm");
}

