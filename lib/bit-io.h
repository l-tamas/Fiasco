/*
 *  bit-io.h
 *
 *  Written by:		Ullrich Hafner
 *
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:49:37 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#ifndef _BIT_IO_H
#define _BIT_IO_H

#include <stdio.h>
#include "types.h"

#define OUTPUT_BYTE_ALIGN(bfile) while ((bfile)->bitpos) put_bit (bfile, 0);
#define INPUT_BYTE_ALIGN(bfile)  while ((bfile)->bitpos) get_bit (bfile);

typedef enum {READ_ACCESS, WRITE_ACCESS} openmode_e;

typedef struct bitfile
{
   FILE	      *file;			/* associated filepointer */
   char	      *filename;		/* corresponding filename */
   byte_t     *buffer;			/* stream buffer */
   byte_t     *ptr;			/* pointer to current buffer pos */
   unsigned    bytepos;			/* current I/O byte */
   unsigned    bitpos;			/* current I/O bit */
   unsigned    bits_processed;		/* number of bits already processed */
   openmode_e  mode;			/* access mode */
} bitfile_t;

FILE *
open_file (const char *filename, const char *env_var, openmode_e mode);
bitfile_t *
open_bitfile (const char *filename, const char *env_var, openmode_e mode);
void
put_bit (bitfile_t *bitfile, unsigned value);
void
put_bits (bitfile_t *bitfile, unsigned value, unsigned bits);
bool_t
get_bit (bitfile_t *bitfile);
unsigned 
get_bits (bitfile_t *bitfile, unsigned bits);
void
close_bitfile (bitfile_t *bitfile);
unsigned
bits_processed (const bitfile_t *bitfile);

#endif /* not _BIT_IO_H */

