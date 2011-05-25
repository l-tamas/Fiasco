/*
 *  options.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/10/28 17:39:31 $
 *  $Author: hafner $
 *  $Revision: 5.4 $
 *  $State: Exp $
 */

#ifndef _OPTIONS_H
#define _OPTIONS_H

typedef struct c_options
{
   char      	       id [9];
   char  	      *basis_name;
   unsigned  	       lc_min_level;
   unsigned  	       lc_max_level;
   unsigned  	       p_min_level;
   unsigned  	       p_max_level;
   unsigned  	       images_level;
   unsigned  	       max_states;
   unsigned  	       chroma_max_states;
   unsigned  	       max_elements;
   unsigned  	       tiling_exponent;
   fiasco_tiling_e     tiling_method;
   char        	      *id_domain_pool;
   char        	      *id_d_domain_pool;
   char        	      *id_rpf_model;
   char        	      *id_d_rpf_model;
   unsigned  	       rpf_mantissa;
   real_t    	       rpf_range;
   unsigned  	       dc_rpf_mantissa;
   fiasco_rpf_range_e  dc_rpf_range;
   unsigned  	       d_rpf_mantissa;
   fiasco_rpf_range_e  d_rpf_range;
   unsigned  	       d_dc_rpf_mantissa;
   fiasco_rpf_range_e  d_dc_rpf_range;
   real_t    	       chroma_decrease;
   bool_t    	       prediction;
   bool_t    	       delta_domains;
   bool_t    	       normal_domains;
   unsigned  	       search_range;
   unsigned  	       fps;
   char        	      *pattern;
   char        	      *reference_filename;
   bool_t    	       half_pixel_prediction;
   bool_t    	       cross_B_search;
   bool_t    	       B_as_past_ref;
   bool_t    	       check_for_underflow;
   bool_t    	       check_for_overflow;
   bool_t    	       second_domain_block;
   bool_t    	       full_search;
   fiasco_progress_e   progress_meter;
   char 	      *title;
   char 	      *comment;
   unsigned    	       smoothing;
} c_options_t;

typedef struct d_options
{
   char     id [9];
   unsigned smoothing;
   unsigned magnification;
   format_e image_format;
} d_options_t;

c_options_t *
cast_c_options (fiasco_c_options_t *options);
d_options_t *
cast_d_options (fiasco_d_options_t *options);

#endif /* not _OPTIONS_H */
