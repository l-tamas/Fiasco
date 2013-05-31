/*
 *  options.c:		FIASCO options handling
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/10/28 17:39:31 $
 *  $Author: hafner $
 *  $Revision: 5.5 $
 *  $State: Exp $
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "macros.h"
#include "error.h"

#include "wfa.h"
#include "misc.h"
#include "bit-io.h"
#include "fiasco.h"
#include "options.h"

fiasco_c_options_t *
fiasco_c_options_new (void)
/*
 *  FIASCO options constructor.
 *  Allocate memory for the FIASCO coder options structure and
 *  fill in default values.
 *
 *  Return value:
 *	pointer to the new option structure
 */
{
   c_options_t 	      *options = fiasco_calloc (1, sizeof (c_options_t));
   fiasco_c_options_t *public  = fiasco_calloc (1, sizeof (fiasco_c_options_t));

   public->private 	      = options;
   public->delete 	      = fiasco_c_options_delete;
   public->set_tiling 	      = fiasco_c_options_set_tiling;
   public->set_frame_pattern  = fiasco_c_options_set_frame_pattern;
   public->set_basisfile      = fiasco_c_options_set_basisfile;
   public->set_chroma_quality = fiasco_c_options_set_chroma_quality;
   public->set_optimizations  = fiasco_c_options_set_optimizations;
   public->set_video_param    = fiasco_c_options_set_video_param;
   public->set_quantization   = fiasco_c_options_set_quantization;
   public->set_progress_meter = fiasco_c_options_set_progress_meter;
   public->set_smoothing      = fiasco_c_options_set_smoothing;
   public->set_title   	      = fiasco_c_options_set_title;
   public->set_comment        = fiasco_c_options_set_comment;
   
   strcpy (options->id, "COFIASCO");

   /*
    *  Set default value of fiasco options
    */
   options->basis_name 		  = strdup ("small.fco");
   options->lc_min_level 	  = 4;
   options->lc_max_level 	  = 12;
   options->p_min_level 	  = 8;
   options->p_max_level 	  = 10;
   options->images_level 	  = 5;
   options->max_states 		  = MAXSTATES;
   options->chroma_max_states 	  = 40;
   options->max_elements 	  = MAXEDGES;
   options->tiling_exponent 	  = 4;
   options->tiling_method 	  = FIASCO_TILING_VARIANCE_DSC;
   options->id_domain_pool 	  = strdup ("rle");
   options->id_d_domain_pool 	  = strdup ("rle");
   options->id_rpf_model 	  = strdup ("adaptive");
   options->id_d_rpf_model 	  = strdup ("adaptive");
   options->rpf_mantissa 	  = 3;
   options->rpf_range 		  = FIASCO_RPF_RANGE_1_50;
   options->dc_rpf_mantissa 	  = 5;
   options->dc_rpf_range 	  = FIASCO_RPF_RANGE_1_00;
   options->d_rpf_mantissa 	  = 3;
   options->d_rpf_range 	  = FIASCO_RPF_RANGE_1_50;
   options->d_dc_rpf_mantissa 	  = 5;
   options->d_dc_rpf_range 	  = FIASCO_RPF_RANGE_1_00;
   options->chroma_decrease 	  = 2.0;
   options->prediction 		  = NO;
   options->delta_domains 	  = YES;
   options->normal_domains 	  = YES;
   options->search_range 	  = 16;
   options->fps 		  = 25;
   options->pattern 		  = strdup ("IPPPPPPPPP");
   options->reference_filename 	  = NULL;
   options->half_pixel_prediction = NO;
   options->cross_B_search 	  = YES;
   options->B_as_past_ref 	  = YES;
   options->check_for_underflow   = NO;
   options->check_for_overflow 	  = NO;
   options->second_domain_block   = NO;
   options->full_search 	  = NO;
   options->progress_meter 	  = FIASCO_PROGRESS_NONE;
   options->smoothing 	 	  = 70;
   options->comment 		  = strdup ("");
   options->title 		  = strdup ("");
   
   return public;
}

void
fiasco_c_options_delete (fiasco_c_options_t *options)
/*
 *  FIASCO options destructor.
 *  Free memory of FIASCO options struct.
 *
 *  No return value.
 *
 *  Side effects:
 *	structure 'options' is discarded.
 */
{
   c_options_t *this = cast_c_options (options);

   if (!this)
      return;
   
   fiasco_free (this->id_domain_pool);
   fiasco_free (this->id_d_domain_pool);
   fiasco_free (this->id_rpf_model);
   fiasco_free (this->id_d_rpf_model);
   fiasco_free (this->pattern);
   fiasco_free (this->comment);
   fiasco_free (this->title);
   
   fiasco_free (this);

   return;
}

int
fiasco_c_options_set_tiling (fiasco_c_options_t *options,
			     fiasco_tiling_e method, unsigned exponent)
/*
 *  Set tiling `method' and `exponent'.
 *  See type `fiasco_tiling_e' for a list of valid  tiling `methods'.
 *  The image is subdivied into 2^`exponent' tiles
 *
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   switch (method)
   {
      case FIASCO_TILING_SPIRAL_ASC:
      case FIASCO_TILING_SPIRAL_DSC:
      case FIASCO_TILING_VARIANCE_ASC:
      case FIASCO_TILING_VARIANCE_DSC:
	 this->tiling_method = method;
	 break;
      default:
	 set_error (_("Invalid tiling method `%d' specified "
		      "(valid methods are 0, 1, 2, or 3)."), method);
	 return 0;
   }
   this->tiling_exponent = exponent;
   
   return 1;
}

int
fiasco_c_options_set_frame_pattern (fiasco_c_options_t *options,
				    const char *pattern)
/*
 *  Set `pattern' of input frames.
 *  `pattern' has to be a sequence of the following
 *  characters (case insensitive):
 *  'i' intra frame
 *  'p' predicted frame
 *  'b' bidirectional predicted frame
 *  E.g. pattern = 'IBBPBBPBB'
 *
 *  When coding video frames the prediction type of input frame N is determined
 *  by reading `pattern' [N] (`pattern' is periodically extended).
 *  
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   else if (!pattern)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "pattern");
      return 0;
   }
   else if (strlen (pattern) < 1)
   {
      set_error (_("Frame type pattern doesn't contain any character."));
      return 0;
   }
   else
   {
      const char *str;
      bool_t 	  parse_error = NO;
      int	  c 	      = 0;
      
      for (str = pattern; *str && !parse_error; str++)
	 switch (*str)
	 {
	    case 'i':
	    case 'I':
	    case 'b':
	    case 'B':
	    case 'p':
	    case 'P':
	       break;
	    default:
	       c = *str;
	       parse_error = YES;
	 }

      if (parse_error)
      {
	 set_error (_("Frame type pattern contains invalid character `%c' "
		      "(choose I, B or P)."), c);
	 return 0;
      }
      else
      {
	 fiasco_free (this->pattern);
	 this->pattern = strdup (pattern);

	 return 1;
      }
   }
}

int
fiasco_c_options_set_basisfile (fiasco_c_options_t *options,
				const char *filename)
/*
 *  Set `filename' of FIASCO initial basis.
 *  
 *  Return value:
 *	1 on success (if the file is readable)
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   else if (!filename)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "filename");
      return 0;
   }
   else
   {
      FILE *file = open_file (filename, "FIASCO_DATA", READ_ACCESS);
      if (file)
      {
	 fclose (file);
	 fiasco_free (this->basis_name);
	 this->basis_name = strdup (filename);
	 return 1;
      }
      else
      {
	 set_error (_("Can't read basis file `%s'.\n%s."), filename,
		    get_system_error ());
	 return 0;
      }
   }
}

int
fiasco_c_options_set_chroma_quality (fiasco_c_options_t *options,
				     float quality_factor,
				     unsigned dictionary_size)
/*
 *  Set color compression parameters.
 *  When coding chroma channels (Cb and Cr)
 *  - approximation quality is given by `quality_factor' * `Y quality' and
 *  - `dictionary_size' gives the number of dictionary elements.
 *  
 *  If 'quality' <= 0 then the luminancy coding quality is also during
 *  chroma channel coding.
 *  
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   else if (!dictionary_size)
   {
      set_error (_("Size of chroma compression dictionary has to be "
		   "a positive number."));
      return 0;
   }
   else if (quality_factor <= 0)
   {
      set_error (_("Quality of chroma channel compression has to be "
		   "positive value."));
      return 0;
   }
   else
   {
      this->chroma_decrease   = quality_factor;
      this->chroma_max_states = dictionary_size;

      return 1;
   }
}

int
fiasco_c_options_set_optimizations (fiasco_c_options_t *options,
				    unsigned min_block_level,
				    unsigned max_block_level,
				    unsigned max_elements,
				    unsigned dictionary_size,
				    unsigned optimization_level)
/*
 *  Set various optimization parameters.
 *  - During compression only image blocks of size
 *    {`min_block_level', ... ,`max_block_level'} are considered.
 *    The smaller this set of blocks is the faster the coder runs
 *    and the worse the image quality will be.  
 *  - An individual approximation may use at most `max_elements'
 *    elements of the dictionary which itself contains at most
 *    `dictionary_size' elements. The smaller these values are
 *    the faster the coder runs and the worse the image quality will be. 
 *  - `optimization_level' enables some additional low level optimizations.
 *    0: standard approximation method
 *    1: significantly increases the approximation quality,
 *       running time is twice as high as with the standard method
 *    2: hardly increases the approximation quality of method 1, 
 *       running time is twice as high as with method 1
 *       (this method just remains for completeness)
 *
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   else if (!dictionary_size)
   {
      set_error (_("Size of dictionary has to be a positive number."));
      return 0;
   }
   else if (!max_elements)
   {
      set_error (_("At least one dictionary element has to be used "
		   "in an approximation."));
      return 0;
   }
   else if (max_block_level < 4)
   {
      set_error (_("Maximum image block size has to be at least level 4."));
      return 0;
   }
   else if (min_block_level < 4)
   {
      set_error (_("Minimum image block size has to be at least level 4."));
      return 0;
   }
   else if (max_block_level < min_block_level)
   {
      set_error (_("Maximum block size has to be larger or "
		   "equal minimum block size."));
      return 0;
   }
   else
   {
      this->lc_min_level 	= min_block_level;
      this->lc_max_level 	= max_block_level;
      this->max_states 	 	= dictionary_size;
      this->max_elements 	= max_elements;
      this->second_domain_block = optimization_level > 0 ? YES : NO;
      this->check_for_overflow  = optimization_level > 1 ? YES : NO;
      this->check_for_underflow = optimization_level > 1 ? YES : NO;
      this->full_search 	= optimization_level > 1 ? YES : NO;

      return 1;
   }
}

int
fiasco_c_options_set_prediction (fiasco_c_options_t *options,
				 int intra_prediction,
				 unsigned min_block_level,
				 unsigned max_block_level)
/*
 *  Set minimum and maximum size of image block prediction to
 *  `min_block_level' and `max_block_level'.
 *  (For either motion compensated prediction of inter frames
 *   or DC based prediction of intra frames)
 *  Prediction of intra frames is only used if `intra_prediction' != 0.
 *
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   else if (max_block_level < 6)
   {
      set_error (_("Maximum prediction block size has to be "
		   "at least level 6"));
      return 0;
   }
   else if (min_block_level < 6)
   {
      set_error (_("Minimum prediction block size has to be "
		   "at least level 6"));
      return 0;
   }
   else if (max_block_level < min_block_level)
   {
      set_error (_("Maximum prediction block size has to be larger or "
		   "equal minimum block size."));
      return 0;
   }
   else
   {
      this->p_min_level = min_block_level;
      this->p_max_level = max_block_level;
      this->prediction  = intra_prediction;
      
      return 1;
   }
}

int
fiasco_c_options_set_video_param (fiasco_c_options_t *options,
				  unsigned frames_per_second,
				  int half_pixel_prediction,
				  int cross_B_search,
				  int B_as_past_ref)
/*
 *  Set various parameters used for video compensation.
 *  'frames_per_second' defines the frame rate which should be
 *  used when the video is decoded. This value has no effect during coding,
 *  it is just passed to the FIASCO output file.
 *  If 'half_pixel_prediction' is not 0 then half pixel precise
 *  motion compensated prediction is used.
 *  If 'cross_B_search' is not 0 then the fast Cross-B-Search algorithm is
 *  used to determine the motion vectors of interpolated prediction. Otherwise
 *  exhaustive search (in the given search range) is used.
 *  If 'B_as_past_ref' is not 0 then B frames are allowed to be used
 *  for B frame predicion.
 *  
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   else
   {
      this->fps 	  	  = frames_per_second;
      this->half_pixel_prediction = half_pixel_prediction;
      this->cross_B_search 	  = cross_B_search;
      this->B_as_past_ref 	  = B_as_past_ref;

      return 1;
   }
}

int
fiasco_c_options_set_quantization (fiasco_c_options_t *options,
				   unsigned mantissa,
				   fiasco_rpf_range_e range,
				   unsigned dc_mantissa,
				   fiasco_rpf_range_e dc_range)
/*
 *  Set accuracy of coefficients quantization.
 *  DC coefficients (of the constant dictionary vector f(x,y) = 1)
 *  are quantized to values of the interval [-`dc_range', `dc_range'] using
 *  #`dc_mantissa' bits. All other quantized coefficients are quantized in
 *  an analogous way using the parameters `range' and `mantissa'.
 *  
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   else if (mantissa < 2 || mantissa > 8 || dc_mantissa < 2 || dc_mantissa > 8)
   {
      set_error (_("Number of RPF mantissa bits `%d', `%d' have to be in "
		   "the interval [2,8]."), mantissa, dc_mantissa);
      return 0;
   }
   else
   {
      if ((range == FIASCO_RPF_RANGE_0_75
	  || range == FIASCO_RPF_RANGE_1_00
	  || range == FIASCO_RPF_RANGE_1_50
	   || range == FIASCO_RPF_RANGE_2_00)
	  &&
	  (dc_range == FIASCO_RPF_RANGE_0_75
	   || dc_range == FIASCO_RPF_RANGE_1_00
	   || dc_range == FIASCO_RPF_RANGE_1_50
	   || dc_range == FIASCO_RPF_RANGE_2_00))
      {
	 this->rpf_range       = range;
	 this->dc_rpf_range    = dc_range;
	 this->rpf_mantissa    = mantissa;
	 this->dc_rpf_mantissa = dc_mantissa;

	 return 1;
      }
      else
      {
	 set_error (_("Invalid RPF ranges `%d', `%d' specified."),
		    range, dc_range);
	 return 0;
      }
   }
}

int
fiasco_c_options_set_progress_meter (fiasco_c_options_t *options,
				     fiasco_progress_e type)
/*
 *  Set type of progress meter.
 *
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   switch (type)
   {
      case FIASCO_PROGRESS_BAR:
      case FIASCO_PROGRESS_PERCENT:
      case FIASCO_PROGRESS_NONE:
	 this->progress_meter = type;
	 break;
      default:
	 set_error (_("Invalid progress meter `%d' specified "
		      "(valid values are 0, 1, or 2)."), type);
	 return 0;
   }
   return 1;
}

int
fiasco_c_options_set_smoothing (fiasco_c_options_t *options, int smoothing)
/*
 *  Define `smoothing'-percentage along partitioning borders.
 *  
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   else if (smoothing < -1 || smoothing > 100)
   {
      set_error (_("Smoothing percentage must be in the range [-1, 100]."));
      return 0;
   }
   else
   {
      this->smoothing = smoothing;
      return 1;
   }
}

int
fiasco_c_options_set_comment (fiasco_c_options_t *options, const char *comment)
/*
 *  Define `comment' of FIASCO stream.
 *  
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   else if (!comment)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "title");
      return 0;
   }
   else
   {
      this->comment = strdup (comment);
      return 1;
   }
}

int
fiasco_c_options_set_title (fiasco_c_options_t *options, const char *title)
/*
 *  Define `title' of FIASCO stream.
 *  
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   c_options_t *this = (c_options_t *) cast_c_options (options);

   if (!this)
   {
      return 0;
   }
   else if (!title)
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "title");
      return 0;
   }
   else
   {
      this->title = strdup (title);
      return 1;
   }
}

c_options_t *
cast_c_options (fiasco_c_options_t *options)
/*
 *  Cast generic pointer `options' to type c_options_t.
 *  Check whether `options' is a valid object of type c_options_t.
 *
 *  Return value:
 *	pointer to options struct on success
 *      NULL otherwise
 */
{
   c_options_t *this = (c_options_t *) options->private;
   if (this)
   {
      if (!streq (this->id, "COFIASCO"))
      {
	 set_error (_("Parameter `options' doesn't match required type."));
	 return NULL;
      }
   }
   else
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "options");
   }

   return this;
}

/**************************************************************************
***************************************************************************
			       DECODER
***************************************************************************
**************************************************************************/

fiasco_d_options_t *
fiasco_d_options_new (void)
/*
 *  FIASCO options constructor.
 *  Allocate memory for the FIASCO coder options structure and
 *  fill in default values.
 *
 *  Return value:
 *	pointer to the new option structure
 */
{
   d_options_t 	      *options = fiasco_calloc (1, sizeof (d_options_t));
   fiasco_d_options_t *public  = fiasco_calloc (1, sizeof (fiasco_d_options_t));

   public->private 	      = options;
   public->delete 	      = fiasco_d_options_delete;
   public->set_smoothing      = fiasco_d_options_set_smoothing;
   public->set_magnification  = fiasco_d_options_set_magnification;
   public->set_4_2_0_format   = fiasco_d_options_set_4_2_0_format;
   
   strcpy (options->id, "DOFIASCO");

   /*
    *  Set default value of fiasco decoder options
    */
   options->smoothing 	  = 70;
   options->magnification = 0;
   options->image_format  = FORMAT_4_4_4;
   
   return public;
}

void
fiasco_d_options_delete (fiasco_d_options_t *options)
/*
 *  FIASCO options destructor.
 *  Free memory of FIASCO options struct.
 *
 *  No return value.
 *
 *  Side effects:
 *	structure 'options' is discarded.
 */
{
   d_options_t *this = cast_d_options (options);

   if (!this)
      return;
   
   fiasco_free (this);

   return;
}

int
fiasco_d_options_set_smoothing (fiasco_d_options_t *options, int smoothing)
/*
 *  Define `smoothing'-percentage along partitioning borders.
 *  
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   d_options_t *this = (d_options_t *) cast_d_options (options);

   if (!this)
   {
      return 0;
   }
   else if (smoothing < -1 || smoothing > 100)
   {
      set_error (_("Smoothing percentage must be in the range [-1, 100]."));
      return 0;
   }
   else
   {
      this->smoothing = smoothing;
      return 1;
   }
}

int
fiasco_d_options_set_magnification (fiasco_d_options_t *options, int level)
/*
 *  Set magnification-'level' of decoded image.
 *  0: width x height of original image
 *  1: (2 * width) x (2 * height) of original image
 *  -1: (width / 2 ) x (height / 2) of original image
 *  etc.
 *
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   d_options_t *this = (d_options_t *) cast_d_options (options);

   if (!this)
   {
      return 0;
   }
   else
   {
      this->magnification = level;
      return 1;
   }
}

int
fiasco_d_options_set_4_2_0_format (fiasco_d_options_t *options, int format)
/*
 *  Set image format to 4:2:0 or 4:4:4.
 *  
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   d_options_t *this = (d_options_t *) cast_d_options (options);

   if (!this)
   {
      return 0;
   }
   else
   {
      this->image_format = format ? FORMAT_4_2_0 : FORMAT_4_4_4;
      return 1;
   }
}

d_options_t *
cast_d_options (fiasco_d_options_t *options)
/*
 *  Cast generic pointer `options' to type d_options_t.
 *  Check whether `options' is a valid object of type d_options_t.
 *
 *  Return value:
 *	pointer to options struct on success
 *      NULL otherwise
 */
{
   d_options_t *this = (d_options_t *) options->private;
   
   if (this)
   {
      if (!streq (this->id, "DOFIASCO"))
      {
	 set_error (_("Parameter `options' doesn't match required type."));
	 return NULL;
      }
   }
   else
   {
      set_error (_("Parameter `%s' not defined (NULL)."), "options");
   }

   return this;
}


