/*
 *  macros.h
 *
 *  Written by:		Ullrich Hafner
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

#ifndef _MACROS_H
#define _MACROS_H

#include <string.h>

/*******************************************************************************

			  System configuration section
  
*******************************************************************************/

#ifndef SEEK_CUR
#   define SEEK_CUR	1
#endif /* not SEEK_CUR */

#ifdef WIN32
#undef max
#undef min
#endif /* not WIN32 */

/*****************************************************************************

				Various macros
  
*****************************************************************************/

#define streq(str1, str2)	(strcmp ((str1), (str2)) == 0)
#define strneq(str1, str2)	(strcmp ((str1), (str2)) != 0)
#define square(x)		((x) * (x))
#define first_band(color)	((unsigned) ((color) ? Y  : GRAY))
#define last_band(color)        ((unsigned) ((color) ? Cr : GRAY))
#define width_of_level(l)	((unsigned) (1 << ((l) >> 1)))
#define height_of_level(l)	((unsigned) (1 << (((l) + 1) >> 1)))
#define size_of_level(l)	((unsigned) (1 << (l)))
#define address_of_level(l)	((unsigned) (size_of_level (l) - 1))
#define size_of_tree(l)		((unsigned) (address_of_level ((l) + 1)))
#define is_odd(n)		(abs (n) % 2)
#define max(a,b)		((a) < (b) ? (b) : (a))
#define min(a,b)	        ((a) > (b) ? (b) : (a))
#define _(x) (x) 


#define	MAXSTRLEN 1024
#define	MAXSTRLEN_SCANF "%1024s"

typedef enum color {GRAY = 0, Y = 0, Cb = 1, Cr = 2} color_e;

#endif /* _MACROS_H */



