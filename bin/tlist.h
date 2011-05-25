/*      
 *  tlist.h
 *
 *  Written by:         Martin Grimm
 *
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */
 
/*
 *  $Date: 2000/07/15 17:20:59 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#ifndef _TLIST_H
#define _TLIST_H

typedef struct tlist
{
   int		value; /* the "main" value of the list entry */
   int		count; /* used to count the number of list->value
			  entered in this list */
   struct tlist *next;
} tlist_t;

void
InsertAscList (tlist_t **head, int value, int count);
void
InsertDesList (tlist_t **head, int value, int count);
int
SearchAscList (const tlist_t *SearchList, int value);
int
SearchDesList (const tlist_t *SearchList, int value);
int
CountListEntries (const tlist_t *CountList);
tlist_t *
String2List (const char *str_list);
void
RemoveList (tlist_t *ListHead);

#endif /* not _TLIST_H */
