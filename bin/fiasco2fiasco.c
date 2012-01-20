/*
 *  fiasco2fiasco.c:	Edit and concatenate FIASCO files		
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/18 19:40:33 $
 *  $Author: hafner $
 *  $Revision: 5.3 $
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

#include <stdio.h>

#include "bit-io.h"
#include "image.h"
#include "macros.h"
#include "wfalib.h"
#include "misc.h"
#include "read.h"
#include "write.h"
#include "decoder.h"
#include "params.h"
#include "error.h"

param_t params [] =
{
  {"output-name", "FILE", 'o', PSTR, {0}, "-",
   "Write automaton to `%s' (`-' means stdout)."},
  {"title", "NAME", 't', PSTR, {0}, "",
   "Set title of FIASCO stream to `%s'."},
  {"comment", "NAME", 'c', PSTR, {0}, "",
   "Set comment of FIASCO stream to `%s'."},
  {"framerate", "NUM", 'F', PINT, {0}, "-1",
   "Set display rate to `%s' frames per second."},
  {"smoothing", "NUM", 's', PINT, {0}, "-1",
   "Smooth image(s) by factor `%s' (0-100)"},
  {NULL, NULL, 0, 0, {0}, NULL, NULL }	/* no additional parameters */
};

bool_t
wfa_equal (const wfa_info_t *wi1, const wfa_info_t *wi2);

int
main (int argc, char **argv)
{
   int optind = parseargs (params, argc, argv,
			   "Edit and concatenate FIASCO video files.\n"
			   "The new FIASCO file is produced "
			   "on standard output.",
			   "Environment:\n"
			   "FIASCO_DATA   Search path for FIASCO files. "
			   "Default: ./",
			   " FILE...",
			   FIASCO_SHARE, "system.fiascorc", ".fiascorc");
 
   if (argc - optind + 1 < 2)
   {
      fprintf (stderr, "%s: usage: %s [OPTION]... FILE...\n",
	       argv [0], argv [0]);
      return 1;
   }

   try
   {
      wfa_info_t  wi_compare;		/* WFA info structure 1st file */
      unsigned    file;			/* file couter */
      bitfile_t  *output;		/* new FIASCO stream */
      unsigned    offset;		/* frame offset for current file */
      unsigned    total_frames;		/* total # frames of all files  */
      unsigned    length   = 0;		/* max. string length of filenames */
      char 	 *filename;

      /*
       *  First of all, check whether all input files have the same
       *  geometry and format (i.e. coded with the same options). Moreover,
       *  the total number of video frames is computed.
       */
      for (total_frames = 0, file = optind; file < argc; file++)
      {
	 wfa_info_t wi_read;
	 bitfile_t *input = open_wfa (argv [file], &wi_read);
	 
	 close_bitfile (input);

	 if (argc > 2 && wi_read.release < 2) /* no alignment in version 1 */
	    error ("%s:\nCan't concatenate FIASCO files with "
		   "file format release `%d'.\n"
		   "Current file format release is `%d'.",
		   argv [file], wi_read.release, FIASCO_BINFILE_RELEASE);
	    
	 if (argc - optind > 1 && wi_read.frames == 1)
	    error ("Input file `%s' is not a FIASCO video.", argv [file]);
	 
	 if (total_frames > 0 && !wfa_equal (&wi_read, &wi_compare))
	    error ("Files  `%s' and `%s' don't match.",
		   wi_compare.wfa_name, argv [file]);
	 else
	    wi_compare = wi_read;

	 length = max (length, strlen (argv [file]));
	 total_frames += wi_read.frames;
      }
      
      /*
       *  Write FIASCO header (with computed number of frames)
       */
      wi_compare.frames = total_frames;

      filename = (char *) parameter_value (params, "output-name");
      output   = open_bitfile (filename, "FIASCO_DATA", WRITE_ACCESS);

      /*
       *  Check whether to override FIASCO file options
       */
      {
	 int  value = *((int *) parameter_value (params, "smoothing"));
	 char *string;
	 
	 if (value >= 0)
	    wi_compare.smoothing = min (100, value);

	 value = *((int *) parameter_value (params, "framerate"));
	 if (value > 0)
	    wi_compare.fps = value;

	 string = (char *) parameter_value (params, "title");
	 if (strlen (string) > 0)
	 {
	    fiasco_free (wi_compare.title);
	    wi_compare.title = strdup (string);
	 }
	 
	 string = (char *) parameter_value (params, "comment");
	 if (strlen (string) > 0)
	 {
	    fiasco_free (wi_compare.comment);
	    wi_compare.comment = strdup (string);
	 }
      }

      write_header (&wi_compare, output);

      /*
       * Finally, concatenate the input files and produce the new
       * FIASCO video on standard output.
       */
      for (offset = 0, file = optind; file < argc; file++)
      {
	 unsigned  *position;
	 unsigned   n;
	 unsigned   bitpos;
	 wfa_t     *wfa   = alloc_wfa (NO);
	 video_t   *video = alloc_video (NO);
	 bitfile_t *input = open_wfa (argv [file], wfa->wfainfo);
	 unsigned   progress = 0;
	 
	 read_basis (wfa->wfainfo->basis_name, wfa);

	 if (fiasco_get_verbosity ())
	 {
	    if (length < 48)
	       fprintf (stderr, "%*s ", length, argv [file]);
	    else
	       fprintf (stderr, "%s\n", argv [file]);
	 }
	 
	 /*
	  *  Save file positions of frame `n' in array `positions [n]' 
	  */
	 position = fiasco_calloc (wfa->wfainfo->frames + 1, sizeof (unsigned));
	 for (n = 0; n < wfa->wfainfo->frames; n++)
	 {
	    image_t *frame;
	    
	    position [n] = bits_processed (input);
	    frame = get_next_frame (NO, 0, wfa->wfainfo->smoothing, NULL,
				    FORMAT_4_4_4, video, NULL, wfa, input);

	    if ((n * 30.0 / wfa->wfainfo->frames) > progress)
	    {
	       if (fiasco_get_verbosity ())
		  fprintf (stderr, "#");
	       progress = n * 30.0 / wfa->wfainfo->frames + 1;
	    }
	 }
	 if (fiasco_get_verbosity ())
	    fprintf (stderr, "\n");
	 
	 position [n] = bits_processed (input);
	 close_bitfile (input);
	 
	 /*
	  *  For each frame:
	  *  - read frame header
	  *  - write modified frame header
	  *  - copy remaining frame bits
	  */
	 input = open_bitfile (argv [file], NULL, READ_ACCESS);
	 for (bitpos = bits_processed (input); bitpos < position [0]; bitpos++)
	    get_bit (input);
	 for (n = 0; n < wfa->wfainfo->frames; n++)
	 {
	    const int rice_k = 8;	/* parameter of Rice Code */
	    unsigned  states = read_rice_code (rice_k, input);
	    unsigned  type   = read_rice_code (rice_k, input);
	    unsigned  number = read_rice_code (rice_k, input);
	    
	    INPUT_BYTE_ALIGN (input);

	    write_rice_code (states, rice_k, output);
	    write_rice_code (type, rice_k, output);
	    write_rice_code (number + offset, rice_k, output);

	    OUTPUT_BYTE_ALIGN (output);
	    
	    for (bitpos = bits_processed (input);
		 bitpos < position [n + 1]; bitpos++)
	       put_bit (output, get_bit (input));
	 }
	 close_bitfile (input);

	 offset += wfa->wfainfo->frames;
	 
	 fiasco_free (position);
	 free_video (video);
	 free_wfa (wfa);
      }

      close_bitfile (output);
   }
   catch
   {
      fprintf (stderr, "Error: ");
      fprintf (stderr, fiasco_get_error_message ());
      fprintf (stderr, "\n");

      return 1;
   }
   
   return 0;
}

bool_t
wfa_equal (const wfa_info_t *wi1, const wfa_info_t *wi2)
{
   if (streq (wi1->basis_name, wi2->basis_name)
       && wi1->smoothing == wi2->smoothing
       && wi1->max_states == wi2->max_states
       && wi1->chroma_max_states == wi2->chroma_max_states
       && wi1->p_min_level == wi2->p_min_level
       && wi1->p_max_level == wi2->p_max_level
       && wi1->fps == wi2->fps
       && wi1->half_pixel == wi2->half_pixel
       && wi1->B_as_past_ref == wi2->B_as_past_ref
       && wi1->rpf->mantissa_bits == wi2->rpf->mantissa_bits
       && wi1->rpf->range_e == wi2->rpf->range_e
       && wi1->dc_rpf->mantissa_bits == wi2->dc_rpf->mantissa_bits
       && wi1->dc_rpf->range_e == wi2->dc_rpf->range_e
       && wi1->d_rpf->mantissa_bits == wi2->d_rpf->mantissa_bits
       && wi1->d_rpf->range_e == wi2->d_rpf->range_e
       && wi1->d_dc_rpf->mantissa_bits == wi2->d_dc_rpf->mantissa_bits
       && wi1->d_dc_rpf->range_e == wi2->d_dc_rpf->range_e
       && wi1->width == wi2->width
       && wi1->height == wi2->height
       && wi1->color == wi2->color
       )
      return 1;
   else
      return 0;
}
