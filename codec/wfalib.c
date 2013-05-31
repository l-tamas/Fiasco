/*
 *  wfalib.c:		Library functions both for encoding and decoding
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/18 15:57:28 $
 *  $Author: hafner $
 *  $Revision: 5.5 $
 *  $State: Exp $
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "macros.h"
#include "error.h"

#include "wfa.h"
#include "misc.h"
#include "wfalib.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static unsigned
xy_to_address (unsigned x, unsigned y, unsigned level, unsigned n);

/*****************************************************************************

				public code
  
*****************************************************************************/

wfa_t *
alloc_wfa (bool_t coding)
/*
 *  WFA constructor:
 *  Initialize the WFA structure 'wfa' and allocate memory.
 *  Flag 'coding' indicates whether WFA is used for coding or decoding.
 *
 *  Return value:
 *	pointer to the new WFA structure
 */
{
   wfa_t *wfa = fiasco_calloc (1, sizeof (wfa_t));
		 
   /*
    *  Allocate memory
    */
   wfa->final_distribution = fiasco_calloc (MAXSTATES, sizeof (real_t));
   wfa->level_of_state     = fiasco_calloc (MAXSTATES, sizeof (byte_t));
   wfa->domain_type        = fiasco_calloc (MAXSTATES, sizeof (byte_t));
   wfa->delta_state        = fiasco_calloc (MAXSTATES, sizeof (bool_t));
   wfa->tree               = fiasco_calloc (MAXSTATES * MAXLABELS, sizeof (word_t));
   wfa->x                  = fiasco_calloc (MAXSTATES * MAXLABELS, sizeof (word_t));
   wfa->y                  = fiasco_calloc (MAXSTATES * MAXLABELS, sizeof (word_t));
   wfa->mv_tree            = fiasco_calloc (MAXSTATES * MAXLABELS, sizeof (mv_t));
   wfa->y_state            = fiasco_calloc (MAXSTATES * MAXLABELS, sizeof (word_t));
   wfa->into               = fiasco_calloc (MAXSTATES * MAXLABELS * (MAXEDGES + 1),
				     sizeof (word_t));
   wfa->weight             = fiasco_calloc (MAXSTATES * MAXLABELS * (MAXEDGES + 1),
				     sizeof (real_t));
   wfa->int_weight         = fiasco_calloc (MAXSTATES * MAXLABELS * (MAXEDGES + 1),
				     sizeof (word_t));
   wfa->wfainfo            = fiasco_calloc (1, sizeof (wfa_info_t));;
   wfa->prediction         = fiasco_calloc (MAXSTATES * MAXLABELS, sizeof (byte_t));

   wfa->wfainfo->wfa_name   = NULL;
   wfa->wfainfo->basis_name = NULL;
   wfa->wfainfo->title 	    = strdup ("");
   wfa->wfainfo->comment    = strdup ("");

   /*
    *  Initialize structure
    */
   {
      unsigned  state, label;

      wfa->states       = 0;
      wfa->basis_states = 0;
      wfa->root_state   = 0;
      for (state = 0; state < MAXSTATES; state++) 
      {
	 wfa->final_distribution [state] = 0;
	 wfa->domain_type [state]        = 0;
	 for (label = 0; label < MAXLABELS; label++)
	 {
	    wfa->into [state][label][0] = NO_EDGE;
	    wfa->tree [state][label]    = RANGE;
	    wfa->y_state [state][label] = RANGE;
	 }
      }
   }

   if (coding)				/* initialize additional variables */
      wfa->y_column = fiasco_calloc (MAXSTATES * MAXLABELS, sizeof (byte_t));
   else
      wfa->y_column = NULL;
   
   return wfa;
}

void
free_wfa (wfa_t *wfa)
/*
 *  WFA destructor:
 *  Free memory of given 'wfa'.
 *
 *  No return value.
 *
 *  Side effects:
 *	'wfa' struct is discarded.
 */
{
   if (wfa->wfainfo->wfa_name)
      fiasco_free (wfa->wfainfo->wfa_name);
   if (wfa->wfainfo->basis_name)
      fiasco_free (wfa->wfainfo->basis_name);
   if (wfa->wfainfo->title)
      fiasco_free (wfa->wfainfo->title);
   if (wfa->wfainfo->comment)
      fiasco_free (wfa->wfainfo->comment);

   fiasco_free (wfa->final_distribution);
   fiasco_free (wfa->level_of_state);
   fiasco_free (wfa->domain_type);
   fiasco_free (wfa->tree);
   fiasco_free (wfa->x);
   fiasco_free (wfa->y);
   fiasco_free (wfa->mv_tree);
   fiasco_free (wfa->y_state);
   fiasco_free (wfa->into);
   fiasco_free (wfa->weight);
   fiasco_free (wfa->int_weight);
   fiasco_free (wfa->wfainfo);
   fiasco_free (wfa->prediction);
   fiasco_free (wfa->delta_state);
   if (wfa->y_column)
      fiasco_free (wfa->y_column);
   fiasco_free (wfa);
}

real_t 
compute_final_distribution (unsigned state, const wfa_t *wfa)
/*
 *  Compute the final distribution of the given 'state'.
 *  Uses the fact that the generated 'wfa' is average preserving.
 *
 *  Return value:
 *	final distribution
 */
{
   unsigned label;
   real_t   final = 0;

   for (label = 0; label < MAXLABELS; label++)
   {
      unsigned edge;
      int      domain;
      
      if (ischild (domain = wfa->tree [state][label]))
	 final += wfa->final_distribution [domain];
      for (edge = 0; isedge (domain = wfa->into [state][label][edge]); edge++)
	 final += wfa->weight [state][label][edge]
		  * wfa->final_distribution [domain];
   }
   
   return final / MAXLABELS;
}

word_t *
compute_hits (unsigned from, unsigned to, unsigned n, const wfa_t *wfa)
/*
 *  Selects the 'n' most popular domain images of the given 'wfa'.
 *  Consider only linear combinations of state images
 *  {i | 'from' <= i <= 'to'}. I.e. domains are in {i | from <= i < 'to'}
 *  Always ensure that state 0 is among selected states even though from
 *  may be > 0.
 *  
 *  Return value:
 *	pointer to array of the most popular state images
 *	sorted by increasing state numbers and terminated by -1
 */
{
   word_t   *domains;
   unsigned  state, label, edge;
   int       domain;
   pair_t   *hits = fiasco_calloc (to, sizeof (pair_t));

   for (domain = 0; domain < (int) to; domain++)
   {
      hits [domain].value = domain;
      hits [domain].key   = 0;
   }
   
   for (state = from; state <= to; state++)
      for (label = 0; label < MAXLABELS; label++)
	 for (edge = 0; isedge (domain = wfa->into [state][label][edge]);
	      edge++)
	    hits [domain].key++;

   qsort (hits + 1, to - 1, sizeof (pair_t), sort_desc_pair);

   n       = min (to, n);
   domains = fiasco_calloc (n + 1, sizeof (word_t));

   for (domain = 0; domain < (int) n && (!domain || hits [domain].key);
	domain++)
      domains [domain] = hits [domain].value;
   if (n != domain)
      debug_message ("Only %d domains have been used in the luminance.",
		     domain);
   n = domain;
   qsort (domains, n, sizeof (word_t), sort_asc_word);
   domains [n] = -1;
   
   fiasco_free (hits);
   
   return domains;
}

void
append_edge (unsigned from, unsigned into, real_t weight,
	     unsigned label, wfa_t *wfa)
/*
 *  Append an edge from state 'from' to state 'into' with
 *  the given 'label' and 'weight' to the 'wfa'.
 *
 *  No return value.
 *
 *  Side effects:
 *	'wfa' structure is changed.
 */
{
   unsigned new;			/* position of the new edge */
   unsigned edge;

   /*
    *  First look where to insert the new edge:
    *  edges are sorted by increasing 'into' values
    */
   for (new = 0; (isedge (wfa->into [from][label][new])
		  && wfa->into [from][label][new] < (int) into); new++)
      ;
   /*
    *  Move the edges 'n' to position 'n+1', for n = max, ..., 'new'
    */
   for (edge = new; isedge (wfa->into [from][label][edge]); edge++)
      ;
   for (edge++; edge != new; edge--)
   {
      wfa->into [from][label][edge]    = wfa->into [from][label][edge - 1];
      wfa->weight [from][label][edge]  = wfa->weight [from][label][edge - 1];
      wfa->int_weight [from][label][edge]
	 = wfa->int_weight [from][label][edge - 1];
   }
   /*
    *  Insert the new edge
    */
   wfa->into [from][label][edge]       = into;
   wfa->weight [from][label][edge]     = weight;
   wfa->int_weight [from][label][edge] = weight * 512 + 0.5;
}

void 
remove_states (unsigned from, wfa_t *wfa)
/* 
 *  Remove 'wfa' states 'wfa->basis_states',...,'wfa->states' - 1.
 *
 *  No return value.
 *
 *  Side effects:
 *	'wfa' structure is cleared for the given states.
 */
{
   unsigned state;

   for (state = from; state < wfa->states; state++)
   {
      unsigned label;
      
      for (label = 0; label < MAXLABELS; label++) 
      {
	 wfa->into [state][label][0]      = NO_EDGE;
	 wfa->tree [state][label]         = RANGE;
	 wfa->prediction [state][label]   = FALSE;
	 wfa->y_state [state][label]      = RANGE;
	 wfa->mv_tree [state][label].type = NONE;
	 wfa->mv_tree [state][label].fx   = 0;
	 wfa->mv_tree [state][label].fy   = 0;
	 wfa->mv_tree [state][label].bx   = 0;
	 wfa->mv_tree [state][label].by   = 0;
      }
      wfa->domain_type [state] = 0;
      wfa->delta_state [state] = FALSE;
   }

   wfa->states = from;
}

void
copy_wfa (wfa_t *dst, const wfa_t *src)
/*
 *  Copy WFA struct 'src' to WFA struct 'dst'.
 *
 *  No return value.
 *
 *  Side effects:
 *	'dst' is filled with same data as 'src'
 *
 *  NOTE: size of WFA 'dst' must be at least size of WFA 'src'
 */
{
   unsigned state;

   memset (dst->final_distribution, 0, MAXSTATES * sizeof (real_t));
   memset (dst->level_of_state, 0, MAXSTATES * sizeof (byte_t));
   memset (dst->domain_type, 0, MAXSTATES * sizeof (byte_t));
   memset (dst->mv_tree, 0, MAXSTATES * MAXLABELS * sizeof (mv_t));
   memset (dst->tree, 0, MAXSTATES * MAXLABELS * sizeof (word_t));
   memset (dst->x, 0, MAXSTATES * MAXLABELS * sizeof (word_t));
   memset (dst->y, 0, MAXSTATES * MAXLABELS * sizeof (word_t));
   memset (dst->y_state, 0, MAXSTATES * MAXLABELS * sizeof (word_t));
   memset (dst->into, NO_EDGE,
	   MAXSTATES * MAXLABELS * (MAXEDGES + 1) * sizeof (word_t));
   memset (dst->weight, 0,
	   MAXSTATES * MAXLABELS * (MAXEDGES + 1) * sizeof (real_t));
   memset (dst->int_weight, 0,
	   MAXSTATES * MAXLABELS * (MAXEDGES + 1) * sizeof (word_t));
   memset (dst->prediction, 0, MAXSTATES * MAXLABELS * sizeof (byte_t));
   memset (dst->delta_state, 0, MAXSTATES * sizeof (bool_t));
   if (dst->y_column)
      memset (dst->y_column, 0, MAXSTATES * MAXLABELS * sizeof (byte_t));

   for (state = 0; state < MAXSTATES; state++) /* clear WFA struct */
   {
      unsigned label;
      
      for (label = 0; label < MAXLABELS; label++)
      {
	 dst->into [state][label][0]      = NO_EDGE;
	 dst->tree [state][label]         = RANGE;
	 dst->mv_tree [state][label].type = NONE;
	 dst->y_state[state][label]       = RANGE;
      }
      dst->delta_state [state] = NO;
      dst->domain_type [state] = 0;
   }
   
   dst->frame_type   = src->frame_type;
   dst->states 	     = src->states;
   dst->basis_states = src->basis_states;
   dst->root_state   = src->root_state;

   memcpy (dst->wfainfo, src->wfainfo, sizeof (wfa_info_t));

   if (dst->states == 0)		/* nothing to do */
      return;

   memcpy (dst->final_distribution, src->final_distribution,
	   src->states * sizeof (real_t));
   memcpy (dst->level_of_state, src->level_of_state,
	   src->states * sizeof (byte_t));
   memcpy (dst->domain_type, src->domain_type,
	   src->states * sizeof (byte_t));
   memcpy (dst->delta_state, src->delta_state,
	   src->states * sizeof (bool_t));
   memcpy (dst->mv_tree, src->mv_tree,
	   src->states * MAXLABELS * sizeof (mv_t));
   memcpy (dst->tree, src->tree,
	   src->states * MAXLABELS * sizeof (word_t));
   memcpy (dst->x, src->x,
	   src->states * MAXLABELS * sizeof (word_t));
   memcpy (dst->y, src->y,
	   src->states * MAXLABELS * sizeof (word_t));
   memcpy (dst->y_state, src->y_state,
	   src->states * MAXLABELS * sizeof (word_t));
   memcpy (dst->into, src->into,
	   src->states * MAXLABELS * (MAXEDGES + 1) * sizeof (word_t));
   memcpy (dst->weight, src->weight,
	   src->states * MAXLABELS * (MAXEDGES + 1) * sizeof (real_t));
   memcpy (dst->int_weight, src->int_weight,
	   src->states * MAXLABELS * (MAXEDGES + 1) * sizeof (word_t));
   memcpy (dst->prediction, src->prediction,
	   src->states * MAXLABELS * sizeof (byte_t));
   if (dst->y_column)
      memcpy (dst->y_column, src->y_column,
	      src->states * MAXLABELS * sizeof (byte_t));
}

void
locate_subimage (unsigned orig_level, unsigned level, unsigned bintree,
		 unsigned *x, unsigned *y, unsigned *width, unsigned *height)
/*
 *  Compute pixel coordinates of the subimage which 'bintree' address is given.
 *  The level of the original image is 'orig_level' and the level of the
 *  subimage is 'level'.
 *
 *  No return value.
 *
 *  Side effects:
 *	'*x', '*y'		coordinates of the upper left corner
 *      '*width', '*height'	size of image
 */
{
   /*
    *  Compute coordinates of the subimage
    */
   *x = *y = 0;				/* start at NW corner */
   *width  = width_of_level (level);
   *height = height_of_level (level);

   if (level > orig_level)
   {
      error ("size of tile must be less or equal than image size.");
      return;
   }
   else if (bintree >= (unsigned) (1 << (orig_level - level)))
   {
      error ("address out of bounds.");
      return;
   }
   else if (level < orig_level)
   {
      unsigned mask;			/* mask for bintree -> xy conversion */
      bool_t   hor;			/* 1 next subdivision is horizontal
					   0 next subdivision is vertical */
      unsigned l = orig_level - 1;	/* current level */
      
      hor = orig_level % 2;		/* start with vertival subdivision
					   for square image and vice versa */
   
      for (mask = 1 << (orig_level - level - 1); mask; mask >>= 1, hor = !hor)
      {
	 if (bintree & mask)		/* change coordinates */
	 {
	    if (hor)			/* horizontal subdivision */
	       *y += height_of_level (l);
	    else			/* vertical subdivision */
	       *x += width_of_level (l);
	 }
	 l--;
      }
   }
}

void
compute_spiral (int *vorder, unsigned image_width, unsigned image_height,
		unsigned tiling_exp, bool_t inc_spiral)
/*
 *  Compute image tiling with spiral order.
 *  'inc_spiral' specifies whether the spiral starts in the middle
 *  of the image (TRUE) or at the border (FALSE).
 *  'image_width'x'image_height' define the size of the image.
 *  The image is split into 'tiling->exp' tiles.
 *
 *  No return value.
 *
 *  Side effects:
 *	vorder[] is filled with tiling permutation
 */
{
   unsigned x, y;			/* current position */
   unsigned xmin, xmax, ymin, ymax;	/* boundaries for current line */
   unsigned width, height;		/* offset for each tile */
   unsigned lx, ly, level;		/* level x and y */
   unsigned tiles;			/* total number of tiles */
   unsigned address;			/* bintree address */
   
   lx     = log2 (image_width - 1) + 1;
   ly     = log2 (image_height - 1) + 1;
   level  = max (lx, ly) * 2 - ((ly == lx + 1) ? 1 : 0);
   tiles  = 1 << tiling_exp;		/* Number of image tiles */
   width  = width_of_level (level - tiling_exp);
   height = height_of_level (level - tiling_exp);
   for (address = 0; address < tiles; address++)
   {
      unsigned x0, y0, width, height;
      
      locate_subimage (level, level - tiling_exp, address,
		       &x0, &y0, &width, &height);
      vorder [address] = (x0 < image_width && y0 < image_height) ? 0 : -1;
   }

   xmin    = 0;
   xmax    = width_of_level (level);
   ymin    = 0;
   ymax    = height_of_level (level);
   address = 0;

   /*
    *  1234
    *  CDE5  Traverse image in spiral order 
    *  BGF6  starting at the top left corner
    *  A987
    */
   while (TRUE)
   {
      for (x = xmin, y = ymin; x < xmax; x += width) /* W>E */
      {
	 while (vorder [address] == -1)
	    address++;
	 if (x < image_width && y < image_height) /* valid range */
	    vorder [address++] = xy_to_address (x, y, level, tiling_exp);
	 while (address < tiles && vorder [address] == -1)
	    address++;
      }
      ymin += height;

      if (address >= tiles)
	 break;
      
      for (x = xmax - width, y = ymin; y < ymax; y += height) /* N>S  */
      {
	 while (vorder [address] == -1)
	    address++;
	 if (x <= image_width && y <= image_height) /* valid range */
	    vorder [address++] = xy_to_address (x, y, level, tiling_exp);
	 while (address < tiles && vorder [address] == -1)
	    address++;
      }
      xmax -= width;

      if (address >= tiles)
	 break;

      for (x = xmax - width, y = ymax - width; x >= xmin; x -= width) /* E<W */
      {
	 while (vorder [address] == -1)
	    address++;
	 if (x <= image_width && y <= image_height) /* valid range */
	    vorder [address++] = xy_to_address (x, y, level, tiling_exp);
	 while (address < tiles && vorder [address] == -1)
	    address++;
      }
      ymax -= height;

      if (address >= tiles)
	 break;

      for (x = xmin, y = ymax - height; y >= ymin; y -= height)	/* S>N */
      {
	 while (vorder [address] == -1)
	    address++;
	 if (x <= image_width && y <= image_height) /* valid range */
	    vorder [address++] = xy_to_address (x, y, level, tiling_exp);
	 while (address < tiles && vorder [address] == -1)
	    address++;
      }
      xmin += width;
	 
      if (address >= tiles)
	 break;
   }

   if (inc_spiral)
   {
      int i = 0, j = tiles - 1;

      while (i < j)
      {
	 int tmp;
	    
	 while (vorder [i] == -1)
	    i++;
	 while (vorder [j] == -1)
	    j--;
	    
	 tmp 	       = vorder [i];
	 vorder [i] = vorder [j];
	 vorder [j] = tmp;
	 i++;
	 j--;
      }
   }
   /*
    *  Print tiling info
    */
   {
      unsigned number;
      
      for (number = 0, address = 0; address < tiles; address++)
	 if (vorder [address] != -1)
	    debug_message ("number %d: address %d",
			   number++, vorder [address]);
   }
}

bool_t
find_range (unsigned x, unsigned y, unsigned band,
	    const wfa_t *wfa, unsigned *range_state, unsigned *range_label)
/*
 *  Find a range ('*range_state', '*range_label') that contains
 *  pixel ('x', 'y') in the iven color 'band'.
 *
 *  Return value:
 *	TRUE on success, or FALSE if there is no such range
 *
 *  Side effects:
 *	'*range_state' and '*range_label' are modified on success.
 */
{
   unsigned state, label;
   unsigned first_state, last_state;
   bool_t   success = NO;
   
   first_state = wfa->basis_states;
   last_state  = wfa->states;
   if (wfa->wfainfo->color)
      switch (band)
      {
	 case Y:
	    first_state = wfa->basis_states;
	    last_state  = wfa->tree [wfa->tree [wfa->root_state][0]][0];
	    break;
	 case Cb:
	    first_state = wfa->tree [wfa->tree [wfa->root_state][0]][0] + 1;
	    last_state  = wfa->tree [wfa->tree [wfa->root_state][0]][1];
	    break;
	 case Cr:
	    first_state = wfa->tree [wfa->tree [wfa->root_state][0]][1] + 1;
	    last_state  = wfa->states;
	    break;
	 default:
	    error ("unknown color component.");
      }

   for (state = first_state; state < last_state; state++)
      for (label = 0; label < MAXLABELS; label++)
	 if (isrange (wfa->tree [state][label]))
	    if (x >= wfa->x [state][label] && y >= wfa->y [state][label]
		&& x < (unsigned) (wfa->x [state][label]
			+ width_of_level (wfa->level_of_state [state] - 1))
		&& y < (unsigned) (wfa->y [state][label]
			+ height_of_level (wfa->level_of_state [state] - 1))) 
	    {
	       success      = YES;
	       *range_state = state;
	       *range_label = label;

	       return success;
	    }

   return success;
}

void
sort_ranges (unsigned state, unsigned *domain,
	     range_sort_t *rs, const wfa_t *wfa)
/*
 *  Generate list of ranges in coder order.
 *  'state' is the current state of the call tree while 'domain' is the
 *  index of the last added WFA state.
 *
 *  Side effects:
 *	'domain' is incremented after recursion returns
 *	'rs'	 is filled accordingly
 *
 *  No return value.
 */
{
   unsigned label;
   
   for (label = 0; label < MAXLABELS; label++)
   {
      if (isrange (wfa->tree [state][label]))
	 rs->range_subdivided [rs->range_no] = NO;
      else
      {
	 sort_ranges (wfa->tree [state][label], domain, rs, wfa);
	 rs->range_subdivided [rs->range_no] = YES;
      }

      rs->range_state [rs->range_no]      = state;
      rs->range_label [rs->range_no]      = label;
      rs->range_max_domain [rs->range_no] = *domain;
      while (!usedomain (rs->range_max_domain [rs->range_no], wfa))
	 rs->range_max_domain [rs->range_no]--;

      if (label == 1 || !rs->range_subdivided [rs->range_no])
	 rs->range_no++;
   }
   
   (*domain)++;
}

bool_t
locate_delta_images (wfa_t *wfa)
/*
 *  Locate all WFA states that are part of a delta approximation.
 *  I.e., these states are assigned to ranges that have been predicted
 *  via MC or ND.
 *
 *  Return value:
 *	TRUE	at least one state is part of a delta approximation
 *	FALSE	no delta approximations in this WFA
 *
 *  Side effects:
 *	'wfa->delta [state][label]' is set accordingly.
 */
{
   unsigned state, label;
   bool_t   delta = NO;

   for (state = wfa->root_state; state >= wfa->basis_states; state--)
      wfa->delta_state [state] = NO;

   for (state = wfa->root_state; state >= wfa->basis_states; state--)
      for (label = 0; label < MAXLABELS; label++)
	 if (ischild (wfa->tree [state][label]))
	    if (wfa->mv_tree [state][label].type != NONE
		|| isedge (wfa->into [state][label][0])
		|| wfa->delta_state [state])
	    {
	       delta = YES;
	       wfa->delta_state [wfa->tree [state][label]] = YES;
	    }

   return delta;
}

/*****************************************************************************

				private code
  
******************************************************************************/

static unsigned
xy_to_address (unsigned x, unsigned y, unsigned level, unsigned n)
/*
 *  Compute bintree address of subimage at coordinates ('x', 'y').
 *  Size of original image is determined by 'level'.
 *  'n' specifies number of iterations.
 *
 *  Return value:
 *	address of subimage
 */ 
{ 
   unsigned address = 0;

   while (n--)
   {
      address <<= 1;
      if (--level % 2) 
      {
	 if (x & width_of_level (level))
	    address++;
      }
      else
      {
	 if (y & height_of_level (level))
	    address++;
      }
   }
   
   return address;
}
