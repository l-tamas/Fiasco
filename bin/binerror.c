/*
 *  error.c:		Error handling
 *
 *  Written by:		Stefan Frank
 *			Ullrich Hafner
 *  
 *  Credits:	Modelled after variable argument routines from Jef
 *		Poskanzer's pbmplus package. 
 *
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/03/20 21:29:59 $
 *  $Author: hafner $
 *  $Revision: 4.3 $
 *  $State: Exp $
 */

#define _ERROR_C

#include "config.h"

#include <stdio.h>

#if STDC_HEADERS
#	include <stdarg.h>
#	define VA_START(args, lastarg) va_start(args, lastarg)
#else  /* not STDC_HEADERS */
#	include <varargs.h>
#	define VA_START(args, lastarg) va_start(args)
#endif /* not STDC_HEADERS */
#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */

#if HAVE_SETJMP_H
#	include <setjmp.h>
#endif /* HAVE_SETJMP_H */

#include "fiasco.h"
#include "binerror.h"

/*****************************************************************************

			     global variables
  
*****************************************************************************/

int   error_line = 0;
char *error_file = NULL;

/*****************************************************************************

			     local variables
  
*****************************************************************************/

static char *executable = "(name not initialized)";

/*****************************************************************************

			       public code
  
*****************************************************************************/

void
init_error_handling (const char *name)
/*
 *  Initialize filename of executable.
 *
 *  No return value.
 */
{
   if (name)
      executable = strdup (name);
}

void
_error (const char *format, ...)
/*
 *  Print error message and exit.
 *
 *  No return value.
 */
{
   va_list	args;

   VA_START (args, format);

   fprintf (stderr, "%s: %s: line %d:\nError: ",
	    executable, error_file, error_line);
#if HAVE_VPRINTF
   vfprintf (stderr, format, args);
#elif HAVE_DOPRNT
   _doprnt (format, args, stderr);
#endif /* HAVE_DOPRNT */
   fputc ('\n', stderr);
   va_end(args);

   exit (1);
}

void
_file_error (const char *filename)
/*
 *  Print file error message and exit.
 *
 *  No return value.
 */
{
   fprintf (stderr, "%s: %s: line %d:\nError: ",
	    executable, error_file, error_line);
   perror (filename);

   exit (2);
}

void 
_warning (const char *format, ...)
/*
 *  Issue a warning and continue execution.
 *
 *  No return value.
 */
{
   va_list args;

   VA_START (args, format);

   fprintf (stderr, "%s: %s: line %d:\nWarning: ",
	    executable, error_file, error_line);
#if HAVE_VPRINTF
   vfprintf (stderr, format, args);
#elif HAVE_DOPRNT
   _doprnt (format, args, stderr);
#endif /* HAVE_DOPRNT */
   fputc ('\n', stderr);

   va_end (args);
}
