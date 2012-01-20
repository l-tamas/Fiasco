/*
 *  dfiasco.c:		Decoder public interface
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */
 
/*
 *  $Date: 2000/10/28 17:39:30 $
 *  $Author: hafner $
 *  $Revision: 5.7 $
 *  $State: Exp $
 */

#include "config.h"

#include "types.h"
#include "macros.h"
#include "error.h"

#include "dfiasco.h"
#include "wfa.h"
#include "read.h"
#include "misc.h"
#include "bit-io.h"
#include "decoder.h"
#include "options.h"
#include "wfalib.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static dfiasco_t *
cast_dfiasco (fiasco_decoder_t *dfiasco);
static void
free_dfiasco (dfiasco_t *dfiasco);
static dfiasco_t *
alloc_dfiasco (wfa_t *wfa, video_t *video, bitfile_t *input,
	       int enlarge_factor, int smoothing, format_e image_format);

/*****************************************************************************

				public code
  
*****************************************************************************/

fiasco_decoder_t *
fiasco_decoder_new (const char *filename, const fiasco_d_options_t *options)
{
   try
   {
      bitfile_t        	 *input;	/* pointer to WFA FIASCO stream */
      wfa_t            	 *wfa;		/* wfa structure */
      video_t          	 *video;	/* information about decoder state */
      const d_options_t  *dop;		/* decoder additional options */
      dfiasco_t	       	 *dfiasco;	/* decoder internal state */
      fiasco_decoder_t 	 *decoder;	/* public interface to decoder */
      fiasco_d_options_t *default_options = NULL;

      if (options)
      {
	 dop = cast_d_options ((fiasco_d_options_t *) options);
	 if (!dop)
	    return NULL;
      }
      else
      {
	 default_options = fiasco_d_options_new ();
	 dop 		 = cast_d_options (default_options);
      }
      
      wfa   = alloc_wfa (NO);
      video = alloc_video (NO);
      input = open_wfa (filename, wfa->wfainfo);
      read_basis (wfa->wfainfo->basis_name, wfa);

      decoder 	       	   = fiasco_calloc (1, sizeof (fiasco_decoder_t));
      decoder->delete  	   = fiasco_decoder_delete;
      decoder->write_frame = fiasco_decoder_write_frame;
      decoder->get_frame   = fiasco_decoder_get_frame;
      decoder->get_length  = fiasco_decoder_get_length;
      decoder->get_rate    = fiasco_decoder_get_rate;
      decoder->get_width   = fiasco_decoder_get_width;
      decoder->get_height  = fiasco_decoder_get_height;
      decoder->get_title   = fiasco_decoder_get_title;
      decoder->get_comment = fiasco_decoder_get_comment;
      decoder->is_color    = fiasco_decoder_is_color;

      decoder->private = dfiasco
		       = alloc_dfiasco (wfa, video, input,
					dop->magnification,
					dop->smoothing,
					dop->image_format);
   
      if (default_options)
	 fiasco_d_options_delete (default_options);
      if (dfiasco->enlarge_factor >= 0)
      {
	 int 	       n;
	 unsigned long pixels = wfa->wfainfo->width * wfa->wfainfo->height;

	 for (n = 1; n <= (int) dfiasco->enlarge_factor; n++)
	 {
	    if (pixels << (n << 1) > 2048 * 2048)
	    {
	       set_error (_("Magnifaction factor `%d' is too large. "
			    "Maximium value is %d."),
			  dfiasco->enlarge_factor, max (0, n - 1));
	       fiasco_decoder_delete (decoder);
	       return NULL;
	    }
	 }
      }
      else
      {
	 int n;

	 for (n = 0; n <= (int) - dfiasco->enlarge_factor; n++)
	 {
	    if (wfa->wfainfo->width >> n < 32
		|| wfa->wfainfo->height >> n < 32)
	    {
	       set_error (_("Magnifaction factor `%d' is too small. "
			    "Minimum value is %d."),
			  dfiasco->enlarge_factor, - max (0, n - 1));
	       fiasco_decoder_delete (decoder);
	       return NULL;
	    }
	 }
      }
      return (fiasco_decoder_t *) decoder;
   }
   catch
   {
      return NULL;
   }
}

int
fiasco_decoder_write_frame (fiasco_decoder_t *decoder,
			    const char *filename)
{
   dfiasco_t *dfiasco = cast_dfiasco (decoder);
   
   if (!dfiasco)
      return 0;
   else
   {
      try
      {
	 image_t *frame = get_next_frame (NO, dfiasco->enlarge_factor,
					  dfiasco->smoothing, NULL,
					  FORMAT_4_4_4, dfiasco->video, NULL,
					  dfiasco->wfa, dfiasco->input);
	 write_image (filename, frame);
      }
      catch
      {
	 return 0;
      }
      return 1;
   }
}

fiasco_image_t *
fiasco_decoder_get_frame (fiasco_decoder_t *decoder)
{
   dfiasco_t *dfiasco = cast_dfiasco (decoder);
   
   if (!dfiasco)
      return NULL;
   else
   {
      try
      {
	 fiasco_image_t *image = fiasco_calloc (1, sizeof (fiasco_image_t));
	 image_t 	*frame = get_next_frame (NO, dfiasco->enlarge_factor,
						 dfiasco->smoothing, NULL,
						 dfiasco->image_format,
						 dfiasco->video, NULL,
						 dfiasco->wfa, dfiasco->input);

	 frame->reference_count++;	/* for motion compensation */
	 image->private    = frame;
	 image->delete     = fiasco_image_delete;
	 image->get_width  = fiasco_image_get_width;
	 image->get_height = fiasco_image_get_height;
	 image->is_color   = fiasco_image_is_color;
	 
	 return image;
      }
      catch
      {
	 return NULL;
      }
   }
}

unsigned
fiasco_decoder_get_length (fiasco_decoder_t *decoder)
{
   dfiasco_t *dfiasco = cast_dfiasco (decoder);
   
   if (!dfiasco)
      return 0;
   else
      return dfiasco->wfa->wfainfo->frames;
}

unsigned
fiasco_decoder_get_rate (fiasco_decoder_t *decoder)
{
   dfiasco_t *dfiasco = cast_dfiasco (decoder);
   
   if (!dfiasco)
      return 0;
   else
      return dfiasco->wfa->wfainfo->fps;
}

unsigned
fiasco_decoder_get_width (fiasco_decoder_t *decoder)
{
   dfiasco_t *dfiasco = cast_dfiasco (decoder);

   if (!dfiasco)
      return 0;
   else
   {
      unsigned width;
      
      if (dfiasco->enlarge_factor >= 0)
	 width = dfiasco->wfa->wfainfo->width << dfiasco->enlarge_factor;
      else
	 width = dfiasco->wfa->wfainfo->width >> - dfiasco->enlarge_factor;
      
      return width & 1 ? width + 1 : width;
   }
}

unsigned
fiasco_decoder_get_height (fiasco_decoder_t *decoder)
{
   dfiasco_t *dfiasco = cast_dfiasco (decoder);

   if (!dfiasco)
      return 0;
   else
   {
      unsigned height;
      
      if (dfiasco->enlarge_factor >= 0)
	 height = dfiasco->wfa->wfainfo->height << dfiasco->enlarge_factor;
      else
	 height = dfiasco->wfa->wfainfo->height >> - dfiasco->enlarge_factor;

      return height & 1 ? height + 1 : height;
   }
}

const char *
fiasco_decoder_get_title (fiasco_decoder_t *decoder)
{
   dfiasco_t *dfiasco = cast_dfiasco (decoder);

   if (!dfiasco)
      return NULL;
   else
      return dfiasco->wfa->wfainfo->title;
}

const char *
fiasco_decoder_get_comment (fiasco_decoder_t *decoder)
{
   dfiasco_t *dfiasco = cast_dfiasco (decoder);

   if (!dfiasco)
      return NULL;
   else
      return dfiasco->wfa->wfainfo->comment;
}

int
fiasco_decoder_is_color (fiasco_decoder_t *decoder)
{
   dfiasco_t *dfiasco = cast_dfiasco (decoder);

   if (!dfiasco)
      return 0;
   else
      return dfiasco->wfa->wfainfo->color;
}

int
fiasco_decoder_delete (fiasco_decoder_t *decoder)
{
   dfiasco_t *dfiasco = cast_dfiasco (decoder);
   
   if (!dfiasco)
      return 1;
   
   try
   {
      free_wfa (dfiasco->wfa);
      free_video (dfiasco->video);
      close_bitfile (dfiasco->input);
      strcpy (dfiasco->id, " ");
      fiasco_free (decoder);
   }
   catch
   {
      return 0;
   }

   return 1;
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static dfiasco_t *
alloc_dfiasco (wfa_t *wfa, video_t *video, bitfile_t *input,
	       int enlarge_factor, int smoothing, format_e image_format)
/*
 *  FIASCO decoder constructor:
 *  Initialize decoder structure.
 *
 *  Return value:
 *	pointer to the new decoder structure
 */
{
   dfiasco_t *dfiasco = fiasco_calloc (1, sizeof (dfiasco_t));

   strcpy (dfiasco->id, "DFIASCO");
   
   dfiasco->wfa 	   = wfa;
   dfiasco->video 	   = video;
   dfiasco->input 	   = input;
   dfiasco->enlarge_factor = enlarge_factor;
   dfiasco->smoothing  	   = smoothing;
   dfiasco->image_format   = image_format;
   
   return dfiasco;
}

static void
free_dfiasco (dfiasco_t *dfiasco)
/*
 *  FIASCO decoder destructor:
 *  Free memory of given 'decoder' struct.
 *
 *  No return value.
 *
 *  Side effects:
 *	'video' struct is discarded.
 */
{
   fiasco_free (dfiasco);
}

static dfiasco_t *
cast_dfiasco (fiasco_decoder_t *dfiasco)
/*
 *  Cast pointer `dfiasco' to type dfiasco_t.
 *  Check whether `dfiasco' is a valid object of type dfiasco_t.
 *
 *  Return value:
 *	pointer to dfiasco_t struct on success
 *      NULL otherwise
 */
{
   dfiasco_t *this = (dfiasco_t *) dfiasco->private;
   if (this)
   {
      if (!streq (this->id, "DFIASCO"))
      {
	 set_error (_("Parameter `dfiasco' doesn't match required type."));
	 return NULL;
      }
   }
   else
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "dfiasco");
   }

   return this;
}
