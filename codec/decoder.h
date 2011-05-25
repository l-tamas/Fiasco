/*
 *  decode.h
 *		
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/10/22 10:44:48 $
 *  $Author: hafner $
 *  $Revision: 5.3 $
 *  $State: Exp $
 */

#ifndef _DECODE_H
#define _DECODE_H

#include "types.h"
#include "image.h"
#include "wfa.h"

typedef struct video
{
   unsigned  future_display;		/* number of a future frame */
   unsigned  display;			/* current display number */
   image_t  *frame;			/* current frame */
   image_t  *sframe;			/* current smoothed frame */
   image_t  *future;			/* future reference */
   image_t  *sfuture;			/* future (smmothed) reference */
   image_t  *past ;			/* past reference */
   wfa_t    *wfa;			/* current wfa */
   wfa_t    *wfa_future;		/* future wfa */
   wfa_t    *wfa_past;			/* past wfa */
} video_t;

typedef struct dectimer
{
   unsigned int	input [3];
   unsigned int	preprocessing [3];
   unsigned int	decoder [3];
   unsigned int	cleanup [3];
   unsigned int	motion [3];
   unsigned int	smooth [3];
   unsigned int	display [3];
   unsigned int	frames [3];
} dectimer_t;

image_t *
get_next_frame (bool_t store_wfa, int enlarge_factor,
		int smoothing, const char *reference_frame,
		format_e format, video_t *video, dectimer_t *timer,
		wfa_t *orig_wfa, bitfile_t *input);
image_t *
decode_image (unsigned orig_width, unsigned orig_height, format_e format,
	      unsigned *dec_timer, const wfa_t *wfa);
word_t *
decode_range (unsigned range_state, unsigned range_label, unsigned range_level,
	      word_t **domain, wfa_t *wfa);
image_t *
decode_state (unsigned state, unsigned level, wfa_t *wfa);
void
smooth_image (unsigned sf, const wfa_t *wfa, image_t *image);
video_t *
alloc_video (bool_t store_wfa);
void
free_video (video_t *video);

#endif /* not _DECODE_H */
