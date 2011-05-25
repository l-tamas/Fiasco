/*
 *  dither.c:		Various dithering routines 	
 *
 *  Adapted by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 * Copyright (c) 1995 Erik Corry
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL ERIK CORRY BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
 * SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF
 * THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF ERIK CORRY HAS BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ERIK CORRY SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND ERIK CORRY HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,
 * UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/*
 *  $Date: 2000/11/27 20:22:51 $
 *  $Author: hafner $
 *  $Revision: 5.3 $
 *  $State: Exp $
 */

#include "config.h"

#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */
#if STDC_HEADERS
#	include <stdlib.h>
#endif /* not STDC_HEADERS */

#include "types.h"
#include "macros.h"
#include "error.h"

#include "fiasco.h"
#include "image.h"
#include "misc.h"
#include "dither.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static int 
display_16_bit (const struct fiasco_renderer *this, unsigned char *ximage,
		const fiasco_image_t *fiasco_image);
static int 
display_24_bit_bgr (const struct fiasco_renderer *this, unsigned char *ximage,
		    const fiasco_image_t *fiasco_image);
static int 
display_24_bit_rgb (const struct fiasco_renderer *this, unsigned char *ximage,
		    const fiasco_image_t *fiasco_image);
static int 
display_32_bit (const struct fiasco_renderer *this, unsigned char *ximage,
		const fiasco_image_t *fiasco_image);
static int
free_bits_at_bottom (unsigned long a);
static int
free_bits_at_top (unsigned long a);
static int
number_of_bits_set (unsigned long a);

/*****************************************************************************

				public code
  
*****************************************************************************/

fiasco_renderer_t *
fiasco_renderer_new (unsigned long red_mask, unsigned long green_mask,
		     unsigned long blue_mask, unsigned bpp,
		     int double_resolution)
/*
 *  FIASCO renderer constructor.
 *  Allocate memory for the FIASCO renderer structure and
 *  initialize values.
 *  `red_mask', `green_mask', and `blue_mask' are the corresponding masks
 *  of the X11R6 XImage structure. 
 *  `bpp' gives the depth of the image in bits per pixel (16, 24, or 32).
 *  If `double_resolution' is not 0 the the image width and height is doubled.
 *  (fast pixel doubling, no interpolation!)
 *
 *  Return value:
 *	pointer to the new structure or NULL on error
 */
{
   if (bpp != 16 && bpp != 24 && bpp !=32)
   {
      set_error (_("Rendering depth of XImage must be 16, 24, or 32 bpp."));
      return NULL;
   }
   else
   {
      fiasco_renderer_t  *render    = calloc (1, sizeof (fiasco_renderer_t));
      renderer_private_t *private   = calloc (1, sizeof (renderer_private_t));
      bool_t 	       	  twopixels = (bpp == 16 && double_resolution);
      int 		  crval, cbval, i; /* counter */

      if (!render || !private)
      {
	 set_error (_("Out of memory."));
	 return NULL;
      }
      switch (bpp)
      {
	 case 16:
	    render->render = display_16_bit;
	    break;
	 case 24:
	    if (red_mask > green_mask)
	       render->render = display_24_bit_rgb;
	    else
	       render->render = display_24_bit_bgr;
	    break;
	 case 32:
	    render->render = display_32_bit;
	    break;
	 default:
	    break;			/* does not happen */
      }
      render->private = private;
      render->delete  = fiasco_renderer_delete;

      private->double_resolution = double_resolution;
      private->Cr_r_tab = calloc (256 + 2 * 1024, sizeof (int));
      private->Cr_g_tab = calloc (256 + 2 * 1024, sizeof (int));
      private->Cb_g_tab = calloc (256 + 2 * 1024, sizeof (int));
      private->Cb_b_tab = calloc (256 + 2 * 1024, sizeof (int));

      if (!private->Cr_r_tab || !private->Cr_g_tab
	  || !private->Cb_b_tab || !private->Cb_g_tab)
      {
	 set_error (_("Out of memory."));
	 return NULL;
      }
      
      for (i = 1024; i < 1024 + 256; i++)
      {
	 cbval = crval  = i - 128 - 1024;

	 private->Cr_r_tab [i] =  1.4022 * crval + 0.5;
	 private->Cr_g_tab [i] = -0.7145 * crval + 0.5;
	 private->Cb_g_tab [i] = -0.3456 * cbval + 0.5; 
	 private->Cb_b_tab [i] =  1.7710 * cbval + 0.5;
      }
      for (i = 0; i < 1024; i++)
      {
	 private->Cr_r_tab [i] = private->Cr_r_tab [1024];
	 private->Cr_g_tab [i] = private->Cr_g_tab [1024];
	 private->Cb_g_tab [i] = private->Cb_g_tab [1024]; 
	 private->Cb_b_tab [i] = private->Cb_b_tab [1024];
      }
      for (i = 1024 + 256; i < 2048 + 256; i++)
      {
	 private->Cr_r_tab [i] = private->Cr_r_tab [1024 + 255];
	 private->Cr_g_tab [i] = private->Cr_g_tab [1024 + 255];
	 private->Cb_g_tab [i] = private->Cb_g_tab [1024 + 255]; 
	 private->Cb_b_tab [i] = private->Cb_b_tab [1024 + 255];
      }

      private->Cr_r_tab += 1024 + 128;
      private->Cr_g_tab += 1024 + 128;
      private->Cb_g_tab += 1024 + 128;
      private->Cb_b_tab += 1024 + 128;
   
      /* 
       *  Set up entries 0-255 in rgb-to-pixel value tables.
       */
      private->r_table = calloc (256 + 2 * 1024, sizeof (unsigned int));
      private->g_table = calloc (256 + 2 * 1024, sizeof (unsigned int));
      private->b_table = calloc (256 + 2 * 1024, sizeof (unsigned int));
      private->y_table = calloc (256 + 2 * 1024, sizeof (unsigned int));

      if (!private->r_table || !private->g_table
	  || !private->b_table || !private->y_table)
      {
	 set_error (_("Out of memory."));
	 return NULL;
      }
      
      for (i = 0; i < 256; i++)
      {
	 private->r_table [i + 1024]
	    = i >> (8 - number_of_bits_set(red_mask));
	 private->r_table [i + 1024]
	    <<= free_bits_at_bottom (red_mask);
	 private->g_table [i + 1024]
	    = i >> (8 - number_of_bits_set (green_mask));
	 private->g_table [i + 1024]
	    <<= free_bits_at_bottom (green_mask);
	 private->b_table [i + 1024]
	    <<= free_bits_at_bottom (blue_mask);
	 private->b_table [i + 1024]
	    = i >> (8 - number_of_bits_set (blue_mask));
	 if (twopixels)
	 {
	    private->r_table [i + 1024] = ((private->r_table [i + 1024] << 16)
					   | private->r_table [i + 1024]);
	    private->g_table [i + 1024] = ((private->g_table [i + 1024] << 16)
					   | private->g_table [i + 1024]);
	    private->b_table [i + 1024] = ((private->b_table [i + 1024] << 16)
					   | private->b_table [i + 1024]);
	 }
	 private->y_table [i + 1024] = (private->r_table [i + 1024]
					| private->g_table [i + 1024]
					| private->b_table [i + 1024]);
      }

      /*
       * Spread out the values we have to the rest of the array so that
       * we do not need to check for overflow.
       */
      for (i = 0; i < 1024; i++)
      {
	 private->r_table [i]              = private->r_table [1024];
	 private->r_table [i + 1024 + 256] = private->r_table [1024 + 255];
	 private->g_table [i]              = private->g_table [1024];
	 private->g_table [i + 1024 + 256] = private->g_table [1024 + 255];
	 private->b_table [i]              = private->b_table [1024];
	 private->b_table [i + 1024 + 256] = private->b_table [1024 + 255];
	 private->y_table [i]              = private->y_table [1024];
	 private->y_table [i + 1024 + 256] = private->y_table [1024 + 255];
      }

      private->r_table += 1024;
      private->g_table += 1024;
      private->b_table += 1024;
      private->y_table += 1024 + 128;

      return render;
   }
   
}

void
fiasco_renderer_delete (fiasco_renderer_t *renderer)
/*
 *  FIASCO renderer destructor:
 *  Free memory of 'renderer' structure.
 *
 *  No return value.
 *
 *  Side effects:
 *	structure 'renderer' is discarded.
 */
{
   if (!renderer)
      return;
   else
   {
      renderer_private_t *private = (renderer_private_t *) renderer->private;

      Free (private->Cr_g_tab - (1024 + 128));
      Free (private->Cr_r_tab - (1024 + 128));
      Free (private->Cb_g_tab - (1024 + 128));
      Free (private->Cb_b_tab - (1024 + 128));
      Free (private->r_table - 1024);
      Free (private->g_table - 1024);
      Free (private->b_table - 1024);
      Free (private->y_table - (1024 + 128));

      Free (private);
      Free (renderer);
   }
}

int
fiasco_renderer_render (const fiasco_renderer_t *renderer,
			unsigned char *ximage,
			const fiasco_image_t *fiasco_image)
{
   if (!renderer)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "renderer");
      return 0;
   }
   else
      return renderer->render (renderer, ximage, fiasco_image);
}

/*****************************************************************************

				private code
  
*****************************************************************************/

/*
 *  Erik Corry's multi-byte dither routines.
 *
 *  The basic idea is that the Init generates all the necessary
 *  tables.  The tables incorporate the information about the layout
 *  of pixels in the XImage, so that it should be able to cope with
 *  16-bit 24-bit (non-packed) and 32-bit (10-11 bits per
 *  color!) screens.  At present it cannot cope with 24-bit packed
 *  mode, since this involves getting down to byte level again. It is
 *  assumed that the bits for each color are contiguous in the
 *  longword.
 * 
 *  Writing to memory is done in shorts or ints. (Unfortunately, short
 *  is not very fast on Alpha, so there is room for improvement
 *  here). There is no dither time check for overflow - instead the
 *  tables have slack at each end. This is likely to be faster than an
 *  'if' test as many modern architectures are really bad at
 *  ifs. Potentially, each '&&' causes a pipeline flush!
 *
 *  There is no shifting and fixed point arithmetic, as I really doubt
 *  you can see the difference, and it costs. This may be just my
 *  bias, since I heard that Intel is really bad at shifting.
 */

static int
number_of_bits_set (unsigned long a)
/*
 *  How many 1 bits are there in the longword.
 *  Low performance, do not call often.
 */
{
   if (!a)
      return 0;
   if (a & 1)
      return 1 + number_of_bits_set (a >> 1);
   else
      return (number_of_bits_set (a >> 1));
}

static int
free_bits_at_top (unsigned long a)
/*
 *  How many 0 bits are there at most significant end of longword.
 *  Low performance, do not call often.
 */
{
   if(!a)				/* assume char is 8 bits */
      return sizeof (unsigned long) * 8;
   else if (((long) a) < 0l)		/* assume twos complement */
      return 0;
   else
      return 1 + free_bits_at_top ( a << 1);
}

static int
free_bits_at_bottom (unsigned long a)
/*
 *  How many 0 bits are there at least significant end of longword.
 *  Low performance, do not call often.
 */
{
   /* assume char is 8 bits */
   if (!a)
      return sizeof (unsigned long) * 8;
   else if(((long) a) & 1l)
      return 0;
   else
      return 1 + free_bits_at_bottom ( a >> 1);
}

static int 
display_16_bit (const struct fiasco_renderer *this, unsigned char *ximage,
		const fiasco_image_t *fiasco_image)
/*
 *  Convert 'image' to 16 bit color bitmap.
 *  If 'double_resolution' is true then double image size in both directions.
 *
 *  No return value.
 *
 *  Side effects:
 *	'out[]'	is filled with dithered image
 */
{
   const image_t      *image;
   renderer_private_t *private;
   byte_t	      *out;
   
   if (!this)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "this");
      return 0;
   }
   if (!ximage)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "ximage");
      return 0;
   }
   if (!fiasco_image)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "fiasco_image");
      return 0;
   }

   out 	   = (byte_t *) ximage;
   image   = cast_image ((fiasco_image_t *) fiasco_image);
   if (!image)
      return 0;
   private = (renderer_private_t *) this->private;
   
   if (image->color)
   {
      word_t 	   *cbptr, *crptr;	/* pointer to chroma bands */
      word_t 	   *yptr;		/* pointers to lumincance band */
      int     	    yval, crval, cbval;	/* pixel value in YCbCr color space */
      int     	    R, G, B;		/* pixel value in RGB color space */
      int     	    n;			/* pixel counter */
      int     	    x, y;		/* pixel coordinates */
      int 	   *Cr_r_tab, *Cr_g_tab, *Cb_g_tab, *Cb_b_tab;
      unsigned int *r_table, *g_table, *b_table;

      Cr_g_tab = private->Cr_g_tab;
      Cr_r_tab = private->Cr_r_tab;
      Cb_b_tab = private->Cb_b_tab;
      Cb_g_tab = private->Cb_g_tab;
      r_table  = private->r_table;
      g_table  = private->g_table;
      b_table  = private->b_table;
      yptr     = image->pixels [Y];
      cbptr    = image->pixels [Cb];
      crptr    = image->pixels [Cr];

      if (image->format == FORMAT_4_2_0)
      {
	 u_word_t *dst, *dst2;		/* pointers to dithered pixels */
	 word_t	  *yptr2;		/* pointers to lumincance band */

	 if (private->double_resolution)
	 {
	    yptr2 = yptr + image->width;
	    dst   = (u_word_t *) out;
	    dst2  = dst + 4 * image->width;
	    for (y = image->height / 2; y; y--)
	    {
	       for (x = image->width / 2; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  crval = *crptr++ >> 4;
		  cbval = *cbptr++ >> 4;
		  yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  crval = *crptr++ / 16;
		  cbval = *cbptr++ / 16;
		  yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];
	       
#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];

#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];

#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];
	       }
	       memcpy (dst, dst - 2 * image->width,
		       2 * image->width * sizeof (u_word_t));
	       memcpy (dst2, dst2 - 2 * image->width,
		       2 * image->width * sizeof (u_word_t));
	       yptr  += image->width;
	       yptr2 += image->width;
	       dst   += 3 * image->width * 2;
	       dst2  += 3 * image->width * 2;
	    }
	 }
	 else
	 {
	    yptr2 = yptr + image->width;
	    dst  = (u_word_t *) out;
	    dst2 = dst + image->width;
	    
	    for (y = image->height / 2; y; y--)
	    {
	       for (x = image->width / 2; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  crval = *crptr++ >> 4;
		  cbval = *cbptr++ >> 4;
		  yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  crval = *crptr++ / 16;
		  cbval = *cbptr++ / 16;
		  yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];
	       
#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];

#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];

#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];
	       }
	       yptr  += image->width;
	       yptr2 += image->width;
	       dst   += image->width;
	       dst2  += image->width;
	    }
	 }
      }
      else				/* 4:4:4 format */
      {
	 if (private->double_resolution)
	 {
	    unsigned int *dst;		/* pointer to dithered pixels */
	    
	    dst  = (unsigned int *) out;
	    
	    for (y = image->height; y; y--)
	    {
	       for (x = image->width; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  crval = *crptr++ >> 4;
		  cbval = *cbptr++ >> 4;
		  yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  crval = *crptr++ / 16;
		  cbval = *cbptr++ / 16;
		  yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  
		  *dst++ = r_table [R] | g_table [G] | b_table [B];
	       }
	       memcpy (dst, dst - image->width,
		       image->width * sizeof (unsigned int));
	       dst += image->width;
	    }
	 }
	 else
	 {
	    u_word_t *dst;		/* pointer to dithered pixels */

	    dst  = (u_word_t *) out;
	    
	    for (n = image->width * image->height; n; n--)
	    {
#ifdef HAVE_SIGNED_SHIFT
	       crval = *crptr++ >> 4;
	       cbval = *cbptr++ >> 4;
	       yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
	       crval = *crptr++ / 16;
	       cbval = *cbptr++ / 16;
	       yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
	       R = yval + Cr_r_tab [crval];
	       G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
	       B = yval + Cb_b_tab [cbval];

	       *dst++ = r_table [R] | g_table [G] | b_table [B];
	    }
	 }
      }
   }
   else
   {
      unsigned int *dst;		/* pointer to dithered pixels */
      word_t	   *src;		/* current pixel of frame */
      unsigned int *y_table;

      y_table = private->y_table;
      dst     = (unsigned int *) out;
      src     = image->pixels [GRAY];
      
      if (private->double_resolution)
      {
	 int x, y;			/* pixel coordinates */
  	    
	 for (y = image->height; y; y--)
	 {
	    for (x = image->width; x; x--)
	    {
	       int value;
	       
#ifdef HAVE_SIGNED_SHIFT
	       value = y_table [*src++ >> 4];
#else /* not HAVE_SIGNED_SHIFT */
	       value = y_table [*src++ / 16];
#endif /* not HAVE_SIGNED_SHIFT */
	       *dst++ = (value << 16) | value;
	    }
	    
	    memcpy (dst, dst - image->width,
		    image->width * sizeof (unsigned int));
	    dst += image->width;
	 }
      }
      else
      {
	 int n;				/* pixel counter */
	 
	 for (n = image->width * image->height / 2; n; n--, src += 2)
#ifdef HAVE_SIGNED_SHIFT
#	ifndef WORDS_BIGENDIAN
	    *dst++ = (y_table [src [1] >> 4] << 16) | y_table [src [0] >> 4];
#	else /* not WORDS_BIGENDIAN  */
	    *dst++ = (y_table [src [0] >> 4] << 16) | y_table [src [1] >> 4];
#	endif
#else /* not HAVE_SIGNED_SHIFT */
#	ifndef WORDS_BIGENDIAN
	    *dst++ = (y_table [src [1] / 16] << 16) | y_table [src [0] / 16];
#	else /* not WORDS_BIGENDIAN  */
	    *dst++ = (y_table [src [0] / 16] << 16) | y_table [src [1] / 16];
#	endif
#endif /* not HAVE_SIGNED_SHIFT */
      }
   }

   return 1;
}

static int 
display_24_bit_bgr (const struct fiasco_renderer *this, unsigned char *ximage,
		    const fiasco_image_t *fiasco_image)
/*
 *  Convert 'image' to 16 bit color bitmap.
 *  If 'double_resolution' is true then double image size in both directions.
 *
 *  No return value.
 *
 *  Side effects:
 *	'out[]'	is filled with dithered image
 */
{
   unsigned 	      *gray_clip = init_clipping ();
   const image_t      *image;
   renderer_private_t *private;
   byte_t	      *out;

   if (!gray_clip)
      return 0;
   if (!this)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "this");
      return 0;
   }
   if (!ximage)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "ximage");
      return 0;
   }
   if (!fiasco_image)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "fiasco_image");
      return 0;
   }

   out 	   = (byte_t *) ximage;
   image   = cast_image ((fiasco_image_t *) fiasco_image);
   if (!image)
      return 0;
   private = (renderer_private_t *) this->private;
   
   if (image->color)
   {
      word_t 	   *cbptr, *crptr;	/* pointer to chroma bands */
      word_t 	   *yptr;		/* pointers to lumincance band */
      int 	   *Cr_r_tab, *Cr_g_tab, *Cb_g_tab, *Cb_b_tab;
      unsigned int *r_table, *g_table, *b_table;

      Cr_g_tab = private->Cr_g_tab;
      Cr_r_tab = private->Cr_r_tab;
      Cb_b_tab = private->Cb_b_tab;
      Cb_g_tab = private->Cb_g_tab;
      r_table  = private->r_table;
      g_table  = private->g_table;
      b_table  = private->b_table;
      yptr     = image->pixels [Y];
      cbptr    = image->pixels [Cb];
      crptr    = image->pixels [Cr];

      if (image->format == FORMAT_4_2_0)
      {
	 if (private->double_resolution)
	 {
	    int		  yval1;	/* lumincance pixel */
	    int 	  crval1, cbval1; /* chroma pixels */
	    int		  yval2;	/* pixel in YCbCr color space */
	    unsigned int  R1, G1, B1;	/* pixel in RGB color space */
	    unsigned int  R2, G2, B2;	/* pixel in RGB color space */
	    int		  x, y;		/* pixel counter */
	    unsigned int *dst;		/* pointer to dithered pixels */
	    unsigned int *dst2;		/* pointers to dithered pixels */
	    word_t	 *yptr2;	/* pointers to lumincance band */
	    
	    dst   = (unsigned int *) out;
	    dst2  = dst + (image->width >> 1) * 3 * 2;
	    yptr2 = yptr + image->width;
	    
	    for (y = image->height >> 1; y; y--)
	    {
	       for (x = image->width >> 1; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr++ >> 4) + 128;
		  yval2  = (*yptr++ >> 4) + 128;
		  crval1 = *crptr++ >> 4;
		  cbval1 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr++  / 16 + 128;
		  yval2  = *yptr++  / 16 + 128;
		  crval1 = *crptr++ / 16;
		  cbval1 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst++ = B1 | (G1 << 8) | (R1 << 16) | (B1 << 24);
		  *dst++ = G1 | (R1 << 8) | (B2 << 16) | (G2 << 24);
		  *dst++ = R2 | (B2 << 8) | (G2 << 16) | (R2 << 24);

#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr2++ >> 4) + 128;
		  yval2  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr2++  / 16 + 128;
		  yval2  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst2++ = B1 | (G1 << 8) | (R1 << 16) | (B1 << 24);
		  *dst2++ = G1 | (R1 << 8) | (B2 << 16) | (G2 << 24);
		  *dst2++ = R2 | (B2 << 8) | (G2 << 16) | (R2 << 24);
	       }
	       memcpy (dst, dst - (image->width >> 1) * 3,
		       (image->width >> 1) * 3 * sizeof (unsigned int));
	       memcpy (dst2, dst2 - (image->width >> 1) * 3, 
		       (image->width >> 1) * 3 * sizeof (unsigned int));
	       dst   += (image->width >> 1) * 3 * 3;
	       dst2  += (image->width >> 1) * 3 * 3;
	       yptr  += image->width;
	       yptr2 += image->width;
	    }
	 }
	 else
	 {
	    int		  yval1;	/* lumincance pixel */
	    int 	  crval1, cbval1; /* chroma pixels */
	    int		  yval2;	/* pixel in YCbCr color space */
	    unsigned int  R1, G1, B1;	/* pixel in RGB color space */
	    unsigned int  R2, G2, B2;	/* pixel in RGB color space */
	    int		  x, y;		/* pixel counter */
	    unsigned int *dst;		/* pointer to dithered pixels */
	    unsigned int *dst2;		/* pointers to dithered pixels */
	    word_t	 *yptr2;	/* pointers to lumincance band */
	    
	    dst   = (unsigned int *) out;
	    dst2  = dst + (image->width >> 2) * 3;
	    yptr2 = yptr + image->width;
	    
	    for (y = image->height >> 1; y; y--)
	    {
	       for (x = image->width >> 2; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr++ >> 4) + 128;
		  yval2  = (*yptr++ >> 4) + 128;
		  crval1 = *crptr++ >> 4;
		  cbval1 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr++  / 16 + 128;
		  yval2  = *yptr++  / 16 + 128;
		  crval1 = *crptr++ / 16;
		  cbval1 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst++ = B1 | (G1 << 8) | (R1 << 16) | (B2 << 24);
		  *dst   = G2 | (R2 << 8);

#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr2++ >> 4) + 128;
		  yval2  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr2++  / 16 + 128;
		  yval2  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst2++ = B1 | (G1 << 8) | (R1 << 16) | (B2 << 24);
		  *dst2   = G2 | (R2 << 8);
	       
#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr++ >> 4) + 128;
		  yval2  = (*yptr++ >> 4) + 128;
		  crval1 = *crptr++ >> 4;
		  cbval1 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr++  / 16 + 128;
		  yval2  = *yptr++  / 16 + 128;
		  crval1 = *crptr++ / 16;
		  crval2 = *crptr++ / 16;
		  cbval1 = *cbptr++ / 16;
		  cbval2 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst++ |= (B1 << 16) | (G1 << 24);
		  *dst++ = R1 | (B2 << 8) | (G2 << 16) | (R2 << 24);

#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr2++ >> 4) + 128;
		  yval2  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr2++  / 16 + 128;
		  yval2  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst2++ |= (B1 << 16) | (G1 << 24);
		  *dst2++ = R1 | (B2 << 8) | (G2 << 16) | (R2 << 24);
	       }
	       dst   += (image->width >> 2) * 3;
	       dst2  += (image->width >> 2) * 3;
	       yptr  += image->width;
	       yptr2 += image->width;
	    }
	 }
      }
      else				/* 4:4:4 format */
      {
	 if (private->double_resolution)
	 {
	    unsigned int R1, G1, B1;		/* pixel1 in RGB color space */
	    unsigned int R2, G2, B2;		/* pixel2 in RGB color space */
	    int		 yval1, crval1, cbval1;	/* pixel1 in YCbCr space */
	    int		 yval2, crval2, cbval2;	/* pixel2 in YCbCr space */
	    int		 x, y;			/* pixel counter */
	    unsigned int *dst;		        /* dithered pixel pointer */
	    
	    dst = (unsigned int *) out;
	    
	    for (y = image->height; y; y--)
	    {
	       for (x = image->width >> 1; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr++ >> 4) + 128;
		  yval2  = (*yptr++ >> 4) + 128;
		  crval1 = *crptr++ >> 4;
		  crval2 = *crptr++ >> 4;
		  cbval1 = *cbptr++ >> 4;
		  cbval2 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr++  / 16 + 128;
		  yval2  = *yptr++  / 16 + 128;
		  crval1 = *crptr++ / 16;
		  crval2 = *crptr++ / 16;
		  cbval1 = *cbptr++ / 16;
		  cbval2 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */
		  
		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				 + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval2]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval2]
				 + Cb_g_tab [cbval2]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval2]];

		  *dst++ = B1 | (G1 << 8) | (R1 << 16) | (B1 << 24);
		  *dst++ = G1 | (R1 << 8) | (B2 << 16) | (G2 << 24);
		  *dst++ = R2 | (B2 << 8) | (G2 << 16) | (R2 << 24);
	       }
	       memcpy (dst, dst - 3 * (image->width >> 1),
		       3 * (image->width >> 1) * sizeof (unsigned int));
	       dst += 3 * (image->width >> 1);
	    }
	 }
	 else
	 {
	    unsigned int R1, G1, B1;		/* pixel in RGB color space */
	    unsigned int R2, G2, B2;		/* pixel in RGB color space */
	    int		 yval1, crval1, cbval1;	/* pixel1 in YCbCr space */
	    int		 yval2, crval2, cbval2;	/* pixel2 in YCbCr space */
	    int		 n;			/* pixel counter */
	    unsigned int *dst;		        /* dithered pixel pointer */
	    
	    dst = (unsigned int *) out;
	    
	    for (n = (image->width * image->height) >> 2; n; n--)
	    {
#ifdef HAVE_SIGNED_SHIFT
	       yval1  = (*yptr++ >> 4) + 128;
	       yval2  = (*yptr++ >> 4) + 128;
	       crval1 = *crptr++ >> 4;
	       crval2 = *crptr++ >> 4;
	       cbval1 = *cbptr++ >> 4;
	       cbval2 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
	       yval1  = *yptr++  / 16 + 128;
	       yval2  = *yptr++  / 16 + 128;
	       crval1 = *crptr++ / 16;
	       crval2 = *crptr++ / 16;
	       cbval1 = *cbptr++ / 16;
	       cbval2 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */

	       R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
	       G1 = gray_clip [yval1 + Cr_g_tab [crval1] + Cb_g_tab [cbval1]];
	       B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
	       R2 = gray_clip [yval2 + Cr_r_tab [crval2]];
	       G2 = gray_clip [yval2 + Cr_g_tab [crval2] + Cb_g_tab [cbval2]];
	       B2 = gray_clip [yval2 + Cb_b_tab [cbval2]];

	       *dst++ = B1 | (G1 << 8) | (R1 << 16) | (B2 << 24);
	       *dst   = G2 | (R2 << 8);

#ifdef HAVE_SIGNED_SHIFT
	       yval1  = (*yptr++ >> 4) + 128;
	       yval2  = (*yptr++ >> 4) + 128;
	       crval1 = *crptr++ >> 4;
	       crval2 = *crptr++ >> 4;
	       cbval1 = *cbptr++ >> 4;
	       cbval2 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
	       yval1  = *yptr++  / 16 + 128;
	       yval2  = *yptr++  / 16 + 128;
	       crval1 = *crptr++ / 16;
	       crval2 = *crptr++ / 16;
	       cbval1 = *cbptr++ / 16;
	       cbval2 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */

	       R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
	       G1 = gray_clip [yval1 + Cr_g_tab [crval1] + Cb_g_tab [cbval1]];
	       B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
	       R2 = gray_clip [yval2 + Cr_r_tab [crval2]];
	       G2 = gray_clip [yval2 + Cr_g_tab [crval2] + Cb_g_tab [cbval2]];
	       B2 = gray_clip [yval2 + Cb_b_tab [cbval2]];

	       *dst++ |= (B1 << 16) | (G1 << 24);
	       *dst++ = R1 | (B2 << 8) | (G2 << 16) | (R2 << 24);
	    }
	 }
      }
   }
   else
   {
      unsigned int *dst;		/* pointer to dithered pixels */
      word_t	   *src;		/* current pixel of frame */
      unsigned int *y_table;

      y_table = private->y_table;
      dst     = (unsigned int *) out;
      src     = image->pixels [GRAY];

      if (private->double_resolution)
      {
	 int	   x, y;		/* pixel counter */
	 unsigned *shift_clipping = gray_clip + 128;

	 for (y = image->height; y; y--)
	 {
	    for (x = image->width >> 1; x; x--)
	    {
	       unsigned int val1, val2;
#ifdef HAVE_SIGNED_SHIFT
	       val1 = shift_clipping [*src++ >> 4];
	       val2 = shift_clipping [*src++ >> 4];
#else /* not HAVE_SIGNED_SHIFT */
	       val1 = shift_clipping [*src++ / 16];
	       val2 = shift_clipping [*src++ / 16];
#endif /* not HAVE_SIGNED_SHIFT */

	       *dst++ = val1 | (val1 << 8) | (val1 << 16) | (val1 << 24);
	       *dst++ = val1 | (val1 << 8) | (val2 << 16) | (val2 << 24); 
	       *dst++ = val2 | (val2 << 8) | (val2 << 16) | (val2 << 24);
	    }

	    memcpy (dst, dst - 3 * (image->width >> 1),
		    3 * (image->width >> 1) * sizeof (unsigned int));
	    dst += 3 * (image->width >> 1);
	 }
      }
      else
      {
	 int	   n;			/* pixel counter */
	 unsigned *shift_clipping = gray_clip + 128;

	 for (n = (image->width * image->height) >> 2; n; n--)
	 {
	    unsigned int val1, val2;

#ifdef HAVE_SIGNED_SHIFT
	    val1 = shift_clipping [*src++ >> 4];
	    val2 = shift_clipping [*src++ >> 4];
#else /* not HAVE_SIGNED_SHIFT */
	    val1 = shift_clipping [*src++ / 16];
	    val2 = shift_clipping [*src++ / 16];
#endif /* not HAVE_SIGNED_SHIFT */

	    *dst++ = val1 | (val1 << 8)
		     | (val1 << 16) | (val2 << 24);  /* RGBR */
	    *dst   = val2 | (val2 << 8);             /* GB-- */

#ifdef HAVE_SIGNED_SHIFT
	    val1 = shift_clipping [*src++ >> 4];
	    val2 = shift_clipping [*src++ >> 4];
#else /* not HAVE_SIGNED_SHIFT */
	    val1 = shift_clipping [*src++ / 16];
	    val2 = shift_clipping [*src++ / 16];
#endif /* not HAVE_SIGNED_SHIFT */
	    
	    *dst++ |= (val1 << 16) | (val1 << 24);   /* --RG */
	    *dst++  = val1 | (val2 << 8)
		      | (val2 << 16) | (val2 << 24); /* BRGB */
	 }
      }
   }
   
   return 1;
}

static int 
display_24_bit_rgb (const struct fiasco_renderer *this, unsigned char *ximage,
		    const fiasco_image_t *fiasco_image)
/*
 *  Convert 'image' to 16 bit color bitmap.
 *  If 'double_resolution' is true then double image size in both directions.
 *
 *  No return value.
 *
 *  Side effects:
 *	'out[]'	is filled with dithered image
 */
{
   unsigned 	      *gray_clip = init_clipping ();
   const image_t      *image;
   renderer_private_t *private;
   byte_t	      *out;

   if (!gray_clip)
      return 0;
   if (!this)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "this");
      return 0;
   }
   if (!ximage)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "ximage");
      return 0;
   }
   if (!fiasco_image)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "fiasco_image");
      return 0;
   }

   out 	   = (byte_t *) ximage;
   image   = cast_image ((fiasco_image_t *) fiasco_image);
   if (!image)
      return 0;
   private = (renderer_private_t *) this->private;
   
   if (image->color)
   {
      word_t 	   *cbptr, *crptr;	/* pointer to chroma bands */
      word_t 	   *yptr;		/* pointers to lumincance band */
      int 	   *Cr_r_tab, *Cr_g_tab, *Cb_g_tab, *Cb_b_tab;
      unsigned int *r_table, *g_table, *b_table;

      Cr_g_tab = private->Cr_g_tab;
      Cr_r_tab = private->Cr_r_tab;
      Cb_b_tab = private->Cb_b_tab;
      Cb_g_tab = private->Cb_g_tab;
      r_table  = private->r_table;
      g_table  = private->g_table;
      b_table  = private->b_table;
      yptr     = image->pixels [Y];
      cbptr    = image->pixels [Cb];
      crptr    = image->pixels [Cr];

      if (image->format == FORMAT_4_2_0)
      {
	 if (private->double_resolution)
	 {
	    int		  yval1;	/* lumincance pixel */
	    int 	  crval1, cbval1; /* chroma pixels */
	    int		  yval2;	/* pixel in YCbCr color space */
	    unsigned int  R1, G1, B1;	/* pixel in RGB color space */
	    unsigned int  R2, G2, B2;	/* pixel in RGB color space */
	    int		  x, y;		/* pixel counter */
	    unsigned int *dst;		/* pointer to dithered pixels */
	    unsigned int *dst2;		/* pointers to dithered pixels */
	    word_t	 *yptr2;	/* pointers to lumincance band */
	    
	    dst   = (unsigned int *) out;
	    dst2  = dst + (image->width >> 1) * 3 * 2;
	    yptr2 = yptr + image->width;
	    
	    for (y = image->height >> 1; y; y--)
	    {
	       for (x = image->width >> 1; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr++ >> 4) + 128;
		  yval2  = (*yptr++ >> 4) + 128;
		  crval1 = *crptr++ >> 4;
		  cbval1 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr++  / 16 + 128;
		  yval2  = *yptr++  / 16 + 128;
		  crval1 = *crptr++ / 16;
		  cbval1 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst++ = R1 | (G1 << 8) | (B1 << 16) | (R1 << 24);
		  *dst++ = G1 | (B1 << 8) | (R2 << 16) | (G2 << 24);
		  *dst++ = B2 | (R2 << 8) | (G2 << 16) | (B2 << 24);

#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr2++ >> 4) + 128;
		  yval2  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr2++  / 16 + 128;
		  yval2  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst2++ = R1 | (G1 << 8) | (B1 << 16) | (R1 << 24);
		  *dst2++ = G1 | (B1 << 8) | (R2 << 16) | (G2 << 24);
		  *dst2++ = B2 | (R2 << 8) | (G2 << 16) | (B2 << 24);
	       }
	       memcpy (dst, dst - (image->width >> 1) * 3,
		       (image->width >> 1) * 3 * sizeof (unsigned int));
	       memcpy (dst2, dst2 - (image->width >> 1) * 3, 
		       (image->width >> 1) * 3 * sizeof (unsigned int));
	       dst   += (image->width >> 1) * 3 * 3;
	       dst2  += (image->width >> 1) * 3 * 3;
	       yptr  += image->width;
	       yptr2 += image->width;
	    }
	 }
	 else
	 {
	    int		  yval1;	/* lumincance pixel */
	    int 	  crval1, cbval1; /* chroma pixels */
	    int		  yval2;	/* pixel in YCbCr color space */
	    unsigned int  R1, G1, B1;	/* pixel in RGB color space */
	    unsigned int  R2, G2, B2;	/* pixel in RGB color space */
	    int		  x, y;		/* pixel counter */
	    unsigned int *dst;		/* pointer to dithered pixels */
	    unsigned int *dst2;		/* pointers to dithered pixels */
	    word_t	 *yptr2;	/* pointers to lumincance band */
	    
	    dst   = (unsigned int *) out;
	    dst2  = dst + (image->width >> 2) * 3;
	    yptr2 = yptr + image->width;
	    
	    for (y = image->height >> 1; y; y--)
	    {
	       for (x = image->width >> 2; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr++ >> 4) + 128;
		  yval2  = (*yptr++ >> 4) + 128;
		  crval1 = *crptr++ >> 4;
		  cbval1 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr++  / 16 + 128;
		  yval2  = *yptr++  / 16 + 128;
		  crval1 = *crptr++ / 16;
		  cbval1 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst++ = R1 | (G1 << 8) | (B1 << 16) | (R2 << 24);
		  *dst   = G2 | (B2 << 8);

#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr2++ >> 4) + 128;
		  yval2  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr2++  / 16 + 128;
		  yval2  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst2++ = R1 | (G1 << 8) | (B1 << 16) | (R2 << 24);
		  *dst2   = G2 | (B2 << 8);
	       
#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr++ >> 4) + 128;
		  yval2  = (*yptr++ >> 4) + 128;
		  crval1 = *crptr++ >> 4;
		  cbval1 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr++  / 16 + 128;
		  yval2  = *yptr++  / 16 + 128;
		  crval1 = *crptr++ / 16;
		  crval2 = *crptr++ / 16;
		  cbval1 = *cbptr++ / 16;
		  cbval2 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst++ |= (R1 << 16) | (G1 << 24);
		  *dst++ = B1 | (R2 << 8) | (G2 << 16) | (B2 << 24);

#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr2++ >> 4) + 128;
		  yval2  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr2++  / 16 + 128;
		  yval2  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */

		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval1]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval1]];

		  *dst2++ |= (R1 << 16) | (G1 << 24);
		  *dst2++ = B1 | (R2 << 8) | (G2 << 16) | (B2 << 24);
	       }
	       dst   += (image->width >> 2) * 3;
	       dst2  += (image->width >> 2) * 3;
	       yptr  += image->width;
	       yptr2 += image->width;
	    }
	 }
      }
      else				/* 4:4:4 format */
      {
	 if (private->double_resolution)
	 {
	    unsigned int R1, G1, B1;		/* pixel1 in RGB color space */
	    unsigned int R2, G2, B2;		/* pixel2 in RGB color space */
	    int		 yval1, crval1, cbval1;	/* pixel1 in YCbCr space */
	    int		 yval2, crval2, cbval2;	/* pixel2 in YCbCr space */
	    int		 x, y;			/* pixel counter */
	    unsigned int *dst;		        /* dithered pixel pointer */
	    
	    dst = (unsigned int *) out;
	    
	    for (y = image->height; y; y--)
	    {
	       for (x = image->width >> 1; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  yval1  = (*yptr++ >> 4) + 128;
		  yval2  = (*yptr++ >> 4) + 128;
		  crval1 = *crptr++ >> 4;
		  crval2 = *crptr++ >> 4;
		  cbval1 = *cbptr++ >> 4;
		  cbval2 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
		  yval1  = *yptr++  / 16 + 128;
		  yval2  = *yptr++  / 16 + 128;
		  crval1 = *crptr++ / 16;
		  crval2 = *crptr++ / 16;
		  cbval1 = *cbptr++ / 16;
		  cbval2 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */
		  
		  R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
		  G1 = gray_clip [yval1 + Cr_g_tab [crval1]
				  + Cb_g_tab [cbval1]];
		  B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
		  R2 = gray_clip [yval2 + Cr_r_tab [crval2]];
		  G2 = gray_clip [yval2 + Cr_g_tab [crval2]
				  + Cb_g_tab [cbval2]];
		  B2 = gray_clip [yval2 + Cb_b_tab [cbval2]];

		  *dst++ = R1 | (G1 << 8) | (B1 << 16) | (R1 << 24);
		  *dst++ = G1 | (B1 << 8) | (R2 << 16) | (G2 << 24);
		  *dst++ = B2 | (R2 << 8) | (G2 << 16) | (B2 << 24);
	       }
	       memcpy (dst, dst - 3 * (image->width >> 1),
		       3 * (image->width >> 1) * sizeof (unsigned int));
	       dst += 3 * (image->width >> 1);
	    }
	 }
	 else
	 {
	    unsigned int R1, G1, B1;		/* pixel in RGB color space */
	    unsigned int R2, G2, B2;		/* pixel in RGB color space */
	    int		 yval1, crval1, cbval1;	/* pixel1 in YCbCr space */
	    int		 yval2, crval2, cbval2;	/* pixel2 in YCbCr space */
	    int		 n;			/* pixel counter */
	    unsigned int *dst;		        /* dithered pixel pointer */
	    
	    dst = (unsigned int *) out;
	    
	    for (n = (image->width * image->height) >> 2; n; n--)
	    {
#ifdef HAVE_SIGNED_SHIFT
	       yval1  = (*yptr++ >> 4) + 128;
	       yval2  = (*yptr++ >> 4) + 128;
	       crval1 = *crptr++ >> 4;
	       crval2 = *crptr++ >> 4;
	       cbval1 = *cbptr++ >> 4;
	       cbval2 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
	       yval1  = *yptr++  / 16 + 128;
	       yval2  = *yptr++  / 16 + 128;
	       crval1 = *crptr++ / 16;
	       crval2 = *crptr++ / 16;
	       cbval1 = *cbptr++ / 16;
	       cbval2 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */

	       R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
	       G1 = gray_clip [yval1 + Cr_g_tab [crval1] + Cb_g_tab [cbval1]];
	       B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
	       R2 = gray_clip [yval2 + Cr_r_tab [crval2]];
	       G2 = gray_clip [yval2 + Cr_g_tab [crval2] + Cb_g_tab [cbval2]];
	       B2 = gray_clip [yval2 + Cb_b_tab [cbval2]];

	       *dst++ = R1 | (G1 << 8) | (B1 << 16) | (R2 << 24);
	       *dst   = G2 | (B2 << 8);

#ifdef HAVE_SIGNED_SHIFT
	       yval1  = (*yptr++ >> 4) + 128;
	       yval2  = (*yptr++ >> 4) + 128;
	       crval1 = *crptr++ >> 4;
	       crval2 = *crptr++ >> 4;
	       cbval1 = *cbptr++ >> 4;
	       cbval2 = *cbptr++ >> 4;
#else /* not HAVE_SIGNED_SHIFT */
	       yval1  = *yptr++  / 16 + 128;
	       yval2  = *yptr++  / 16 + 128;
	       crval1 = *crptr++ / 16;
	       crval2 = *crptr++ / 16;
	       cbval1 = *cbptr++ / 16;
	       cbval2 = *cbptr++ / 16;
#endif /* not HAVE_SIGNED_SHIFT */

	       R1 = gray_clip [yval1 + Cr_r_tab [crval1]];
	       G1 = gray_clip [yval1 + Cr_g_tab [crval1] + Cb_g_tab [cbval1]];
	       B1 = gray_clip [yval1 + Cb_b_tab [cbval1]];
	       R2 = gray_clip [yval2 + Cr_r_tab [crval2]];
	       G2 = gray_clip [yval2 + Cr_g_tab [crval2] + Cb_g_tab [cbval2]];
	       B2 = gray_clip [yval2 + Cb_b_tab [cbval2]];

	       *dst++ |= (R1 << 16) | (G1 << 24);
	       *dst++ = B1 | (R2 << 8) | (G2 << 16) | (B2 << 24);
	    }
	 }
      }
   }
   else
   {
      unsigned int *dst;		/* pointer to dithered pixels */
      word_t	   *src;		/* current pixel of frame */
      unsigned int *y_table;

      y_table = private->y_table;
      dst     = (unsigned int *) out;
      src     = image->pixels [GRAY];

      if (private->double_resolution)
      {
	 int	   x, y;		/* pixel counter */
	 unsigned *shift_clipping = gray_clip + 128;

	 for (y = image->height; y; y--)
	 {
	    for (x = image->width >> 1; x; x--)
	    {
	       unsigned int val1, val2;
#ifdef HAVE_SIGNED_SHIFT
	       val1 = shift_clipping [*src++ >> 4];
	       val2 = shift_clipping [*src++ >> 4];
#else /* not HAVE_SIGNED_SHIFT */
	       val1 = shift_clipping [*src++ / 16];
	       val2 = shift_clipping [*src++ / 16];
#endif /* not HAVE_SIGNED_SHIFT */

	       *dst++ = val1 | (val1 << 8) | (val1 << 16) | (val1 << 24);
	       *dst++ = val1 | (val1 << 8) | (val2 << 16) | (val2 << 24); 
	       *dst++ = val2 | (val2 << 8) | (val2 << 16) | (val2 << 24);
	    }

	    memcpy (dst, dst - 3 * (image->width >> 1),
		    3 * (image->width >> 1) * sizeof (unsigned int));
	    dst += 3 * (image->width >> 1);
	 }
      }
      else
      {
	 int	   n;			/* pixel counter */
	 unsigned *shift_clipping = gray_clip + 128;

	 for (n = (image->width * image->height) >> 2; n; n--)
	 {
	    unsigned int val1, val2;

#ifdef HAVE_SIGNED_SHIFT
	    val1 = shift_clipping [*src++ >> 4];
	    val2 = shift_clipping [*src++ >> 4];
#else /* not HAVE_SIGNED_SHIFT */
	    val1 = shift_clipping [*src++ / 16];
	    val2 = shift_clipping [*src++ / 16];
#endif /* not HAVE_SIGNED_SHIFT */

	    *dst++ = val1 | (val1 << 8)
		     | (val1 << 16) | (val2 << 24);  /* RGBR */
	    *dst   = val2 | (val2 << 8);             /* GB-- */

#ifdef HAVE_SIGNED_SHIFT
	    val1 = shift_clipping [*src++ >> 4];
	    val2 = shift_clipping [*src++ >> 4];
#else /* not HAVE_SIGNED_SHIFT */
	    val1 = shift_clipping [*src++ / 16];
	    val2 = shift_clipping [*src++ / 16];
#endif /* not HAVE_SIGNED_SHIFT */
	    
	    *dst++ |= (val1 << 16) | (val1 << 24);   /* --RG */
	    *dst++  = val1 | (val2 << 8)
		      | (val2 << 16) | (val2 << 24); /* BRGB */
	 }
      }
   }
   
   return 1;
}

static int 
display_32_bit (const struct fiasco_renderer *this, unsigned char *ximage,
		const fiasco_image_t *fiasco_image)
/*
 *  Convert 'image' to 16 bit color bitmap.
 *  If 'double_resolution' is true then double image size in both directions.
 *
 *  No return value.
 *
 *  Side effects:
 *	'out[]'	is filled with dithered image
 */
{
   const image_t      *image;
   renderer_private_t *private;
   byte_t	      *out;
   
   if (!this)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "this");
      return 0;
   }
   if (!ximage)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "ximage");
      return 0;
   }
   if (!fiasco_image)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "fiasco_image");
      return 0;
   }

   out 	   = (byte_t *) ximage;
   private = (renderer_private_t *) this->private;
   image   = cast_image ((fiasco_image_t *) fiasco_image);
   if (!image)
      return 0;
   
   if (image->color)
   {
      word_t 	   *cbptr, *crptr;	/* pointer to chroma bands */
      word_t 	   *yptr;		/* pointers to lumincance band */
      int     	    yval, crval, cbval;	/* pixel value in YCbCr color space */
      int     	    R, G, B;		/* pixel value in RGB color space */
      int     	    n;			/* pixel counter */
      int     	    x, y;		/* pixel coordinates */
      int 	   *Cr_r_tab, *Cr_g_tab, *Cb_g_tab, *Cb_b_tab;
      unsigned int *r_table, *g_table, *b_table;

      Cr_g_tab = private->Cr_g_tab;
      Cr_r_tab = private->Cr_r_tab;
      Cb_b_tab = private->Cb_b_tab;
      Cb_g_tab = private->Cb_g_tab;
      r_table  = private->r_table;
      g_table  = private->g_table;
      b_table  = private->b_table;
      yptr  = image->pixels [Y];
      cbptr = image->pixels [Cb];
      crptr = image->pixels [Cr];

      if (image->format == FORMAT_4_2_0)
      {
	 unsigned int	*dst, *dst2;	/* pointers to dithered pixels */
	 word_t		*yptr2;		/* pointers to lumincance band */

	 if (private->double_resolution)
	 {
	    yptr2 = yptr + image->width;
	    dst  = (unsigned int *) out;
	    dst2 = dst + 4 * image->width;
	    for (y = image->height / 2; y; y--)
	    {
	       for (x = image->width / 2; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  crval = *crptr++ >> 4;
		  cbval = *cbptr++ >> 4;
		  yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  crval = *crptr++ / 16;
		  cbval = *cbptr++ / 16;
		  yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];
	       
#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];

#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];

#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];
	       }
	       memcpy (dst, dst - 2 * image->width,
		       2 * image->width * sizeof (unsigned int));
	       memcpy (dst2, dst2 - 2 * image->width,
		       2 * image->width * sizeof (unsigned int));
	       yptr  += image->width;
	       yptr2 += image->width;
	       dst   += 3 * image->width * 2;
	       dst2  += 3 * image->width * 2;
	    }
	 }
	 else
	 {
	    yptr2 = yptr + image->width;
	    dst   = (unsigned int *) out;
	    dst2  = dst + image->width;
	    
	    for (y = image->height / 2; y; y--)
	    {
	       for (x = image->width / 2; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  crval = *crptr++ >> 4;
		  cbval = *cbptr++ >> 4;
		  yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  crval = *crptr++ / 16;
		  cbval = *cbptr++ / 16;
		  yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];
	       
#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];

#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];

#ifdef HAVE_SIGNED_SHIFT
		  yval  = (*yptr2++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  yval  = *yptr2++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  *dst2++ = r_table [R] | g_table [G] | b_table [B];
	       }
	       yptr  += image->width;
	       yptr2 += image->width;
	       dst   += image->width;
	       dst2  += image->width;
	    }
	 }
      }
      else				/* 4:4:4 format */
      {
	 if (private->double_resolution)
	 {
	    unsigned int *dst;		/* pointer to dithered pixels */
	    
	    dst = (unsigned int *) out;
	    
	    for (y = image->height; y; y--)
	    {
	       for (x = image->width; x; x--)
	       {
#ifdef HAVE_SIGNED_SHIFT
		  crval = *crptr++ >> 4;
		  cbval = *cbptr++ >> 4;
		  yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
		  crval = *crptr++ / 16;
		  cbval = *cbptr++ / 16;
		  yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
		  R = yval + Cr_r_tab [crval];
		  G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
		  B = yval + Cb_b_tab [cbval];
		  
		  *dst++ = r_table [R] | g_table [G] | b_table [B];
		  *dst++ = r_table [R] | g_table [G] | b_table [B];
	       }
	       memcpy (dst, dst - 2 * image->width,
		       2 * image->width * sizeof (unsigned int));
	       dst += image->width * 2;
	    }
	 }
	 else
	 {
	    unsigned int *dst;		/* pointer to dithered pixels */

	    dst  = (unsigned int *) out;
	    
	    for (n = image->width * image->height; n; n--)
	    {
#ifdef HAVE_SIGNED_SHIFT
	       crval = *crptr++ >> 4;
	       cbval = *cbptr++ >> 4;
	       yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
	       crval = *crptr++ / 16;
	       cbval = *cbptr++ / 16;
	       yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */
	       R = yval + Cr_r_tab [crval];
	       G = yval + Cr_g_tab [crval] + Cb_g_tab [cbval];
	       B = yval + Cb_b_tab [cbval];

	       *dst++ = r_table [R] | g_table [G] | b_table [B];
	    }
	 }
      }
   }
   else
   {
      unsigned int *dst;		/* pointer to dithered pixels */
      word_t	   *src;		/* current pixel of frame */
      unsigned int *y_table;

      y_table = private->y_table;
      dst     = (unsigned int *) out;
      src     = image->pixels [GRAY];
      
      if (private->double_resolution)
      {
	 int x, y;			/* pixel coordinates */
	 
	 for (y = image->height; y; y--)
	 {
	    for (x = image->width; x; x--)
	    {
	       int value;

#ifdef HAVE_SIGNED_SHIFT
	       value = y_table [*src++ >> 4];
#else /* not HAVE_SIGNED_SHIFT */
	       value = y_table [*src++ / 16];
#endif /* not HAVE_SIGNED_SHIFT */
	       *dst++ = value;
	       *dst++ = value;
	    }

	    memcpy (dst, dst - 2 * image->width,
		    2 * image->width * sizeof (unsigned int));
	    dst += 2 * image->width;
	 }
      }
      else
      {
	 int n;				/* pixel counter */
	 
	 for (n = image->width * image->height; n; n--)
#ifdef HAVE_SIGNED_SHIFT
	    *dst++ = y_table [*src++ >> 4];
#else /* not HAVE_SIGNED_SHIFT */
	    *dst++ = y_table [*src++ / 16];
#endif /* not HAVE_SIGNED_SHIFT */
      }
   }
   
   return 1;
}


 
