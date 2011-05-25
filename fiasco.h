/*
 *  fiasco.h		
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 */

/*
 *  $Date: 2000/10/28 17:39:28 $
 *  $Author: hafner $
 *  $Revision: 5.6 $
 *  $State: Exp $
 */

#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
# define __BEGIN_DECLS extern "C" {
# define __END_DECLS }
#else
# define __BEGIN_DECLS /* empty */
# define __END_DECLS /* empty */
#endif

#ifndef _FIASCO_H
#define _FIASCO_H 1

__BEGIN_DECLS

/****************************************************************************
			  FIASCO data types
****************************************************************************/

/*
 *  Verbosity level:
 *  FIASCO_NO_VERBOSITY:       No output at all.
 *  FIASCO_SOME_VERBOSITY:     Show progress meter during coding
 *  FIASCO_ULTIMATE_VERBOSITY: Show debugging output
 */
typedef enum {FIASCO_NO_VERBOSITY,
	      FIASCO_SOME_VERBOSITY,
	      FIASCO_ULTIMATE_VERBOSITY} fiasco_verbosity_e;
  
/*
 *  Image tiling methods:
 *  VARIANCE_ASC:  Tiles are sorted by variance.
 *                 The first tile has the lowest variance.
 *  VARIANCE_DSC:  Tiles are sorted by variance.
 *                 The first tile has the largest variance.
 *  SPIRAL_ASC:    Tiles are sorted like a spiral starting
 *                 in the middle of the image.
 *  SPIRAL_DSC:    Tiles are sorted like a spiral starting
 *                 in the upper left corner.
 */
typedef enum {FIASCO_TILING_SPIRAL_ASC,
	      FIASCO_TILING_SPIRAL_DSC,
	      FIASCO_TILING_VARIANCE_ASC,
	      FIASCO_TILING_VARIANCE_DSC} fiasco_tiling_e;

/*
 *  Range of reduced precision format:
 *  FIASCO_RPF_RANGE_0_75: use interval [-0.75,0.75]
 *  FIASCO_RPF_RANGE_1_00: use interval [-1.00,1.00]
 *  FIASCO_RPF_RANGE_1_50: use interval [-1.50,0.75]
 *  FIASCO_RPF_RANGE_2_00: use interval [-2.00,2.00]
 */
typedef enum {FIASCO_RPF_RANGE_0_75,
	      FIASCO_RPF_RANGE_1_00,
	      FIASCO_RPF_RANGE_1_50,
	      FIASCO_RPF_RANGE_2_00} fiasco_rpf_range_e;

/*
 *  Type of progress meter to be used during coding
 *  FIASCO_PROGRESS_NONE:    no output at all
 *  FIASCO_PROGRESS_BAR:     RPM style progress bar using 50 hash marks ###### 
 *  FIASCO_PROGRESS_PERCENT: percentage meter 50% 
 */
typedef enum {FIASCO_PROGRESS_NONE,
	      FIASCO_PROGRESS_BAR,
	      FIASCO_PROGRESS_PERCENT} fiasco_progress_e;

/*
 * Class to encapsulate FIASCO images.
 */
typedef struct fiasco_image
{
   void		(*delete)	(struct fiasco_image *image);
   unsigned	(*get_width)	(struct fiasco_image *image);
   unsigned	(*get_height)	(struct fiasco_image *image);
   int		(*is_color)	(struct fiasco_image *image);
   void *private;
} fiasco_image_t;

/*
 * Class to store internal state of decoder.
 */
typedef struct fiasco_decoder
{
   int			(*delete)	 (struct fiasco_decoder *decoder);
   int			(*write_frame)   (struct fiasco_decoder *decoder,
					  const char *filename);
   fiasco_image_t *	(*get_frame)     (struct fiasco_decoder *decoder);
   unsigned		(*get_length)    (struct fiasco_decoder *decoder);
   unsigned		(*get_rate)	 (struct fiasco_decoder *decoder);
   unsigned		(*get_width)	 (struct fiasco_decoder *decoder);
   unsigned		(*get_height)	 (struct fiasco_decoder *decoder);
   const char *		(*get_title)	 (struct fiasco_decoder *decoder);
   const char *		(*get_comment)	 (struct fiasco_decoder *decoder);
   int			(*is_color)	 (struct fiasco_decoder *decoder);
   void *private;
} fiasco_decoder_t;

/*
 * Class to encapsulate advanced coder options.
 */
typedef struct fiasco_c_options
{
   void (*delete)            (struct fiasco_c_options *options);
   int (*set_tiling)         (struct fiasco_c_options *options,
			      fiasco_tiling_e method,
			      unsigned exponent);
   int (*set_frame_pattern)  (struct fiasco_c_options *options,
			      const char *pattern);
   int (*set_basisfile)      (struct fiasco_c_options *options,
			      const char *filename);
   int (*set_chroma_quality) (struct fiasco_c_options *options,
			      float quality_factor,
			      unsigned dictionary_size);
   int (*set_optimizations)  (struct fiasco_c_options *options,
			      unsigned min_block_level,
			      unsigned max_block_level,
			      unsigned max_elements,
			      unsigned dictionary_size,
			      unsigned optimization_level);
   int (*set_prediction)     (struct fiasco_c_options *options,
			      int intra_prediction,
			      unsigned min_block_level,
			      unsigned max_block_level);
   int (*set_video_param)    (struct fiasco_c_options *options,
			      unsigned frames_per_second,
			      int half_pixel_prediction,
			      int cross_B_search,
			      int B_as_past_ref);
   int (*set_quantization)   (struct fiasco_c_options *options,
			      unsigned mantissa,
			      fiasco_rpf_range_e range,
			      unsigned dc_mantissa,
			      fiasco_rpf_range_e dc_range);
   int (*set_progress_meter) (struct fiasco_c_options *options,
			      fiasco_progress_e type);
   int (*set_smoothing)      (struct fiasco_c_options *options,
			      int smoothing);
   int (*set_comment)        (struct fiasco_c_options *options,
			      const char *comment);
   int (*set_title)          (struct fiasco_c_options *options,
			      const char *title);
   void *private;
} fiasco_c_options_t;

/*
 * Class to encapsulate advanced decoder options.
 */
typedef struct fiasco_d_options
{
   void (*delete)            (struct fiasco_d_options *options);
   int (*set_smoothing)      (struct fiasco_d_options *options,
			      int smoothing);
   int (*set_magnification)  (struct fiasco_d_options *options,
			      int level);
   int (*set_4_2_0_format)   (struct fiasco_d_options *options,
			      int format);
   void *private;
} fiasco_d_options_t;

/*
 * Class to convert internal FIASCO image structure to a XImage structure.
 * Method `renderer()' is used to convert internal image to XImage. 
 * Method `delete()' is used to delete and free internal image. 
 */
typedef struct fiasco_renderer
{
   int  (*render) (const struct fiasco_renderer *this,
		   unsigned char *data,
		   const fiasco_image_t *fiasco_image);
   void (*delete) (struct fiasco_renderer *this);
   void *private;
} fiasco_renderer_t;

/****************************************************************************
		       miscellaneous functions
****************************************************************************/
  
/* Get last error message of FIASCO library */
const char *fiasco_get_error_message (void);

/* Set verbosity of FIASCO library */
void fiasco_set_verbosity (fiasco_verbosity_e level);

/* Get verbosity of FIASCO library */
fiasco_verbosity_e fiasco_get_verbosity (void);

/****************************************************************************
			  decoder functions
****************************************************************************/

/* Decode FIASCO image or sequence */
fiasco_decoder_t *fiasco_decoder_new (const char *filename,
				      const fiasco_d_options_t *options);

/* Flush and discard FIASCO decoder */
int fiasco_decoder_delete (fiasco_decoder_t *decoder);

/* Decode next FIASCO frame and write to PNM image 'filename' */
int fiasco_decoder_write_frame (fiasco_decoder_t *decoder,
				const char *filename);

/* Decode next FIASCO frame to FIASCO image structure */
fiasco_image_t *fiasco_decoder_get_frame (fiasco_decoder_t *decoder);

/* Get width of FIASCO image or sequence */
unsigned fiasco_decoder_get_width (fiasco_decoder_t *decoder);

/* Get height of FIASCO image or sequence */
unsigned fiasco_decoder_get_height (fiasco_decoder_t *decoder);

/* Get width of FIASCO image or sequence */
int fiasco_decoder_is_color (fiasco_decoder_t *decoder);

/* Get frame rate of FIASCO sequence */
unsigned fiasco_decoder_get_rate (fiasco_decoder_t *decoder);

/* Get number of frames of FIASCO file */
unsigned fiasco_decoder_get_length (fiasco_decoder_t *decoder);

/* Get title of FIASCO file */
const char *
fiasco_decoder_get_title (fiasco_decoder_t *decoder);

/* Get comment of FIASCO file */
const char *
fiasco_decoder_get_comment (fiasco_decoder_t *decoder);

/****************************************************************************
			  image functions
****************************************************************************/

/* Read FIASCO image (raw ppm or pgm format) */
fiasco_image_t * fiasco_image_new (const char *filename);

/* Discard FIASCO image */
void fiasco_image_delete (fiasco_image_t *image); 

/* Get width of FIASCO image or sequence */
unsigned fiasco_image_get_width (fiasco_image_t *image);

/* Get height of FIASCO image or sequence */
unsigned fiasco_image_get_height (fiasco_image_t *image);

/* Get width of FIASCO image or sequence */
int fiasco_image_is_color (fiasco_image_t *image);

/****************************************************************************
			  renderer functions
****************************************************************************/

/* Constructor of FIASCO image structure to a XImage renderer */
fiasco_renderer_t *
fiasco_renderer_new (unsigned long red_mask, unsigned long green_mask,
		     unsigned long blue_mask, unsigned bpp,
		     int double_resolution);

/* Destructor of FIASCO image structure to a XImage renderer */
void
fiasco_renderer_delete (fiasco_renderer_t *renderer);

/* FIASCO image structure to a XImage renderer */
int
fiasco_renderer_render (const fiasco_renderer_t *renderer,
			unsigned char *ximage,
			const fiasco_image_t *fiasco_image);

/****************************************************************************
			   coder functions
****************************************************************************/

/* Encode image or sequence by FIASCO */
int fiasco_coder (char const * const *inputname,
		  const char *outputname,
		  float quality,
		  const fiasco_c_options_t *options);

/****************************************************************************
		 coder options functions
****************************************************************************/

/* FIASCO additional options constructor */
fiasco_c_options_t *fiasco_c_options_new (void);

/* FIASCO additional options destructor */
void fiasco_c_options_delete (fiasco_c_options_t *options);

/* Define `smoothing'-percentage along partitioning borders.*/
int fiasco_c_options_set_smoothing (fiasco_c_options_t *options,
				    int smoothing);

/* Set type of frame prediction for sequence of frames */
int fiasco_c_options_set_frame_pattern (fiasco_c_options_t *options,
					const char *pattern);

/* Set method and number of tiles for image tiling */
int fiasco_c_options_set_tiling (fiasco_c_options_t *options,
				 fiasco_tiling_e method,
				 unsigned exponent);

/* Set FIASCO initial basis file */
int fiasco_c_options_set_basisfile (fiasco_c_options_t *options,
				    const char *filename);

/* Set quality and dictionary size of chroma compression */
int fiasco_c_options_set_chroma_quality (fiasco_c_options_t *options,
					 float quality_factor,
					 unsigned dictionary_size);

/*
 *  Since FIASCO internally works with binary trees, all functions
 *  (which are handling image geometry) rather expect the `level' of
 *  the corresponding binary tree than the traditional `width' and
 *  `height' arguments.  Refer to following table to convert these
 *  values:
 *  
 *  level | width | height
 *  ------+-------+--------
 *    0   |    1  |    1  
 *    1   |    1  |    2  
 *    2   |    2  |    2  
 *    3   |    2  |    4  
 *    4   |    4  |    4  
 *    5   |    4  |    8  
 *    6   |    8  |    8  
 *    7   |    8  |   16
 *
 */
   
/* Set various optimization parameters. */
int fiasco_c_options_set_optimizations (fiasco_c_options_t *options,
					unsigned min_block_level,
					unsigned max_block_level,
					unsigned max_elements,
					unsigned dictionary_size,
					unsigned optimization_level);

/* Set minimum and maximum size of image block prediction */
int fiasco_c_options_set_prediction (fiasco_c_options_t *options,
				     int intra_prediction,
				     unsigned min_block_level,
				     unsigned max_block_level);

/*  Set various parameters used for video compensation */
int fiasco_c_options_set_video_param (fiasco_c_options_t *options,
				      unsigned frames_per_second,
				      int half_pixel_prediction,
				      int cross_B_search,
				      int B_as_past_ref);

/* Set accuracy of coefficients quantization */
int fiasco_c_options_set_quantization (fiasco_c_options_t *options,
				       unsigned mantissa,
				       fiasco_rpf_range_e range,
				       unsigned dc_mantissa,
				       fiasco_rpf_range_e dc_range);

/* Set type of progress meter */
int fiasco_c_options_set_progress_meter (fiasco_c_options_t *options,
					 fiasco_progress_e type);

/*  Set comment of FIASCO stream */
int fiasco_c_options_set_comment (fiasco_c_options_t *options,
				  const char *comment);

/*  Set title of FIASCO stream */
int fiasco_c_options_set_title (fiasco_c_options_t *options,
				const char *title);

/****************************************************************************
		 decoder options functions
****************************************************************************/

/* FIASCO additional options constructor */
fiasco_d_options_t *fiasco_d_options_new (void);

/* FIASCO additional options destructor */
void fiasco_d_options_delete (fiasco_d_options_t *options);


/* Define `smoothing'-percentage along partitioning borders.*/
int fiasco_d_options_set_smoothing (fiasco_d_options_t *options,
				  int smoothing);

/* Set magnification-'level' of decoded image */
int fiasco_d_options_set_magnification (fiasco_d_options_t *options,
				      int level);

/* Set image format to 4:2:0 or 4:4:4 */
int fiasco_d_options_set_4_2_0_format (fiasco_d_options_t *options,
				     int format);

__END_DECLS

#endif /* not _FIASCO_H */
