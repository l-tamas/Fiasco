/*
 *  image.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/10/22 10:43:56 $
 *  $Author: hafner $
 *  $Revision: 5.3 $
 *  $State: Exp $
 */

#ifndef _IMAGE_H
#define _IMAGE_H

#include <stdio.h>
#include "types.h"
#include "fiasco.h"

typedef enum {FORMAT_4_4_4, FORMAT_4_2_0} format_e;

typedef struct image
/*
 *  Image data
 */
{
   char      id [7];
   unsigned  reference_count;
   unsigned  width;			/* Width of the image */
   unsigned  height;			/* Height of the image */
   bool_t    color;			/* Color or grayscale image */
   format_e  format;			/* Pixel format 4:4:4 or 4:2:0 */
   word_t   *pixels [3];		/* Pixels in short format */
} image_t;

image_t *
cast_image (fiasco_image_t *image);
image_t *
alloc_image (unsigned width, unsigned height, bool_t color, format_e format);
image_t *
clone_image (image_t *image);
void
free_image (image_t *image);
FILE *
read_pnmheader (const char *image_name, unsigned *width, unsigned *height,
		bool_t *color);
image_t *
read_image (const char *image_name);
void
write_image (const char *image_name, const image_t *image);
bool_t
same_image_type (const image_t *img1, const image_t *img2);

#endif /* not _IMAGE_H */

