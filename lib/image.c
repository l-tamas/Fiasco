/*
 *  image.c:		Input and output of PNM images.
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/10/22 10:43:56 $
 *  $Author: hafner $
 *  $Revision: 5.4 $
 *  $State: Exp $
 */

#include "config.h"

#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */

#include "types.h"
#include "macros.h"
#include "error.h"

#include "fiasco.h"
#include "misc.h"
#include "image.h"

/*****************************************************************************

				local variables
  
*****************************************************************************/

static const int RPGM_FORMAT = 'P' * 256 + '5';
static const int RPPM_FORMAT = 'P' * 256 + '6';

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
init_chroma_tables (void);
static void
gray_write (const image_t *image, FILE *output);
static void
color_write (const image_t *image, FILE *output);

/*****************************************************************************

				public code
  
*****************************************************************************/

fiasco_image_t *
fiasco_image_new (const char *filename)
/*
 *  FIASCO image constructor.
 *  Allocate memory for the FIASCO image structure and
 *  load the specified image `filename'. The image has to be in
 *  raw pgm or ppm format.
 *
 *  Return value:
 *	pointer to the new image structure
 *	or NULL in case of an error
 */
{
   try
   {
      fiasco_image_t *image = fiasco_calloc (1, sizeof (fiasco_image_t));

      image->private 	= read_image (filename);
      image->delete  	= fiasco_image_delete;
      image->get_width  = fiasco_image_get_width;
      image->get_height = fiasco_image_get_height;
      image->is_color  	= fiasco_image_is_color;

      return image;
   }
   catch
   {
      return NULL;
   }
}

void
fiasco_image_delete (fiasco_image_t *image)
/*
 *  FIASCO image destructor.
 *  Free memory of FIASCO image struct.
 *
 *  No return value.
 *
 *  Side effects:
 *	structure 'image' is discarded.
 */
{
   image_t *this = cast_image (image);

   if (!this)
      return;

   try
   {
      free_image (this);
   }
   catch
   {
      return;
   }
}

unsigned
fiasco_image_get_width (fiasco_image_t *image)
{
   image_t *this = cast_image (image);

   if (!this)
      return 0;
   else
      return this->width;
}

unsigned
fiasco_image_get_height (fiasco_image_t *image)
{
   image_t *this = cast_image (image);

   if (!this)
      return 0;
   else
      return this->width;
}

int
fiasco_image_is_color (fiasco_image_t *image)
{
   image_t *this = cast_image (image);

   if (!this)
      return 0;
   else
      return this->color;
}

image_t *
cast_image (fiasco_image_t *image)
/*
 *  Cast pointer `image' to type image_t.
 *  Check whether `image' is a valid object of type image_t.
 *
 *  Return value:
 *	pointer to dfiasco_t struct on success
 *      NULL otherwise
 */
{
   image_t *this = (image_t *) image->private;
   if (this)
   {
      if (!streq (this->id, "IFIASCO"))
      {
	 set_error (_("Parameter `image' doesn't match required type."));
	 return NULL;
      }
   }
   else
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "image");
   }

   return this;
}

image_t *
alloc_image (unsigned width, unsigned height, bool_t color, format_e format)
/*
 *  Image constructor:
 *  Allocate memory for the image_t structure.
 *  Image size is given by 'width' and 'height'.
 *  If 'color' == YES then allocate memory for three color bands (Y, Cb, Cr).
 *  otherwise just allocate memory for a grayscale image.
 *  'format' specifies whether image pixels of color images
 *  are stored in 4:4:4 or 4:2:0 format.
 *
 *  Return value:
 *	pointer to the new image structure.
 */
{
   image_t *image;
   color_e band;

   if ((width & 1) || (height & 1))
      error ("Width and height of images must be even numbers.");
   if (!color)
      format = FORMAT_4_4_4;

   image         	  = fiasco_calloc (1, sizeof (image_t));
   image->width  	  = width;
   image->height 	  = height;
   image->color  	  = color;
   image->format 	  = format;
   image->reference_count = 1;
   
   image->id = strdup ("IFIASCO");

   for (band = first_band (color); band <= last_band (color); band++)
      if (format == FORMAT_4_2_0 && band != Y)
	 image->pixels [band] = fiasco_calloc ((width * height) >> 2,
					sizeof (word_t));
      else
	 image->pixels [band] = fiasco_calloc (width * height, sizeof (word_t));
   
   return image;
}

image_t *
clone_image (image_t *image)
/*
 *  Copy constructor:
 *  Construct new image by copying the given `image'.
 *
 *  Return value:
 *	pointer to the new image structure.
 */
{
   image_t *new = alloc_image (image->width, image->height, image->color,
			       image->format);
   color_e band;
   
   for (band = first_band (new->color); band <= last_band (new->color); band++)
      if (new->format == FORMAT_4_2_0 && band != Y)
      {
	 memcpy (new->pixels [band], image->pixels [band],
		 ((new->width * new->height) >> 2) * sizeof (word_t));
      }
      else
      {
	 memcpy (new->pixels [band], image->pixels [band],
		 new->width * new->height * sizeof (word_t));
      }

   return new;
}

void
free_image (image_t *image)
/*
 *  Image destructor:
 *  Free memory of 'image' struct and pixel data.
 *
 *  No return value.
 *
 *  Side effects:
 *	structure 'image' is discarded.
 */
{
   if (image != NULL)
   {
      if (--image->reference_count)
	 return;			/* image is still referenced */
      else
      {
	 fiasco_free (image->id);

	 color_e band;

	 for (band  = first_band (image->color);
	      band <= last_band (image->color); band++)
	    if (image->pixels [band])
	       fiasco_free (image->pixels [band]);
	 fiasco_free (image);
      }
   }
   else
      warning ("Can't free image <NULL>.");
}


FILE *
read_pnmheader (const char *image_name, unsigned *width, unsigned *height,
		bool_t *color)
/*
 *  Read header of raw PNM image file 'image_name'.
 *  
 *  Return values:
 *	return		File to image stream (pos. at beginning of raw data)
 *
 *  Side effects:
 *	'*width'	Width of image
 *	'*height'	Height of image
 *	'*color'	Color image flag
 */
{
   FILE	*infile;			/* input stream */
   int	 magic;				/* magic number */
   int   dummy;
   
   assert (width && height && color);
   
   infile = open_file (image_name, "FIASCO_IMAGES", READ_ACCESS);
   if (infile == NULL)
      file_error (image_name ? image_name : "stdin");

   magic  = fgetc (infile) << 8;
   magic += fgetc (infile);

   if (magic == RPGM_FORMAT)
      *color = NO;
   else if (magic == RPPM_FORMAT)
      *color = YES;
   else
      error ("%s: image format '%c%c' not supported.",
	     image_name, magic >> 8, magic % 256);

   dummy = read_int (infile);
   if (dummy < 32)
      error ("Width of image `%s' has to be at least 32 pixels.");
   *width = dummy;

   dummy = read_int (infile);
   if (dummy < 32)
      error ("Height of image `%s' has to be at least 32 pixels.");
   *height = dummy;

   dummy = read_int (infile);

   if (fgetc (infile) == EOF)
      error ("%s: EOF reached, input seems to be truncated!");

   return infile;
}

image_t *
read_image (const char *image_name)
/*
 *  Read image 'image_name'.
 *  
 *  Return value:
 *	pointer to the image structure.
 */
{
   FILE	    *input;			/* input stream */
   image_t  *image;			/* pointer to new image structure */
   unsigned  width, height;		/* image size */
   bool_t    color;			/* color image ? (YES/NO) */

   input = read_pnmheader (image_name, &width, &height, &color);
   image = alloc_image (width, height, color, FORMAT_4_4_4);

   if (!color)				/* grayscale image */
   {
      word_t   *ptr = image->pixels [GRAY]; /* pointer to pixels */
      unsigned  n;			/* pixel counter */
      
      for (n = 0; n < width * height; n++)
      {
	 int gray = getc (input);

	 if (gray == EOF)
	    file_error (image_name ? image_name : "stdin");
	 else
	    *ptr++ = (gray - 128) * 16;
      }
   }
   else
   {
      word_t   *lu, *cb, *cr;		/* pointer to pixels of color band */
      int     	red, green, blue;	/* color values */
      unsigned  n;			/* pixel counter */
      
      lu = image->pixels [Y]; 
      cb = image->pixels [Cb];
      cr = image->pixels [Cr];
      for (n = 0; n < width * height; n++)
      {
	 if ((red = getc (input)) == EOF)
	    file_error (image_name ? image_name : "stdin");
	 if ((green = getc (input)) == EOF)
	    file_error (image_name ? image_name : "stdin");
	 if ((blue = getc (input)) == EOF)
	    file_error (image_name ? image_name : "stdin");
	 
	 *lu++ = (+ 0.2989 * red + 0.5866 * green + 0.1145 * blue - 128) * 16;
	 *cb++ = (- 0.1687 * red - 0.3312 * green + 0.5000 * blue) * 16;
	 *cr++ = (+ 0.5000 * red - 0.4183 * green - 0.0816 * blue) * 16;
      }
   }
   fclose (input);
   
   return image;
}   

void
write_image (const char *image_name, const image_t *image)
/*
 *  Write given 'image' data to the file 'image_name'.
 *  
 *  No return value.
 */
{
   FILE	*output;			/* output stream */

   assert (image && image_name);
   
   if (image->format == FORMAT_4_2_0)
   {
      warning ("Writing of images in 4:2:0 format not supported.");
      return;
   }
   
   output = open_file (image_name, "FIASCO_IMAGES", WRITE_ACCESS);
   if(output == NULL) 
      file_error (image_name ? image_name : "stdout");

   fprintf (output, "%s\n%d %d\n255\n", image->color ? "P6" : "P5",
	    image->width, image->height);	

   if (image->color == NO)		/* Grayscale image */
      gray_write (image, output);
   else					/* Color image */
      color_write (image, output);

   fclose (output);
}

bool_t
same_image_type (const image_t *img1, const image_t *img2)
/*
 *  Check whether the given images 'img1' and `img2' are of the same type.
 *
 *  Return value:
 *	YES	if images 'img1' and `img2' are of the same type
 *	NO	otherwise.
 */
{
   assert (img1 && img2);
   
   return ((img1->width == img2->width)
	   && (img1->height == img2->height)
	   && (img1->color == img2->color)
	   && (img1->format == img2->format));
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static void
gray_write (const image_t *image, FILE *output)
/*
 *  Write grayscale 'image' to 'output' stream.
 *
 *  No return value.
 */
{
   word_t   *pixels;			/* pointer to current pixel */
   unsigned  n;				/* number of pixels to write */
   unsigned *gray_clip;			/* clipping table */

   gray_clip  = init_clipping ();	/* mapping of int -> unsigned */
   if (!gray_clip)
      error (fiasco_get_error_message ());
   gray_clip += 128;			/* [-128, 127] -> [0,255] */
   
   pixels = image->pixels [GRAY];
   for (n = image->width * image->height; n; n--)
   {
      unsigned value;			/* int representation of pixel */
      
#ifdef HAVE_SIGNED_SHIFT
      value = gray_clip [*pixels++ >> 4];
#else /* not HAVE_SIGNED_SHIFT */
      value = gray_clip [*pixels++ / 16];
#endif /* not HAVE_SIGNED_SHIFT */
      if (putc (value, output) != (int) value)
	 error ("Can't write image pixel %d.", n);
   }
}

static int *Cr_r_tab = NULL;
static int *Cr_g_tab = NULL;
static int *Cb_g_tab = NULL;
static int *Cb_b_tab = NULL;

static void
init_chroma_tables (void)
/*
 *  Chroma tables are used to perform fast YCbCr->RGB color space conversion.
 */
{
   int crval, cbval, i;

   if (Cr_r_tab != NULL || Cr_g_tab != NULL ||
       Cb_g_tab != NULL || Cb_b_tab != NULL)
      return;

   Cr_r_tab = fiasco_calloc (768, sizeof (int));
   Cr_g_tab = fiasco_calloc (768, sizeof (int));
   Cb_g_tab = fiasco_calloc (768, sizeof (int));
   Cb_b_tab = fiasco_calloc (768, sizeof (int));

   for (i = 256; i < 512; i++)
   {
      cbval = crval  = i - 128 - 256;

      Cr_r_tab[i] =  1.4022 * crval + 0.5;
      Cr_g_tab[i] = -0.7145 * crval + 0.5;
      Cb_g_tab[i] = -0.3456 * cbval + 0.5; 
      Cb_b_tab[i] =  1.7710 * cbval + 0.5;
   }
   for (i = 0; i < 256; i++)
   {
      Cr_r_tab[i] = Cr_r_tab[256];
      Cr_g_tab[i] = Cr_g_tab[256];
      Cb_g_tab[i] = Cb_g_tab[256]; 
      Cb_b_tab[i] = Cb_b_tab[256];
   }
   for (i = 512; i < 768; i++)
   {
      Cr_r_tab[i] = Cr_r_tab[511];
      Cr_g_tab[i] = Cr_g_tab[511];
      Cb_g_tab[i] = Cb_g_tab[511]; 
      Cb_b_tab[i] = Cb_b_tab[511];
   }

   Cr_r_tab += 256 + 128;
   Cr_g_tab += 256 + 128;
   Cb_g_tab += 256 + 128;
   Cb_b_tab += 256 + 128;
}

static void
color_write (const image_t *image, FILE *output)
/*
 *  Write grayscale 'image' to 'output' stream.
 *
 *  No return value.
 */
{
   word_t   *yptr, *cbptr, *crptr;	/* pointer to current pixel */
   unsigned *gray_clip;			/* clipping table */
   unsigned  n;				/* number of pixels to write */

   gray_clip = init_clipping ();
   if (!gray_clip)
      error (fiasco_get_error_message ());
   init_chroma_tables ();

   yptr  = image->pixels [Y];
   cbptr = image->pixels [Cb];
   crptr = image->pixels [Cr];

   for (n = image->width * image->height; n; n--)
   {
      int yval, cbval, crval;		/* pixel values of different bands */
      int value;			/* integer value of current pixel */
      
#ifdef HAVE_SIGNED_SHIFT
      crval = *crptr++ >> 4;
      cbval = *cbptr++ >> 4;
      yval  = (*yptr++ >> 4) + 128;
#else /* not HAVE_SIGNED_SHIFT */
      crval = *crptr++ / 16;
      cbval = *cbptr++ / 16;
      yval  = *yptr++  / 16 + 128;
#endif /* not HAVE_SIGNED_SHIFT */

      value = gray_clip [yval + Cr_r_tab [crval]];
      if (putc (value, output) != value)
	 error ("Can't write red component of image pixel %d.", n);

      value = gray_clip [yval + Cr_g_tab [crval] + Cb_g_tab [cbval]];
      if (putc (value, output) != value)
	 error ("Can't write green component of image pixel %d.", n);

      value = gray_clip [yval + Cb_b_tab [cbval]];
      if (putc (value, output) != value)
	 error ("Can't write blue component of image pixel %d.", n);
   }
}
