/*
 *  tiling.c:		Subimage permutation
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:50:51 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#include "config.h"

#if STDC_HEADERS
#	include <stdlib.h>
#endif /* not STDC_HEADERS */

#include "types.h"
#include "macros.h"
#include "error.h"

#include "image.h"
#include "misc.h"
#include "wfalib.h"
#include "tiling.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static int
cmpdecvar (const void *value1, const void *value2);
static int
cmpincvar (const void *value1, const void *value2);

/*****************************************************************************

				public code
  
*****************************************************************************/

typedef struct var_list
{
   int	  address;			/* bintree address */
   real_t variance;			/* variance of tile */
} var_list_t;

tiling_t *
alloc_tiling (fiasco_tiling_e method, unsigned tiling_exponent,
	      unsigned image_level)
/*
 *  Image tiling constructor.
 *  Allocate memory for the tiling_t structure.
 *  `method' defines the tiling method (spiral or variance,
 *  ascending or descending).
 *  In case of invalid parameters, a structure with tiling.exponent == 0 is
 *  returned. 
 *
 *  Return value
 *	pointer to the new tiling structure on success
 */
{
   tiling_t *tiling = Calloc (1, sizeof (tiling_t));

   if ((int) image_level - (int) tiling_exponent < 6)
   {
      tiling_exponent = 6;
      warning (_("Image tiles must be at least 8x8 pixels large.\n"
		 "Setting tiling size to 8x8 pixels."));
   }
   
   switch (method)
   {
      case FIASCO_TILING_SPIRAL_ASC:
      case FIASCO_TILING_SPIRAL_DSC:
      case FIASCO_TILING_VARIANCE_ASC:
      case FIASCO_TILING_VARIANCE_DSC:
	 tiling_exponent = tiling_exponent;
	 break;
      default:
	 warning (_("Invalid tiling method specified. Disabling tiling."));
	 tiling_exponent = 0;
	 break;
   }

   return tiling;
}

void
free_tiling (tiling_t *tiling)
/*
 *  Tiling struct destructor:
 *  Free memory of 'tiling' struct.
 *
 *  No return value.
 *
 *  Side effects:
 *	structure 'tiling' is discarded.
 */
{
   if (tiling->vorder)
      Free (tiling->vorder);
   Free (tiling);
}

void
perform_tiling (const image_t *image, tiling_t *tiling)
/*
 *  Compute image tiling permutation.
 *  The image is split into 2**'tiling->exponent' tiles.
 *  Depending on 'tiling->method', the following algorithms are used:
 *  "VARIANCE_ASC" :  Tiles are sorted by variance.
 *                    The first tile has the lowest variance
 *  "VARIANCE_DSC" :  Tiles are sorted by variance.
 *                    The first tile has the largest variance
 *  "SPIRAL_ASC" :    Tiles are sorted like a spiral starting
 *                    in the middle of the image.
 *  "SPIRAL_DSC" :    Tiles are sorted like a spiral starting
 *                    in the upper left corner.
 *
 *  No return value.
 *
 *  Side effects:
 *	The tiling permutation is stored in 'tiling->vorder'.
 */
{
   if (tiling->exponent)
   {
      unsigned 	tiles = 1 << tiling->exponent; /* number of image tiles */
      bool_t   *tile_valid;		/* tile i is in valid range ? */
      
      tiling->vorder = Calloc (tiles, sizeof (int));
      tile_valid     = Calloc (tiles, sizeof (bool_t));

      if (tiling->method == FIASCO_TILING_VARIANCE_ASC
	  || tiling->method == FIASCO_TILING_VARIANCE_DSC)
      {
	 unsigned    address;		/* bintree address of tile */
	 unsigned    number;		/* number of image tiles */
	 unsigned    lx       = log2 (image->width - 1) + 1; /* x level */
	 unsigned    ly       = log2 (image->height - 1) + 1; /* y level */
	 unsigned    level    = max (lx, ly) * 2 - ((ly == lx + 1) ? 1 : 0);
	 var_list_t *var_list = Calloc (tiles, sizeof (var_list_t));
	 
	 /*
	  *  Compute variances of image tiles
	  */
	 for (number = 0, address = 0; address < tiles; address++)
	 {
	    unsigned width, height;	/* size of image tile */
	    unsigned x0, y0;		/* NW corner of image tile */
      
	    locate_subimage (level, level - tiling->exponent, address,
			     &x0, &y0, &width, &height);
	    if (x0 < image->width && y0 < image->height) /* valid range */
	    {
	       if (x0 + width > image->width)	/* outside image area */
		  width = image->width - x0;
	       if (y0 + height > image->height) /* outside image area */
		  height = image->height - y0;

	       var_list [number].variance
		  = variance (image->pixels [GRAY], x0, y0,
			      width, height, image->width);
	       var_list [number].address  = address;
	       number++;
	       tile_valid [address] = YES;
	    }
	    else
	       tile_valid [address] = NO;
	 }

	 /*
	  *  Sort image tiles according to sign of 'tiling->exp'
	  */
	 if (tiling->method == FIASCO_TILING_VARIANCE_DSC)
	    qsort (var_list, number, sizeof (var_list_t), cmpdecvar);
	 else
	    qsort (var_list, number, sizeof (var_list_t), cmpincvar);

	 for (number = 0, address = 0; address < tiles; address++)
	    if (tile_valid [address])
	    {
	       tiling->vorder [address] = var_list [number].address;
	       number++;
	       debug_message ("tile number %d has original address %d",
			      number, tiling->vorder [address]);
	    }
	    else
	       tiling->vorder [address] = -1;

	 Free (var_list);
      }
      else if (tiling->method == FIASCO_TILING_SPIRAL_DSC
	       || tiling->method == FIASCO_TILING_SPIRAL_ASC)
      {
	 compute_spiral (tiling->vorder, image->width, image->height,
			 tiling->exponent,
			 tiling->method == FIASCO_TILING_SPIRAL_ASC);
      }
      else
      {
	 warning ("Unsupported image tiling method.\n"
		  "Skipping image tiling step.");
	 tiling->exponent = 0;
      }
   }
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static int
cmpincvar (const void *value1, const void *value2)
/*
 *  Sorts by increasing variances (quicksort sorting function).
 */
{
  return ((var_list_t *) value1)->variance - ((var_list_t *) value2)->variance;
}

static int
cmpdecvar (const void *value1, const void *value2)
/*
 *  Sorts by decreasing variances (quicksort sorting function).
 */
{
  return ((var_list_t *) value2)->variance - ((var_list_t *) value1)->variance;
}
