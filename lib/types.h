/*
 *  types.h
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

#ifndef _TYPES_H
#define _TYPES_H

#undef FALSE
#undef NO
#undef TRUE
#undef YES

enum boolean { NO = 0, FALSE = 0, YES = 1, TRUE = 1};

typedef float			real_t;
typedef enum boolean		bool_t;
typedef unsigned char           byte_t;
typedef short                   word_t;
typedef unsigned short          u_word_t;
typedef struct pair
{
   word_t key;
   word_t value;
} pair_t;

#endif /* not _TYPES_H */

