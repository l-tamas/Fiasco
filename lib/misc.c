/*
 *  misc.c:		Some usefull functions, that don't fit in one of 
 *			the other files and that are needed by at least
 *			two modules. 
 *
 *  Written by:		Stefan Frank
 *			Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:49:37 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#include "config.h"

#include <math.h>
#include <ctype.h>
#include <time.h>

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "macros.h"
#include "error.h"

#include "bit-io.h"
#include "misc.h"

/*****************************************************************************

				prototypes
  
*****************************************************************************/

static void
remove_comments (FILE *file);

/*****************************************************************************

				public code
  
*****************************************************************************/

void *
fiasco_calloc (size_t n, size_t size)
/*
 *  Allocate memory like calloc ().
 *
 *  Return value: Pointer to the new block of memory on success,
 *		  otherwise the program is terminated.
 */
{
   void	*ptr;				/* pointer to the new memory block */

   if (n <= 0 || size <= 0)
      error ("Can't allocate memory for %d items of size %d",
	     (int) n, (int) size);

   ptr = calloc (n, size);
   if (ptr == NULL)
      error ("Out of memory!");

   return ptr;
}

void
fiasco_free (void *ptr)
/*
 *  Free memory given by the pointer 'memory'
 *
 *  No return value.
 */
{
   if (ptr != NULL)
      free (ptr);
   else
      warning ("Can't free memory block <NULL>.");
}

unsigned
prg_timer (clock_t *last_timer, enum action_e action)
/*
 *  If 'action' == START then store current value of system timer.
 *  If 'action' == STOP	 then compute number of elapsed micro seconds since
 *			 the last time 'prg_timer' was called
 *			 with 'action' == START.
 *
 *  Return value:
 *	Number of elapsed micro seconds if 'action' == STOP
 *	0				if 'action' == START
 *
 *  Side effects:
 *	last_timer is set to current timer if action == START
 */
{
   assert (last_timer);
   
   if (action == START)
   {
      *last_timer = clock ();
      return 0;
   }
   else
      return (clock () - *last_timer) / (CLOCKS_PER_SEC / 1000.0);
}

real_t 
read_real (FILE *infile)
/* 
 *  Read one real value from the given input stream 'infile'.
 *  
 *  Return value:
 *	real value on success
 */
{
   float input;

   assert (infile);
   
   remove_comments (infile);
   if (fscanf(infile, "%f", &input) != 1)
      error("Can't read float value!");

   return (real_t) input;
}

int 
read_int (FILE *infile)
/* 
 *  Read one integer value from the given input stream 'infile'.
 *
 *  Return value:
 *	integer value on success
 */
{
   int input;				/* integer */

   assert (infile);
   
   remove_comments (infile);
   if (fscanf(infile, "%d", &input) != 1)
      error("Can't read integer value!");

   return input;
}
   
static void
remove_comments (FILE *file)
/*
 *  Remove shell/pgm style comments (#) from the input 'file'
 *
 *  No return value.
 */
{
   int c;				/* current character */
   
   assert (file);
   
   do
   {
      while (isspace(c = getc (file)))
	 ;
      if (c == EOF)
	 error ("EOF reached, input seems to be truncated!");
      if (c == '#')
      {
	 int dummy;
	 
	 while (((dummy = getc (file)) != '\n') && dummy != EOF)
	    ;
	 if (dummy == EOF)
	    error ("EOF reached, input seems to be truncated!");
      }
      else 
	 ungetc (c, file);
   } while (c == '#');
}

void
write_rice_code (unsigned value, unsigned rice_k, bitfile_t *output)
/*
 *  Write 'value' to the stream 'output' using Rice coding with base 'rice_k'.
 *
 *  No return value.
 */
{
   unsigned unary;			/* unary part of Rice Code */

   assert (output);
   
   for (unary = value >> rice_k; unary; unary--)
      put_bit (output, 1);
   put_bit (output, 0);
   put_bits (output, value & ((1 << rice_k) - 1), rice_k);
}

unsigned
read_rice_code (unsigned rice_k, bitfile_t *input)
/*
 *  Read a Rice encoded integer (base 'rice_k') from the stream 'input'.
 *
 *  Return value:
 *	decoded integer
 */
{
   unsigned unary;			/* unary part of Rice code */
   
   assert (input);
   
   for (unary = 0; get_bit (input); unary++) /* unary part */
      ;

   return (unary << rice_k) | get_bits (input, rice_k);
}

void
write_bin_code (unsigned value, unsigned maxval, bitfile_t *output)
/*
 *  Write 'value' to the stream 'output' using an adjusted binary code
 *  based on given 'maxval'.
 *
 *  No return value.
 */
{
   unsigned k;
   unsigned r;
   
   assert (output && maxval && value <= maxval);

   k = log2 (maxval + 1);
   r = (maxval + 1) % (1 << k);

   if (value < maxval + 1 - 2 * r)	/* 0, ... , maxval - 2r */
      put_bits (output, value, k);
   else					/* maxval - 2r + 1, ..., maxval */
      put_bits (output, value + maxval + 1 - 2 * r, k + 1);
}

unsigned
read_bin_code (unsigned maxval, bitfile_t *input)
/*
 *  Read a bincode encoded integer from the stream 'input'.
 *
 *  Return value:
 *	decoded integer
 */
{
   unsigned k;
   unsigned r;
   unsigned value;
   
   assert (input);

   k = log2 (maxval + 1);
   r = (maxval + 1) % (1 << k);

   value = get_bits (input, k);
   if (value < maxval + 1 - 2 * r)
      return value;
   else
   {
      value <<= 1;
      if (get_bit (input))
	 value++;
      return value - maxval - 1 + 2 * r;
   }
}

unsigned
bits_rice_code (unsigned value, unsigned rice_k)
/*
 *  Compute number of bits needed for coding integer 'value'
 *  with given Rice code 'rice_k'.
 *
 *  Return value:
 *	number of bits
 */
{
   unsigned unary;
   unsigned bits = 0;
   
   for (unary = value >> rice_k; unary; unary--)
      bits++;
   bits += rice_k + 1;

   return bits;
}

unsigned
bits_bin_code (unsigned value, unsigned maxval)
/*
 *  Compute number of bits needed for coding integer 'value'
 *  with adjusted binary code of given maximum value 'maxval'.
 *
 *  Return value:
 *	number of bits
 */
{
   unsigned k;
   unsigned r;

   assert (maxval && value <= maxval);

   k = log2 (maxval + 1);
   r = (maxval + 1) % (1 << k);

   return value < maxval + 1 - 2 * r ? k : k + 1;
}

unsigned *
init_clipping (void)
/*
 *  Initialize the clipping tables
 *
 *  Return value:
 *	pointer to clipping table
 */
{
   static unsigned *gray_clip = NULL;	/* clipping array */

   if (gray_clip == NULL)		/* initialize clipping table */
   {
      int i;				/* counter */

      gray_clip  = calloc (256 * 3, sizeof (unsigned));
      if (!gray_clip)
      {
	 set_error (_("Out of memory."));
	 return NULL;
      }
      gray_clip += 256;

      for (i = -256; i < 512; i++)
	 if (i < 0)
	    gray_clip [i] = 0;
	 else if (i > 255)
	    gray_clip [i] = 255;
	 else
	    gray_clip [i] = i;
   }

   return gray_clip;
}

#ifndef HAVE_STRDUP
char *
strdup (const char *s)
/*
 *  Duplicate given string 's'.
 *
 *  Return value:
 *	pointer to new string value
 */
{
   assert (s);
   
   return strcpy (fiasco_calloc (strlen (s) + 1, sizeof (char)), s);
}
#endif /* not HAVE_STRDUP */

#ifndef HAVE_LOG2
double
log2 (double x)
/*
 *  Return value:
 *	base-2 logarithm of 'x'
 */
{
   return log (x) / 0.69314718;
}
#endif /* not HAVE_LOG2 */

#ifndef HAVE_STRCASECMP
bool_t
strcaseeq (const char *s1, const char *s2)
/*
 *  Compare strings 's1' and 's2', ignoring  the  case of the characters.
 *
 *  Return value:
 *	TRUE if strings match, else FALSE
 */
{
   bool_t  matched;
   char	  *ls1, *ls2, *ptr;

   assert (s1 && s2);
   
   ls1 = strdup (s1);
   ls2 = strdup (s2);
   
   for (ptr = ls1; *ptr; ptr++)
      *ptr = tolower (*ptr);
   for (ptr = ls2; *ptr; ptr++)
      *ptr = tolower (*ptr);

   matched = streq (ls1, ls2) ? YES : NO;

   fiasco_free (ls1);
   fiasco_free (ls2);
   
   return matched;
}
#endif /* not HAVE_STRCASECMP */

real_t
variance (const word_t *pixels, unsigned x0, unsigned y0,
	  unsigned width, unsigned height, unsigned cols)
/*
 *  Compute variance of subimage ('x0', y0', 'width', 'height') of 
 *  the image data given by 'pixels' ('cols' is the number of pixels
 *  in one row of the image).
 *
 *  Return value:
 *	variance
 */
{
   real_t   average;			/* average of pixel values */
   real_t   variance;			/* variance of pixel values */
   unsigned x, y;			/* pixel counter */
   unsigned n;				/* number of pixels */

   assert (pixels);
   
   for (average = 0, n = 0, y = y0; y < y0 + height; y++)
      for (x = x0; x < min (x0 + width, cols); x++, n++)
	 average += pixels [y * cols + x] / 16;

   average /= n;

   for (variance = 0, y = y0; y < y0 + height; y++)
      for (x = x0; x < min (x0 + width, cols); x++)
	 variance += square ((pixels [y * cols + x] / 16) - average);

   return variance;
}

int
sort_asc_word (const void *value1, const void *value2)
/*
 *  Sorting function for quicksort.
 *  Smallest values come first.
 */
{
   if (* (word_t *) value1 < * (word_t *) value2)
      return -1;
   else if (* (word_t *) value1 > * (word_t *) value2)
      return +1;
   else
      return 0;
}

int
sort_desc_word (const void *value1, const void *value2)
/*
 *  Sorting function for quicksort.
 *  Largest values come first.
 */
{
   if (* (word_t *) value1 > * (word_t *) value2)
      return -1;
   else if (* (word_t *) value1 < * (word_t *) value2)
      return +1;
   else
      return 0;
}

int
sort_asc_pair (const void *value1, const void *value2)
/*
 *  Sorting function for quicksort.
 *  Smallest values come first.
 */
{
   word_t v1 = ((pair_t *) value1)->key;
   word_t v2 = ((pair_t *) value2)->key;
   
   if (v1 < v2)
      return -1;
   else if (v1 > v2)
      return +1;
   else
      return 0;
}

int
sort_desc_pair (const void *value1, const void *value2)
/*
 *  Sorting function for quicksort.
 *  Largest values come first.
 */
{
   word_t v1 = ((pair_t *) value1)->key;
   word_t v2 = ((pair_t *) value2)->key;

   if (v1 > v2)
      return -1;
   else if (v1 < v2)
      return +1;
   else
      return 0;
}
