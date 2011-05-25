/*      
 *  twfa.c:		Visualization of WFA bintree
 *
 *  Written by:         Martin Grimm
 *
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */
 
/*
 *  $Date: 2000/07/16 17:37:12 $
 *  $Author: hafner $
 *  $Revision: 5.2 $
 *  $State: Exp $
 */

#include "config.h"

#include "types.h"
#include "macros.h"

#if STDC_HEADERS
#	include <stdlib.h>
#endif /* not STDC_HEADERS */

#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */

#include <getopt.h>

#include "wfa.h"
#include "bit-io.h"
#include "misc.h"
#include "decoder.h"
#include "wfalib.h"
#include "read.h"
#include "params.h"

#include "tlist.h"
#include "ttypes.h"
#include "twfa.h"
#include "fig.h"
#include "lctree.h"
#include "error.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static int
subtree_depth (const wfa_t *wfa, int local_root);
static void
lrw_to_lwr (const wfa_t *wfa, int *lrw_to_lwr, int *pos, int local_root);
static void
make_fig (const wfa_t *wfa, toptions_t *options, int frame, int color_image);
static void
browse_mwfa (toptions_t *options);
static int 
checkargs (int argc, char **argv, toptions_t *options);

/*****************************************************************************

				public code
  
*****************************************************************************/

int
main (int argc, char **argv)
{
   init_error_handling (argv[0]);

   try
   {
      toptions_t *options;		
      int	  last_arg;		/* last processed cmdline parameter */

      options = Calloc (1, sizeof (toptions_t));
      
      last_arg = checkargs (argc, argv, options);

      options->wfa_name = last_arg >= argc ? "-" : argv [last_arg];
      browse_mwfa (options);
      
      if (options->LC_tree_list)
	 RemoveList (options->LC_tree_list);
      if (options->LC_basis_list)
	 RemoveList (options->LC_basis_list);
      if (options->frames_list)
	 RemoveList (options->frames_list);
      
      Free (options);
   }
   catch
   {
      fprintf (stderr, "Error: ");
      fprintf (stderr, fiasco_get_error_message ());
      fprintf (stderr, "\n");

      return 1;
   }
   
   return 0;
}

/*****************************************************************************

				private code
  
*****************************************************************************/

param_t params [] =
{
   {"output", "FILE", 'o', PSTR, {0}, "-",
    "Write figure to `%s'."},
   {"root-state", "NUM", 'r', PINT, {0}, "-1",
    "Set root state of subtree to `%s'."},
   {"max-depth", "NUM", 'd', PINT, {0}, "-1",
    "Set maximum depth of subtree to `%s'."},
   {"grid", NULL, 'g', PFLAG, {0}, "NO",
    "Show location of the root state in WFA grid."},
   {"color-grid", NULL, 'G', PFLAG, {0}, "NO",
    "Same as --grid with colored states and grid."},
   {"prune", NULL, 'p', PFLAG, {0}, "NO",
    "Prune tree at the first lc found in each subtree."},
   {"states", NULL, 's', PFLAG, {0}, "NO",
    "Display type of WFA state with corresponding symbol."},
   {"state-numbers", NULL, 'S', PFLAG, {0}, "NO",
    "Same as --states with state numbers inside the symbols."},
   {"basis", NULL, 'b', PFLAG, {0}, "NO",
    "Show initial basis states."}, 
   {"lc", "LIST", 'l', POSTR, {0}, NULL,
    "Show lc edges of WFA states [in '%s`] into non-basis."},
   {"lc-basis", "LIST", 'L', POSTR, {0}, NULL,
    "Show lc edges of WFA states [in '%s`] into basis."},
   {"shadows", NULL, 0, PFLAG, {0}, "NO",
    "Show shadows."},
   {"key", NULL, 0, PFLAG, {0}, "NO",
    "Show key."},
   {"levels", NULL, 0, PFLAG, {0}, "NO",
    "Show levels."},
   {"frame-list", "LIST", 'F', PSTR, {0}, "1",
    "Set frame(s) to be shown to `%s'."},
   {NULL, NULL, 0, 0, {0}, NULL, NULL }
};

static int 
checkargs (int argc, char **argv, toptions_t *options)
/*
 *  Check validness of command line parameters and of the parameter files.
 *
 *  Return value.
 *	index in argv of the first argv-element that is not an option.
 *
 *  Side effects:
 *	struct 'options' is modified.
 */
{
   int	optind;				/* last processed cmdline parameter */
   int  i;

   /*
    *  Store command line paramters in 'options->parameter_string'
    */
   {
      int size = 0;
      
      for (i = 1; i < argc; i++)
	 size += strlen (argv [i]) + 1;

      options->parameter_string = Calloc (size + 1, sizeof (char));
      
      for (i = 1; i < argc; i++)
      {
	 strcat (options->parameter_string, argv [i]);
	 strcat (options->parameter_string, " ");
      }
   }
   
   optind = parseargs (params, argc, argv,
		       "Generate figure in XFIG format of the "
		       "bintree structure of FIASCO encoded FILE.",
		       "With no FILE, or when FILE is -, "
		       "read standard input.\n"
		       "A LIST is defined by the regular expression "
		       "(NUM[-NUM],)*[NUM[-NUM]].",
		       " [FILE]", FIASCO_SHARE,
		       "system.fiascorc", ".fiascorc");

   options->grid       = * ((bool_t *) parameter_value (params, "grid"));
   options->color_grid = * ((bool_t *) parameter_value (params, "color-grid"));
   
   options->root_state = *((int *) parameter_value (params, "root-state"));
   options->max_depth  = *((int *) parameter_value (params, "max-depth"));
   options->cut_first  = *((bool_t *) parameter_value (params, "prune"));

   options->states     = * ((bool_t *) parameter_value (params, "states"));
   options->state_text = * ((bool_t *) parameter_value (params,
							"state-numbers"));
   {
      char *opt_val;
      
      options->LC_tree_list = NULL;
      if ((opt_val = (char *) parameter_value (params, "lc")))
      {
	 options->into_states = YES;
	 options->states      = YES;
	 if (strneq (opt_val, ""))	/* parameter specified */
	    options->LC_tree_list = String2List (opt_val);
      }
      else
	 options->into_states = NO;
      
      options->LC_basis_list = NULL;
      if ((opt_val = (char *) parameter_value (params, "lc-basis")))
      {
	 options->into_basis = YES;
	 options->basis	     = YES;
	 options->states     = YES;
	 if (strneq (opt_val, ""))	/* parameter specified */
	    options->LC_basis_list = String2List (opt_val);
      }
      else
	 options->into_basis = NO;
	 
      options->basis = *((bool_t *) parameter_value (params, "basis"));
   }
   
   options->frames_list
      = String2List ((char *) parameter_value (params, "frame-list"));
   if (options->frames_list)
      options->nr_of_frames = CountListEntries (options->frames_list);
   else
      options->nr_of_frames = 1;

   options->with_shadows = *((bool_t *) parameter_value (params, "shadows"));
   options->with_key     = *((bool_t *) parameter_value (params, "key"));
   options->with_levels  = *((bool_t *) parameter_value (params, "levels"));

   options->output_name = (char *) parameter_value (params, "output");
   
   return optind;
}

static int
subtree_depth (const wfa_t *wfa, int local_root)
/*
 *  Recursively calculate the depth of the subtree (starting with 'local_root')
 *  of the 'wfa'.
 *
 *  Return value:
 *	depth of subtree
 */
{
   int max_depth = 0; 
   int label;
   
   for (label = 0; label < MAXLABELS; label++)
      if (isedge (wfa->tree [local_root][label]))
	 max_depth = max (max_depth,
			  subtree_depth (wfa,
					 wfa->tree [local_root][label]) + 1);
   return max_depth;
}

static void
lrw_to_lwr (const wfa_t *wfa, int *lwr, int *pos, int local_root)
/*
 *  Recursively create 'lwr' with the state-numbers
 *  in lwr-order at the position of the state_numbers on lrw-order
 *
 *  wfa        - pointer on wfa-tree
 *  lrw_to_lwr - pointer on array of the new state-numbers
 *  pos        - pointer on the value that represents the current lwr-number
 *  local_root - current root state
 */
{
   /* Enter left part of the tree in array */
   if (ischild (wfa->tree [local_root][0]))
      lrw_to_lwr (wfa, lwr, pos, wfa->tree [local_root][0]);

   /* Enter root and increase counter */
   lwr [local_root] = (*pos)++;
 
   /* Enter right part of the tree in array */
   if (ischild (wfa->tree [local_root][1]))
      lrw_to_lwr (wfa, lwr, pos, wfa->tree [local_root][1]);
}

static void
make_fig (const wfa_t *wfa, toptions_t *options, int frame, int color_image)
/*
 *  Create a xfig-file based on the wfa-tree and the options
 *  that are defined in the function header
 *
 *  wfa         - pointer on the wfa-tree to be drawn
 *  options     - pointer on the options to be used
 *  frame       - frame-number of this wfa-tree
 *  color_image - is this wfa-tree a color image ? YES else NO
 */
{
   LCTree_t  LCTree;			/* tree in lwr-order to be drawn */
   int      *lwr;			/* field to transform state numbers
					   from lrw to lwr order */
   int      *color_field;		/* field were the colors for
					   states/grid are stored */
   int       pos           = 0;
   int       depth         = 0;
   int       TreeDeep      = 0;
   int       legend_offset = 0;
   FILE     *outfile;
   
   /*
    *  If no root_state assigned, invalid root or a more than one
    *  frame is defined, use original root_state
    */
   if (isrange (options->root_state)
       || options->root_state > wfa->root_state
       || options->nr_of_frames > 1)
      options->root_state = wfa->root_state;
   
   /* sets depth */
   depth = subtree_depth (wfa, options->root_state);
   if (!isrange (options->max_depth) && options->max_depth < depth)
      depth = options->max_depth;

   /* allocate memory for same storage fields */
   lwr         = Calloc (wfa->root_state + 1, sizeof (int));
   color_field = Calloc (wfa->root_state + 1, sizeof(int));

   /* calculate lrw_to_lwr-field */
   for (pos = 0; pos < wfa->basis_states; pos++)
      lwr [pos] = pos;
   lrw_to_lwr (wfa, lwr, &pos, wfa->root_state); /* do the rest */

   Init_LCTree (wfa, &LCTree, options, lwr); 
   Build_LCTree (wfa, &LCTree, options, options->root_state, lwr, 0); 
   AdjustLC (wfa, &LCTree, options); 
   Depth_limit_LCTree (&LCTree, LCTree.root_state, depth); 
   if (options->cut_first) /* cut tree at first linear combination in a state */
      LC_limit_LCTree (&LCTree, LCTree.root_state);
   GetColorField (wfa, &LCTree, color_field, lwr); 

   /*
    *  Generate outut filename
    */
   {
      char	*filename;

      if (strneq (options->output_name, "-"))	/* outfile specified */
      {
	 char *basename, *suffix;
	 
	 filename = Calloc (strlen (options->output_name) + 4 + 4 + 1,
			    sizeof (char));

	 basename = options->output_name;
	 suffix   = strrchr (basename, '.');

	 if (wfa->wfainfo->frames > 1)
	 {
	    if (suffix)
	    {
	       *suffix = 0;
	       suffix++;
	       if (*suffix)
		  sprintf (filename, "%s.%03d.%s", basename, frame, suffix);
	       else
		  sprintf (filename, "%s.%03d.fig", basename, frame);
	    }
	    else
	       sprintf (filename, "%s.%03d.fig", basename, frame);
	 }
	 else
	 {
	    if (suffix)
	       strcat (filename, basename);
	    else
	       sprintf (filename, "%s.fig", basename);
	 }
      }
      else
	 filename = strdup (options->output_name);

      outfile = open_file (filename, NULL, WRITE_ACCESS);
      if (!outfile)
	 file_error (filename);
      
      Free (filename);
   }
   
   xfig_header (outfile);

   CalcTreeCoordinates (&LCTree);
   TreeDeep = DrawTree (outfile, &LCTree, options, depth, color_field);

   if (options->basis || options->into_basis) /* draw basis states */
   {
      CalcBasisCoordinates (&LCTree, TreeDeep);
      DrawBasis (outfile, &LCTree, options);
   }

   if (options->with_key)		/* draw key */
      legend_offset = DrawLegend (outfile, wfa, options, frame, color_image,
				  LCTree.states[LCTree.basis_states-1].y);

   if (options->grid || options->color_grid) /* draw partition grid */
      DrawGrid (outfile, wfa, &LCTree, color_image, color_field, legend_offset,
		options);
   
   fclose(outfile);

   /* free memory */
   Remove_LCTree (&LCTree);
   Free (lwr);
   Free (color_field);
}

static void
browse_mwfa (toptions_t *options)
/*
 *  Read each frame of a video sequence and create a xfig-file for the
 *  frames in options->frames_list or for frame 0, if no frame in list
 *
 *  options - pointer of struct containing all options
 *
 *  No return value.
 */
{
   wfa_t        *wfa;			/* input MWFA */
   video_t	*video;			/* Video decoding struct */
   bitfile_t	*input;			/* input bit stream */
   int          frame_n = 0;		/* current frame number */
   int          counter = options->nr_of_frames; /* counter of files to create */

   video = alloc_video (YES);
   wfa   = alloc_wfa (NO);

   input = open_wfa (options->wfa_name, wfa->wfainfo);
   read_basis (wfa->wfainfo->basis_name, wfa);
   
   while (frame_n++ < wfa->wfainfo->frames && counter > 0) 
   {
      get_next_frame (YES, 0, 1.0, NULL, FORMAT_4_4_4, video, NULL,
		      wfa, input);
      /*
       *  Create xfig-file, if frame in frames_list or no list exists
       */
      if (!options->frames_list || SearchAscList (options->frames_list, frame_n))
      {
	 counter--;
	 make_fig (video->wfa, options, frame_n, wfa->wfainfo->color);
      }
   }

   free_wfa (wfa);
   free_video (video);
}

