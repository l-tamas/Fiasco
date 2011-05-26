/*
 *  pnmerr.c:		Compute error (RMSE, PSNR) between images
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:51:17 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#include "config.h"

#if STDC_HEADERS
#	include <stdlib.h>
#endif /* not STDC_HEADERS */
#include <stdio.h>
#include <math.h>

#include "types.h"
#include "macros.h"
#include "binerror.h"

#include "fiasco.h"

#define MAXFILES 16

/*****************************************************************************

				public code
  
*****************************************************************************/

int
main (int argc, char **argv)
{
   fiasco_image_t    *img1, *img2;
   fiasco_renderer_t *renderer;
   real_t 	      norm, y_norm, cb_norm, cr_norm;
   unsigned char     *buffer1, *buffer2, *b1, *b2;
   
   init_error_handling (argv[0]);

   if (argc < 3)
   {
      fprintf (stderr, "%s: usage: %s original-image regenerated-image\n",
	       argv[0], argv[0]);
      exit (1);
   }

   /*
    *  Load both images and convert pixel values to RGB format
    */
   if (!(img1 = fiasco_image_new (argv [1])))
      error (fiasco_get_error_message ());
   if (!(img2 = fiasco_image_new (argv [2])))
      error (fiasco_get_error_message ());
   if (img1->get_width (img1) != img2->get_width (img2)
       || img1->get_height (img1) != img2->get_height (img2)
       || img1->is_color (img1) != img2->is_color (img2))
      error ("Images have to be of same size and format.");
   if (!(img1->get_width (img1) * img2->get_height (img2)))
      error ("Image width and height have to be positive.");
   if (!(renderer = fiasco_renderer_new (0xff0000L, 0xff00L, 0xffL, 24, 0)))
      error (fiasco_get_error_message ());
   b1 = buffer1 = calloc (img1->get_width (img1) * img1->get_height (img1) * 3,
			  sizeof (char));
   b2 = buffer2 = calloc (img1->get_width (img1) * img1->get_height (img1) * 3,
			  sizeof (char));
   if (!buffer1 || !buffer2)
      error ("Out of memory.");
   if (!renderer->render (renderer, buffer1, img1))
      error (fiasco_get_error_message ());
   if (!renderer->render (renderer, buffer2, img2))
      error (fiasco_get_error_message ());
   
   y_norm = cb_norm = cr_norm = norm = 0;

   /*
    *  Compute errors
    */
   if (!img1->is_color (img1)) /* grayscale image */
   {
      unsigned n;
      
      for (n = img1->get_width (img1) * img1->get_height (img1); n; n--)
      {
	 norm += square (*buffer1 - *buffer2);
	 buffer1++;
	 buffer1++;
	 buffer1++;
	 buffer2++;
	 buffer2++;
	 buffer2++;
      }
      norm /= img1->get_width (img1) * img1->get_height (img1);
      if (norm > 1e-4)
	 fprintf (stderr, "PSNR between %s and %s: %.2f dB\n",
		  argv [1], argv [2],
		  10 * log ( 255.0 * 255.0 / norm) / log (10.0));
      else
	 fprintf (stderr, "Images %s and %s don't differ.\n",
		  argv [1], argv [2]);
   }
   else
   {
      unsigned n;

      for (n = img1->get_width (img1) * img1->get_height (img1); n; n--)
      {
	 byte_t red1, red2, green1, green2, blue1, blue2;
	 real_t y1, y2, cb1, cb2, cr1, cr2;
	 
	 red1   = *buffer1++;
	 green1 = *buffer1++;
	 blue1  = *buffer1++;
	 red2   = *buffer2++;
	 green2 = *buffer2++;
	 blue2  = *buffer2++;
	 y1 	= (+ 0.2989 * red1 + 0.5866 * green1 + 0.1145 * blue1);
	 cb1 	= (- 0.1687 * red1 - 0.3312 * green1 + 0.5000 * blue1);
	 cr1 	= (+ 0.5000 * red1 - 0.4183 * green1 - 0.0816 * blue1);
	 y2 	= (+ 0.2989 * red2 + 0.5866 * green2 + 0.1145 * blue2);
	 cb2 	= (- 0.1687 * red2 - 0.3312 * green2 + 0.5000 * blue2);
	 cr2 	= (+ 0.5000 * red2 - 0.4183 * green2 - 0.0816 * blue2);

	 y_norm += square (y1 - y2);
	 cb_norm += square (cb1 - cb2);
	 cr_norm += square (cr1 - cr2);
      }
      y_norm /= img1->get_width (img1) * img1->get_height (img1);
      cb_norm /= img1->get_width (img1) * img1->get_height (img1);
      cr_norm  /= img1->get_width (img1) * img1->get_height (img1);
      fprintf (stderr, "PSNR between %s and %s:\n", argv [1], argv [2]);
      if (y_norm > 1e-4)
	 fprintf (stderr, "Y  color component: %.2f dB\n",
		  10 * log ( 255.0 * 255.0 / y_norm) / log (10.0));
      else
	 fprintf (stderr, "Y color component doesn't differ.\n");
      if (cb_norm > 1e-4)
	 fprintf (stderr, "Cb color component: %.2f dB\n",
		  10 * log ( 255.0 * 255.0 / cb_norm) / log (10.0));
      else
	 fprintf (stderr, "Cb color component  doesn't differ.\n");
      if (cr_norm > 1e-4)
	 fprintf (stderr, "Cr color component: %.2f dB\n",
		  10 * log ( 255.0 * 255.0 / cr_norm) / log (10.0));
      else
	 fprintf (stderr, "Cr color component doesn't differ.\n");
   }

   free (b1);
   free (b2);
   img1->delete (img1);
   img2->delete (img2);
   renderer->delete (renderer);
   
   return 0;
}


