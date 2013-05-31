/*
 *  bit-io.c:		Buffered and bit oriented file I/O 
 *
 *  Written by:		Ullrich Hafner
 *  
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */
 
/*
 *  $Date: 2000/07/07 19:21:56 $
 *  $Author: hafner $
 *  $Revision: 5.2 $
 *  $State: Exp $
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "macros.h"
#include "types.h"
#include "error.h"

#include "misc.h"
#include "bit-io.h"

/*****************************************************************************

			     local constants
  
*****************************************************************************/

static const unsigned BUFFER_SIZE = 16350;

static const unsigned mask[] = {0x0001, 0x0002, 0x0004, 0x0008,
				0x0010, 0x0020, 0x0040, 0x0080,
				0x0100, 0x0200, 0x0400, 0x0800,
				0x1000, 0x2000, 0x4000, 0x8000};

/*****************************************************************************

				public code
  
*****************************************************************************/

FILE *
open_file (const char *filename, const char *env_var, openmode_e mode)
/*
 *  Try to open file 'filename' with mode 'mode' (READ_ACCESS, WRITE_ACCESS).
 *  Scan the current directory first and then cycle through the
 *  path given in the environment variable 'env_var', if set.
 * 
 *  Return value:
 *	Pointer to open file on success, else NULL.
 */
{
   char 	      *path;		/* current path */
   FILE 	      *fp;		/* file pointer of I/O stream */
   char 	      *read_mode;    	/* read access identifier */
   char 	      *write_mode;    	/* write access identifier */
   char 	      *ext_filename = NULL; /* full path of file */
   char		      *env_path     = NULL; /* path given by 'env_var' */
   char const * const  PATH_SEP     = " ;:,"; /* path separation characters */
   char const * const  DEFAULT_PATH = "."; /* default for output files */

   assert (mode == READ_ACCESS || mode == WRITE_ACCESS);

#ifdef WINDOWS
   read_mode  = "rb";
   write_mode = "wb";
#else /* not WINDOWS  */
   read_mode  = "r";
   write_mode = "w";
#endif /* not  WINDOWS  */

   /*
    *  First check for stdin or stdout
    */
   if (filename == NULL || streq (filename, "-"))
   {
      if (mode == READ_ACCESS)
	 return stdin;
      else 
	 return stdout;
   }
   
   /*
    *  Try to open 'readonly' file in the current directory
    */
   if (mode == READ_ACCESS && (fp = fopen (filename, read_mode)))
      return fp; 

   if (mode == WRITE_ACCESS && strchr (filename, '/')) /* contains path */
      return fopen (filename, write_mode);
   
   /*
    *  Get value of environment variable 'env_var', if set
    *  else use DEFAULT_PATH ("./")
    */
   if (env_var != NULL)
      env_path = getenv (env_var);
   if (env_path == NULL) 
      env_path = strdup (DEFAULT_PATH);
   else
      env_path = strdup (env_path);
   
   /*
    *  Try to open file in the directory given by the environment
    *  variable env_var - individual path components are separated by PATH_SEP 
    */
   path = strtok (env_path, PATH_SEP);
   do 
   {
      if (ext_filename) 
	 fiasco_free (ext_filename);
      ext_filename =  fiasco_calloc (strlen (path) + strlen (filename) + 2,
			      sizeof (char));
      strcpy (ext_filename, path); 
      if (*(ext_filename + strlen (ext_filename) - 1) != '/')
	 strcat (ext_filename, "/");
      strcat (ext_filename, filename);
      fp = fopen (ext_filename, mode == READ_ACCESS ? read_mode : write_mode);
   }
   while (fp == NULL && (path = strtok (NULL, PATH_SEP)) != NULL);

   /*
    *  Try to open file in the FIASCO_SHARE directory
    */
   if (fp == NULL)
   {
      if (ext_filename) 
	 fiasco_free (ext_filename);
      ext_filename =  fiasco_calloc (strlen (FIASCO_SHARE) + strlen (filename) + 2,
			      sizeof (char));
      strcpy (ext_filename, FIASCO_SHARE); 
      if (*(ext_filename + strlen (ext_filename) - 1) != '/')
	 strcat (ext_filename, "/");
      strcat (ext_filename, filename);
      fp = fopen (ext_filename, mode == READ_ACCESS ? read_mode : write_mode);
   }
   fiasco_free (env_path);
   
   return fp;
}

bitfile_t *
open_bitfile (const char *filename, const char *env_var, openmode_e mode)
/*
 *  Bitfile constructor:
 *  Try to open file 'filename' for buffered bit oriented access with mode
 *  'mode'. Scan the current directory first and then cycle through the path
 *  given in the environment variable 'env_var', if set.
 *
 *  Return value:
 *	Pointer to open bitfile on success,
 *      otherwise the program is terminated.
 */
{
   bitfile_t *bitfile = fiasco_calloc (1, sizeof (bitfile_t));
   
   bitfile->file = open_file (filename, env_var, mode);

   if (bitfile->file == NULL)
      file_error (filename);

   if (mode == READ_ACCESS)
   {
      bitfile->bytepos  = 0;
      bitfile->bitpos   = 0;
      bitfile->mode     = mode;
      bitfile->filename = filename ? strdup (filename) : strdup ("(stdin)");
   }
   else if (mode == WRITE_ACCESS)
   {
      bitfile->bytepos  = BUFFER_SIZE - 1;
      bitfile->bitpos   = 8;
      bitfile->mode     = mode;
      bitfile->filename = filename ? strdup (filename) : strdup ("(stdout)");
   }
   else
      error ("Unknow file access mode '%d'.", mode);
   
   bitfile->bits_processed = 0;
   bitfile->buffer         = fiasco_calloc (BUFFER_SIZE, sizeof (byte_t));
   bitfile->ptr            = bitfile->buffer;

   return bitfile;
}

bool_t
get_bit (bitfile_t *bitfile)
/*
 *  Get one bit from the given stream 'bitfile'.
 *
 *  Return value:
 *	 1	H bit
 *	 0	L bit
 *
 *  Side effects:
 *	Buffer of 'bitfile' is modified accordingly.
 */
{
   assert (bitfile);
   
   if (!bitfile->bitpos--)		/* use next byte ? */
   {
      bitfile->ptr++;
      if (!bitfile->bytepos--)		/* no more bytes left in the buffer? */
      {
	 /*
	  *  Fill buffer with new data
	  */
	 int bytes = fread (bitfile->buffer, sizeof (byte_t),
			    BUFFER_SIZE, bitfile->file) - 1;
	 if (bytes < 0)			/* Error or EOF */
	    error ("Can't read next bit from bitfile %s.", bitfile->filename);
	 else
	    bitfile->bytepos = bytes;

	 bitfile->ptr = bitfile->buffer;
      }
      bitfile->bitpos = 7;
   }

   bitfile->bits_processed++;

   return *bitfile->ptr & mask [bitfile->bitpos] ? 1 : 0;
}

unsigned int
get_bits (bitfile_t *bitfile, unsigned bits)
/*
 *  Get #'bits' bits from the given stream 'bitfile'.
 *
 *  Return value:
 *	composed integer value
 *
 *  Side effects:
 *	Buffer of 'bitfile' is modified.
 */
{
   unsigned value = 0;			/* input value */
   
   while (bits--)
      value = (unsigned) (value << 1) | get_bit (bitfile);

   return value;
}

void
put_bit (bitfile_t *bitfile, unsigned value)
/*     
 *  Put the bit 'value' to the bitfile buffer.
 *  The buffer is written to the file 'bitfile->file' if the number of
 *  buffer bytes exceeds 'BUFFER_SIZE'.
 *
 *  No return value.
 *
 *  Side effects:
 *	Buffer of 'bitfile' is modified.
 */
{
   assert (bitfile);
   
   if (!bitfile->bitpos--)		/* use next byte ? */
   {
      bitfile->ptr++;
      if (!bitfile->bytepos--)		/* no more bytes left ? */
      {
	 /*
	  *  Write buffer to disk and fill buffer with zeros
	  */
	 if (fwrite (bitfile->buffer, sizeof (byte_t),
		     BUFFER_SIZE, bitfile->file) != BUFFER_SIZE)
	    error ("Can't write next bit of bitfile %s!", bitfile->filename);
	 memset (bitfile->buffer, 0, BUFFER_SIZE);
	 bitfile->bytepos = BUFFER_SIZE - 1;
	 bitfile->ptr     = bitfile->buffer;
      }
      bitfile->bitpos = 7;
   }
   
   if (value)
      *bitfile->ptr |= mask [bitfile->bitpos];

   bitfile->bits_processed++;
}

void
put_bits (bitfile_t *bitfile, unsigned value, unsigned bits)
/*     
 *  Put #'bits' bits of integer 'value' to the bitfile buffer 'bitfile'.
 *
 *  No return value.
 *
 *  Side effects:
 *	Buffer of 'bitfile' is modified.
 */
{
   while (bits--)
      put_bit (bitfile, value & mask [bits]);
}

void
close_bitfile (bitfile_t *bitfile)
/*
 *  Bitfile destructor:
 *  Close 'bitfile', if 'bitfile->mode' == WRITE_ACCESS write bit buffer
 *  to disk. 
 *
 *  No return value.
 *
 *  Side effects:
 *	Structure 'bitfile' is discarded.
 */
{
   assert (bitfile);
   
   if (bitfile->mode == WRITE_ACCESS)
   {
      unsigned bytes = fwrite (bitfile->buffer, sizeof (byte_t),
			       BUFFER_SIZE - bitfile->bytepos, bitfile->file);
      if (bytes != BUFFER_SIZE - bitfile->bytepos)
	 error ("Can't write remaining %d bytes of bitfile "
		"(only %d bytes written)!",
		BUFFER_SIZE - bitfile->bytepos, bytes);
   }
   fclose (bitfile->file);
   fiasco_free (bitfile->buffer);
   fiasco_free (bitfile->filename);
   fiasco_free (bitfile);
}

unsigned
bits_processed (const bitfile_t *bitfile)
/*
 *  Return value:
 *	Number of bits processed up to now
 */
{
   return bitfile->bits_processed;
}
