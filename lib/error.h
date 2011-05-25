/*
 *  error.h
 *  
 *  Written by:		Stefan Frank
 *			Ullrich Hafner
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

#ifndef _ERROR_H
#define _ERROR_H

void
set_error (const char *format, ...);
void
error (const char *format, ...);
void
file_error (const char *filename);
void
message (const char *format, ...);
void 
debug_message (const char *format, ...);
void
warning (const char *format, ...);
void 
info (const char *format, ...);
const char *
get_system_error (void);

#if HAVE_SETJMP_H
#	include <setjmp.h>
extern jmp_buf	env;
#endif /* HAVE_SETJMP_H */

#if HAVE_SETJMP_H
#	define try			if (setjmp (env) == 0)
#	define catch			else
#else /* not HAVE_SETJMP_H */
#	define try			if (TRUE)
#	define catch			else
#endif /* HAVE_SETJMP_H */

#if HAVE_ASSERT_H
#	include <assert.h>
#else /* not HAVE_ASSERT_H */
#	define assert(exp)	{if (!(exp)) error ("Assertion `" #exp " != NULL' failed.");}
#endif /* not HAVE_ASSERT_H */

#endif /* not _ERROR_H */

