/*
 *  coder.c:		WFA coder toplevel functions
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/10/28 17:39:30 $
 *  $Author: hafner $
 *  $Revision: 5.6 $
 *  $State: Exp $
 */

#include "config.h"

#include <math.h>
#include <ctype.h>

#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */

#include "types.h"
#include "macros.h"
#include "error.h"

#include "fiasco.h"

#include "cwfa.h"
#include "misc.h"
#include "control.h"
#include "bintree.h"
#include "subdivide.h"
#include "read.h"
#include "write.h"
#include "image.h"
#include "mwfa.h"
#include "list.h"
#include "decoder.h"
#include "motion.h"
#include "wfalib.h"
#include "domain-pool.h"
#include "coeff.h"
#include "coder.h"
#include "rpf.h"

/*****************************************************************************

				global variables
  
*****************************************************************************/

const real_t MAXCOSTS =	1e20;

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static coding_t *
alloc_coder (char const * const *inputname, const c_options_t *options,
	     wfa_info_t *wi);
static void
free_coder (coding_t *c);
static char *
get_input_image_name (char const * const *templptr, unsigned ith_image);
static void
video_coder (char const * const *image_template, bitfile_t *output,
	     wfa_t *wfa, coding_t *c);
static void 
frame_coder (wfa_t *wfa, coding_t *c, bitfile_t *output);
static void
print_statistics (char c, real_t costs, const wfa_t *wfa, const image_t *image,
		  const range_t *range);
static frame_type_e
pattern2type (unsigned frame, const char *pattern);

/*****************************************************************************

				public code
  
*****************************************************************************/

int
fiasco_coder (char const * const *inputname, const char *outputname,
	      float quality, const fiasco_c_options_t *options)
/*
 *  FIASCO coder.
 *  Encode image or video frames given by the array of filenames `inputname'
 *  and write to the outputfile `outputname'.
 *  If 'inputname' = NULL or
 *     'inputname [0]' == NULL or
 *     'inputname [0]' == "-", read standard input.
 *  If 'outputname' == NULL or "-", write on standard output.
 *  'quality' defines the approximation quality and is 1 (worst) to 100 (best).
 *
 *  Return value:
 *	1 on success
 *	0 otherwise
 */
{
   try
   {
      char const * const default_input [] = {"-", NULL};
      fiasco_c_options_t *default_options = NULL;
      const c_options_t  *cop;
      char const * const *template;
      
      /*
       *  Check parameters
       */
      if (!inputname || !inputname [0] || streq (inputname [0], "-"))
	 template = default_input;
      else
	 template = inputname;
      
      if (quality <= 0)
      {
	 set_error (_("Compression quality has to be positive."));
	 return 0;
      }
      else if (quality >= 100)
      {
	 warning (_("Quality typically is 1 (worst) to 100 (best).\n"
		    "Be prepared for a long running time."));
      }

      if (options)
      {
	 cop = cast_c_options ((fiasco_c_options_t *) options);
	 if (!cop)
	    return 0;
      }
      else
      {
	 default_options = fiasco_c_options_new ();
	 cop 		 = cast_c_options (default_options);
      }

   /*
    *  Open output stream and initialize WFA
    */
      {
	 bitfile_t *output = open_bitfile (outputname, "FIASCO_DATA",
					   WRITE_ACCESS);
	 if (!output)
	 {
	    set_error (_("Can't write outputfile `%s'.\n%s"),
		       outputname ? outputname : "<stdout>",
		       get_system_error ());
	    if (default_options)
	       fiasco_c_options_delete (default_options);
	    return 0;
	 }
	 else
	 {
	    wfa_t    *wfa = alloc_wfa (YES);
	    coding_t *c   = alloc_coder (template, cop, wfa->wfainfo);
	 
	    read_basis (cop->basis_name, wfa);
	    append_basis_states (wfa->basis_states, wfa, c);
	 
	    c->price = 128 * 64 / quality;
	 
	    video_coder (template, output, wfa, c);
	 
	    close_bitfile (output);
	    free_wfa (wfa);
	    free_coder (c);
	 
	    if (default_options)
	       fiasco_c_options_delete (default_options);
	 }
      }
      return 1;
   }
   catch
   {
      return 0;
   }
}

/*****************************************************************************

				private code
  
*****************************************************************************/

static coding_t *
alloc_coder (char const * const *inputname, const c_options_t *options,
	     wfa_info_t *wi)
/*
 *  Coder structure constructor.
 *  Allocate memory for the FIASCO coder structure and
 *  fill in default values specified by 'options'.
 *
 *  Return value:
 *	pointer to the new coder structure or NULL on error
 */
{
   coding_t *c = NULL;
   
   /*
    *  Check whether all specified image frames are readable and of same type
    */
   {
      char     *filename;
      unsigned 	width, w = 0, height, h = 0;
      bool_t	color, c = NO;
      unsigned 	n;
      
      for (n = 0; (filename = get_input_image_name (inputname, n)); n++)
      {
	 FILE *file = read_pnmheader (filename, &width, &height, &color);
	 if (!file)
	 {
	    set_error (_("Can't open frame `%s'.\n%s"),
		       filename ? filename : "<stdout>", get_system_error ());
	    return NULL;
	 }
	 fclose (file);
	 if (n)
	 {
	    if (w != width || h != height)
	       error (_("`%s': all images of a sequence have to be "
			"of the same size."), filename ? filename : "<stdin>");
	    if (c != color)
	       error (_("`%s': all images a sequence have to use the same "
			"color model."), filename ? filename : "<stdin>");
	 }
	 else
	 {
	    w = width;
	    h = height;
	    c = color;
	 }
	 Free (filename);
      }
      wi->frames = n;
      wi->width  = w;
      wi->height = h;
      wi->color  = c;
   }

   /*
    *  Levels ...
    */
   {
      unsigned lx, ly;
      
      lx = (unsigned) (log2 (wi->width - 1) + 1);
      ly = (unsigned) (log2 (wi->height - 1) + 1);
      
      wi->level = max (lx, ly) * 2 - ((ly == lx + 1) ? 1 : 0);
   }
   
   c = Calloc (1, sizeof (coding_t));

   c->options 	      	   = *options;
   c->options.lc_min_level = max (options->lc_min_level, 3);
   c->options.lc_max_level = min (options->lc_max_level, wi->level - 1);

   c->tiling = alloc_tiling (options->tiling_method,
			     options->tiling_exponent, wi->level);

   if (wi->frames > 1 && c->tiling->exponent > 0)
   {
      c->tiling->exponent = 0;
      warning (_("Image tiling valid only with still image compression."));
   }

   if (c->options.lc_max_level >= wi->level - c->tiling->exponent)
   {
      message ("'max_level' changed from %d to %d due to image tiling level.",
	       c->options.lc_max_level, wi->level - c->tiling->exponent - 1);
      c->options.lc_max_level = wi->level - c->tiling->exponent - 1;
   }
   
   if (c->options.lc_min_level > c->options.lc_max_level)
      c->options.lc_min_level = c->options.lc_max_level;
   
   /*
    *  p_min_level, p_max_level min and max level for ND/MC prediction
    *  [p_min_level, p_max_level] must be a subset of [min_level, max_level] !
    */
   wi->p_min_level = max (options->p_min_level, c->options.lc_min_level);
   wi->p_max_level = min (options->p_max_level, c->options.lc_max_level);
   if (wi->p_min_level > wi->p_max_level)
      wi->p_min_level = wi->p_max_level;

   c->options.images_level = min (c->options.images_level,
				  c->options.lc_max_level - 1);
   
   c->products_level  = max (0, (c->options.lc_max_level
				 - c->options.images_level - 1));
   c->pixels 	      = Calloc (size_of_level (c->options.lc_max_level),
				sizeof (real_t));
   c->images_of_state = Calloc (MAXSTATES, sizeof (real_t *));
   c->ip_images_state = Calloc (MAXSTATES, sizeof (real_t *));
   c->ip_states_state = Calloc (MAXSTATES * MAXLEVEL, sizeof (real_t *));
   
   debug_message ("Imageslevel :%d, Productslevel :%d",
		  c->options.images_level, c->products_level);
   debug_message ("Memory : (%d + %d + %d * 'states') * 'states' + %d",
		  size_of_tree (c->options.images_level) * 4,
		  size_of_tree (c->products_level) * 4,
		  (c->options.lc_max_level - c->options.images_level),
		  size_of_level (c->options.lc_max_level));
   
   /*
    *  Domain pools ...
    */
   c->domain_pool   = NULL;
   c->d_domain_pool = NULL;

   /*
    *  Coefficients model ...
    */
   c->coeff   = NULL;
   c->d_coeff = NULL;

   /*
    *  Max. number of states and edges
    */
   wi->max_states   	   = max (min (options->max_states, MAXSTATES), 1);
   c->options.max_elements = max (min (options->max_elements, MAXEDGES), 1);

   /*
    *  Title and comment strings
    */
   wi->title   = strdup (options->title);
   wi->comment = strdup (options->comment);
   
   /*
    *  Reduced precision format
    */
   wi->rpf
      = alloc_rpf (options->rpf_mantissa, options->rpf_range);
   wi->dc_rpf
      = alloc_rpf (options->dc_rpf_mantissa, options->dc_rpf_range);
   wi->d_rpf
      = alloc_rpf (options->d_rpf_mantissa, options->d_rpf_range);
   wi->d_dc_rpf
      = alloc_rpf (options->d_dc_rpf_mantissa, options->d_dc_rpf_range);
   
   /*
    *  Color image options ...
    */
   wi->chroma_max_states = max (1, options->chroma_max_states);

   /*
    *  Set up motion compensation struct.
    *  p_min_level, p_max_level are also used for ND prediction
    */
   wi->search_range   = options->search_range;
   wi->fps 	      = options->fps;
   wi->half_pixel     = options->half_pixel_prediction;
   wi->cross_B_search = options->half_pixel_prediction;
   wi->B_as_past_ref  = options->B_as_past_ref;
   wi->smoothing      = options->smoothing;
   
   c->mt = alloc_motion (wi);

   return c;
}

static void
free_coder (coding_t *c)
/*
 *  Coder struct destructor:
 *  Free memory of 'coder' struct.
 *
 *  No return value.
 *
 *  Side effects:
 *	structure 'coder' is discarded.
 */
{
   free_tiling (c->tiling);
   free_motion (c->mt);
   
   Free (c->pixels);
   Free (c->images_of_state);
   Free (c->ip_images_state);
   Free (c->ip_states_state);
   Free (c);
}

static char *
get_input_image_name (char const * const *templptr, unsigned ith_image)
/*
 *  Construct the i-th image-name using templates.
 *  If the template contains a '[' it must be of the form
 *  "prefix[start-end{+,-}step]suffix"
 *  where "{+,-}step" is optional.
 *  Leading zeros of "start" are significant.
 *
 *  Example:
 *   "image0[12-01-1].pgm" yields image012.pgm, image011.pgm, ..., image001.pgm
 *
 *  Return value:
 *      ptr to name of image 'ith_image' or NULL if ith_image is out of range.
 */
{
   while (*templptr)
   {
      const char *template = *templptr++;
      char       *s;

      if (!(s = strchr (template, '['))) /* no template, just a filename */
      {
	 if (ith_image == 0)
	    return strdup (template);
	 else
	    ith_image--;
      }
      else				/* template parser */
      {
	 unsigned  n_digits;		/* # of digits in image name no. */
	 char 	  *s2;
	 char 	  *suffix;		/* characters after template end */
	 char  	   prefix [MAXSTRLEN];	/* chars up to the template start */
	 unsigned  first;		/* first image number */
	 unsigned  last;		/* last image number */
	 int  	   image_num;		/* current image number */
	 int   	   increment = 1;
	 int 	   dummy;

	 strcpy (prefix, template);
	 prefix [s - template] = '\0';
   
	 for (s2 = ++s, n_digits = 0; isdigit (*s2); s2++, n_digits++)
	    ;
	 if (sscanf (s, "%d", &dummy) == 0 || dummy < 0)
	    error ("Input name template conversion failure.\n"
		   "Check spelling of template.");
	 first = (unsigned) dummy;
	 
	 if (*s2++ != '-')
	    error ("Input name template conversion failure.\n"
		   "Check spelling of template.");
   
	 for (s = s2; isdigit (*s2); s2++)
	    ;
	 if (sscanf (s, "%d", &dummy) == 0 || dummy < 0)
	    error ("Input name template conversion failure.\n"
		   "Check spelling of template.");
	 last = (unsigned) dummy;
	 
	 if (*s2 == '+' || *s2 == '-') 
	 {
	    for (s = s2++; isdigit (*s2); s2++)
	       ;
	    if (sscanf (s, "%d", &increment) == 0)
	       error ("Input name template conversion failure.\n"
		      "Check spelling of template.");
	 } 	 
	 if (*s2 != ']')
	    error ("Input name template conversion failure.\n"
		   "Check spelling of template.");
	 suffix = s2 + 1;
   
	 image_num = first + increment * ith_image;
	 if (image_num < 0)
	    error ("Input name template conversion failure.\n"
		   "Check spelling of template.");
	 
	 if ((increment >  0 && (unsigned) image_num > last) || 
	     (increment <= 0 && (unsigned) image_num < last))
	 {
	    /* TODO: check this */
	    ith_image -= (last - first) / increment + 1;
	 }
	 else
	 {
	    char formatstr [MAXSTRLEN]; /* format string for image filename */
	    char image_name [MAXSTRLEN]; /* image file name to be composed */
	    
	    strcpy (formatstr, "%s%0?d%s");
	    formatstr [4] = '0' + (char) n_digits;
	    sprintf (image_name, formatstr, prefix, image_num, suffix);
	    return strdup (image_name);
	 }
      }
   }
   return NULL;
}	

static void
video_coder (char const * const *image_template, bitfile_t *output,
	     wfa_t *wfa, coding_t *c)
/*
 *  Toplevel function to encode a sequence of video frames specified
 *  by 'image_template'. The output is written to stream 'output'.
 *  Coding options are given by 'c'.
 *
 *  No return value.
 */
{
   unsigned  display;			/* picture number in display order */
   int	     future_display;		/* number of future reference */
   int	     frame;			/* current frame number */
   char	    *image_name;		/* image name of current frame */
   image_t  *reconst 	  = NULL; 	/* decoded reference image */
   bool_t    future_frame = NO;		/* YES if last frame was in future */
   
   debug_message ("Generating %d WFA's ...", wfa->wfainfo->frames);

   future_display = -1;
   frame          = -1;
   display        = 0;

   while ((image_name = get_input_image_name (image_template, display)))
   {
      frame_type_e type;		/* current frame type: I, B, P */
      
      /*
       *  Determine type of next frame.
       *  Skip already coded frames (future reference!)
       */
      if (display == 0 && !c->options.reference_filename)
	 type = I_FRAME;		/* Force first frame to be intra */
      else
	 type = pattern2type (display, c->options.pattern);
      
      if (type != I_FRAME && c->options.reference_filename)
	 /* Load reference from disk */
      {
	 debug_message ("Reading reference frame `%s'.",
			c->options.reference_filename);
	 reconst 	 = read_image (c->options.reference_filename);
	 c->options.reference_filename = NULL;
      }
      if ((int) display == future_display) /* Skip already coded future ref */
      {				
	 display++;
	 continue;
      }	  
      else if (type == B_FRAME && (int) display > future_display) 
      {
	 unsigned i = display;
	 /*
	  *  Search for future reference
	  */
	 while (type == B_FRAME)
	 {
	    char *name;			/* image name of future frame */

	    i++;
	    name = get_input_image_name (image_template, i);
	    
	    if (!name)			/* Force last valid frame to be 'P' */
	    {
	       future_display = i - 1;
	       type = P_FRAME;
	    }
	    else
	    {
	       future_display = i;    
	       image_name     = name;
	       type 	      = pattern2type (i, c->options.pattern);
	    }
	    frame = future_display;
	 }
      }
      else
      {
	 frame = display;
	 display++;
      }

      debug_message ("Coding \'%s\' [%c-frame].", image_name,
		     type == I_FRAME ? 'I' : (type == P_FRAME ? 'P' : 'B'));
       
      /*
       *  Depending on current frame type update past and future frames
       *  which are needed as reference frames.
       */
      c->mt->frame_type = type;
      if (type == I_FRAME)
      {
	 if (c->mt->past)		/* discard past frame */
	    free_image (c->mt->past);
	 c->mt->past = NULL;
	 if (c->mt->future)		/* discard future frame */
	    free_image (c->mt->future);
	 c->mt->future = NULL;
	 if (reconst)			/* discard current frame */
	    free_image (reconst);
	 reconst = NULL;
      }
      else if (type == P_FRAME)
      {
	 if (c->mt->past)		/* discard past frame */
	    free_image (c->mt->past);
	 c->mt->past = reconst;		/* past frame <- current frame */
	 reconst    = NULL;
	 if (c->mt->future)		/* discard future frame */
	    free_image (c->mt->future);
	 c->mt->future = NULL;
      }
      else				/* B_FRAME */
      {
	 if (future_frame)		/* last frame was future frame */
	 {
	    if (c->mt->future)		/* discard future frame */
	       free_image (c->mt->future);
	    c->mt->future = reconst;	/* future frame <- current frame */
	    reconst      = NULL;
	 }
	 else
	 {
	    if (wfa->wfainfo->B_as_past_ref == YES)
	    {
	       if (c->mt->past)		/* discard past frame */
		  free_image (c->mt->past);
	       c->mt->past = reconst;	/* past frame <- current frame */
	       reconst    = NULL;
	    }
	    else
	    {
	       if (reconst)		/* discard current frame */
		  free_image (reconst);
	       reconst = NULL;
	    }
	 }
      }

      /*
       *  Start WFA coding of current frame
       */
      future_frame   = frame == future_display;
      c->mt->number   = frame;
      c->mt->original = read_image (image_name);
      if (c->tiling->exponent && type == I_FRAME) 
	 perform_tiling (c->mt->original, c->tiling);

      frame_coder (wfa, c, output);

      /*
       *  Regenerate image:
       *  1. Compute approximation of WFA ranges (real image bintree order)
       *  2. Generate byte image in rasterscan order
       *  3. Apply motion compensation
       */
      reconst = decode_image (wfa->wfainfo->width, wfa->wfainfo->height,
			      FORMAT_4_4_4, NULL, wfa);

      if (type != I_FRAME)
	 restore_mc (0, reconst, c->mt->past, c->mt->future, wfa);

      if (c->mt->original)
	 free_image (c->mt->original);
      c->mt->original = NULL;
      
      remove_states (wfa->basis_states, wfa); /* Clear WFA structure */
   }

   if (reconst)
      free_image (reconst);
   if (c->mt->future)
      free_image (c->mt->future);
   if (c->mt->past)
      free_image (c->mt->past);
   if (c->mt->original)
      free_image (c->mt->original);
}

static frame_type_e
pattern2type (unsigned frame, const char *pattern)
{
   int tmp = toupper (pattern [frame % strlen (pattern)]);

   switch (tmp)
   {
      case 'I':
	 return I_FRAME;
	 break;
      case 'B':
	 return B_FRAME;
	 break;
      case 'P':
	 return P_FRAME;
	 break;
      default:
	 error ("Frame type %c not valid. Choose one of I,B or P.", tmp);
   }
   return NONE;
}

static void 
frame_coder (wfa_t *wfa, coding_t *c, bitfile_t *output)
/*
 * 
 *  WFA Coding of next frame.  All important coding parameters are
 *  stored in 'c'.  The generated 'wfa' is written to stream 'output'
 *  immediately after coding.
 *
 *  No return value.
 */
{
   unsigned state;
   range_t  range;			/* first range == the entire image */
   real_t   costs;			/* total costs (minimized quantity) */
   unsigned bits;			/* number of bits written on disk */
   clock_t  ptimer;
   
   prg_timer (&ptimer, START);

   bits = bits_processed (output);
   
   init_tree_model (&c->tree);
   init_tree_model (&c->p_tree);

   c->domain_pool
      = alloc_domain_pool (c->options.id_domain_pool,
			   wfa->wfainfo->max_states,
			   c->options.max_elements, wfa);
   c->d_domain_pool
      = alloc_domain_pool ((c->options.prediction
			    || c->mt->frame_type != I_FRAME)
			   ? c->options.id_d_domain_pool : "constant",
			   wfa->wfainfo->max_states,
			   c->options.max_elements, wfa);

   c->coeff   = alloc_coeff_model (c->options.id_rpf_model,
				   wfa->wfainfo->rpf,
				   wfa->wfainfo->dc_rpf,
				   c->options.lc_min_level,
				   c->options.lc_max_level);
   c->d_coeff = alloc_coeff_model (c->options.id_d_rpf_model,
				   wfa->wfainfo->d_rpf,
				   wfa->wfainfo->d_dc_rpf,
				   c->options.lc_min_level,
				   c->options.lc_max_level);

   if (!c->mt->original->color)		/* grayscale image */
   {
      memset (&range, 0, sizeof (range_t));
      range.level = wfa->wfainfo->level;

      costs = subdivide (MAXCOSTS, GRAY, RANGE, &range, wfa, c,
			 c->options.prediction || c->mt->frame_type != I_FRAME,
			 NO);
      if (c->options.progress_meter != FIASCO_PROGRESS_NONE)
	 message ("");

      if (isrange (range.tree))		/* entire image is approx. by lc? */
	 error ("No root state generated!");
      else
	 wfa->root_state = range.tree;

      print_statistics ('\0', costs, wfa, c->mt->original, &range);
   }
   else
   {
      int     YCb_node = -1;
      int     tree [3];			/* 3 root states of each color comp. */
      color_e band;
      
      /*
       *  When compressing color images, the three color components (YCbCr) 
       *  are copied into a large image:
       *  [  Y  Cr ]
       *  [  Cb 0  ]
       *  I.e. the color components of an image are processed in a row.
       *  After all components are compressed, virtual states are generated
       *  to describe the large image.
       */
      for (band = first_band (YES); band <= last_band (YES) ; band++)
      {
	 debug_message ("Encoding color component %d", band);
	 tree [band] = RANGE;
	 if (band == Cb)
	 {
	    unsigned min_level;

	    c->domain_pool->chroma (wfa->wfainfo->chroma_max_states, wfa,
				    c->domain_pool->model);
	    /*
	     *  Don't use a finer partioning for the chrominancy bands than for
	     *  the luminancy band.
	     */
	    for (min_level = MAXLEVEL, state = wfa->basis_states;
		 state < wfa->states; state++)
	    {
	       unsigned lincomb, label;
	       
	       for (lincomb = 0, label = 0; label < MAXLABELS; label++)
		  lincomb += isrange (wfa->tree [state][label]) ? 1 : 0;
	       if (lincomb)
		  min_level = min (min_level,
				   (unsigned) (wfa->level_of_state [state]
					       - 1));
	    }
	    c->options.lc_min_level = min_level;
	    if (c->mt->frame_type != I_FRAME) /* subtract mc of luminance */
	       subtract_mc (c->mt->original, c->mt->past, c->mt->future, wfa);
	 }

	 memset (&range, 0, sizeof (range_t));
	 range.level = wfa->wfainfo->level;
	 
	 costs = subdivide (MAXCOSTS, band, tree [Y], &range, wfa, c,
			    c->mt->frame_type != I_FRAME && band == Y, NO);
	 if (c->options.progress_meter != FIASCO_PROGRESS_NONE)
	    message ("");
	 {
	    char colors [] = {'Y', 'B', 'R'};
	    
	    print_statistics (colors [band], costs, wfa,
			      c->mt->original, &range);
	 }
	 
	 if (isrange (range.tree))	/* whole image is approx. by a l.c. */
	    error ("No root state generated for color component %d!", band);
	 else
	    tree[band] = range.tree;
	 
	 if (band == Cb)
	 {
	    wfa->tree [wfa->states][0] = tree[Y];
	    wfa->tree [wfa->states][1] = tree[Cb];
	    YCb_node = wfa->states;
	    append_state (YES, compute_final_distribution (wfa->states, wfa),
			  wfa->wfainfo->level + 1, wfa, c);
	 }
      }
      /*
       *  generate two virtual states (*) 
       *
       *              *
       *            /   \
       *           +     *
       *          / \   /  
       *         Y   CbCr 
       */
      wfa->tree [wfa->states][0] = tree[Cr];
      wfa->tree [wfa->states][1] = RANGE;
      append_state (YES, compute_final_distribution (wfa->states, wfa),
		    wfa->wfainfo->level + 1, wfa, c);
      wfa->tree[wfa->states][0] = YCb_node;
      wfa->tree[wfa->states][1] = wfa->states - 1;
      append_state (YES, compute_final_distribution (wfa->states, wfa),
		    wfa->wfainfo->level + 2, wfa, c);

      wfa->root_state = wfa->states - 1;
   }

   for (state = wfa->basis_states; state < MAXSTATES; state++)
   {
      unsigned level;
      
      if (c->images_of_state [state])
      {
	 Free (c->images_of_state [state]);
	 c->images_of_state [state] = NULL;
      }
      if (c->ip_images_state [state])
      {
	 Free (c->ip_images_state [state]);
	 c->ip_images_state [state] = NULL;
      }
      for (level = c->options.images_level + 1;
	   level <= c->options.lc_max_level;
	   level++)
	 if (c->ip_states_state [state][level])
	 {
	    Free (c->ip_states_state [state][level]);
	    c->ip_states_state [state][level] = NULL;
	 }
      
   }
   
   locate_delta_images (wfa);
   write_next_wfa (wfa, c, output);
   
   bits = bits_processed (output) - bits;
   debug_message ("Total number of bits written: %d (%d bytes, %5.3f bpp)",
		  bits, bits >> 3,
		  bits / (double) (c->mt->original->height
				   * c->mt->original->width));
   debug_message ("Total encoding time (real): %d sec",
		  prg_timer (&ptimer, STOP) / 1000);

   c->domain_pool->free (c->domain_pool);
   c->d_domain_pool->free (c->d_domain_pool);

   c->coeff->free (c->coeff);
   c->d_coeff->free (c->d_coeff);
}

static void
print_statistics (char c, real_t costs, const wfa_t *wfa, const image_t *image,
		  const range_t *range)
{
   unsigned max_level, min_level, state, label, lincomb;
   
   for (max_level = 0, min_level = MAXLEVEL, state = wfa->basis_states;
	state < wfa->states; state++)
   {
      for (lincomb = 0, label = 0; label < MAXLABELS; label++)
	 lincomb += isrange(wfa->tree[state][label]) ? 1 : 0;
	 
      if (lincomb)
      {
	 max_level = max (max_level,
			  (unsigned) (wfa->level_of_state [state] - 1));
	 min_level = min (min_level,
			  (unsigned) (wfa->level_of_state [state] - 1));
      }
   }
   debug_message ("Image partitioning: maximum level %d , minimum level %d",
		  max_level, min_level);
   debug_message ("WFA contains %d states (%d basis states).",
		  wfa->states, wfa->basis_states);
   debug_message ("Estimated error: %.2f (RMSE: %.2f, PSNR: %.2f dB).",
		  (double) range->err,
		  sqrt (range->err / image->width / image->height),
		  10 * log ( 255.0 * 255.0 /
			     (range->err / image->width / image->height))
		  / log (10.0));
   debug_message ("Estimated filesize: %.0f bits (%.0f bytes).",
		  (double) (range->tree_bits + range->matrix_bits
			    + range->weights_bits
			    + range->mv_tree_bits + range->mv_coord_bits
			    + range->nd_tree_bits + range->nd_weights_bits),
		  (double) (range->tree_bits + range->matrix_bits
			    + range->weights_bits + range->mv_tree_bits
			    + range->mv_coord_bits + range->nd_tree_bits
			    + range->nd_weights_bits) / 8);
   if (c)
      debug_message ("(%cT: %.0f, %cM: %.0f, %cW: %.0f, %cMC: %.0f, "
		     "%cMV: %.0f, %cNT: %.0f, %cNW: %.0f.)",
		     c, (double) range->tree_bits,
		     c, (double) range->matrix_bits,
		     c, (double) range->weights_bits,
		     c, (double) range->mv_tree_bits,
		     c, (double) range->mv_coord_bits,
		     c, (double) range->nd_tree_bits,
		     c, (double) range->nd_weights_bits);
   else
      debug_message ("(T: %.0f, M: %.0f, W: %.0f, MC: %.0f, MV: %.0f, "
		     "NT: %.0f, NW: %.0f.)",
		     (double) range->tree_bits,
		     (double) range->matrix_bits,
		     (double) range->weights_bits,
		     (double) range->mv_tree_bits,
		     (double) range->mv_coord_bits,
		     (double) range->nd_tree_bits,
		     (double) range->nd_weights_bits);
   debug_message ("Total costs : %.2f", (double) costs);
}
