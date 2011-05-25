/*      
 *  tlist.c:		Simple list
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


#include "config.h"

#include "types.h"
#include "macros.h"
#include "error.h"

#if STDC_HEADERS
#	include <stdlib.h>
#	include <string.h>
#else /* not STDC_HEADERS */
#	if HAVE_STRING_H
#		include <string.h>
#	else /* not HAVE_STRING_H */
#		include <strings.h>
#	endif /* not HAVE_STRING_H */
#endif /* not STDC_HEADERS */

#include <ctype.h>

#include "misc.h"
#include "tlist.h"

void
InsertAscList (tlist_t **head, int value, int count)
/*
 *  Insert value/count in ascending ordered list at (*head),
 *  if value is already in list, list->count is increased by count
 *
 *  head  - pointer on pointer at first list element
 *  value - value to be inserted
 *  count - "weight" of value to be inserted (or added)
 */
{
   tlist_t *pos      = *head;
   tlist_t **pointer = head;

   while (pos && pos->value < value) /* search insert position */
   {
      pointer = &pos->next;
      pos     = pos->next;
   }

   if (pos && pos->value == value) /* if entry exists, add count */
      pos->count += count;
   else                            /* else create new entry */
   {
      tlist_t *tmp  = Calloc (1, sizeof (tlist_t));
      
      tmp->value = value;
      tmp->count = count;
      tmp->next  = pos;
      *pointer 	 = tmp;
   }
}

void
InsertDesList (tlist_t **head, int value, int count)
/*
 *  Insert value/count in descending ordered list at (*head),
 *  if value is already in list, list->count is increased by count
 *
 *  head  - pointer on pointer at first list element
 *  value - value to be inserted
 *  count - "weight" of value to be inserted (or added)
 */
{
   tlist_t *pos      = *head;
   tlist_t **pointer = head;

   while (pos && pos->value > value) /* search insert position */
   {
      pointer = &pos->next;
      pos     = pos->next;
   }

   if (pos && pos->value == value) /* if entry exists, add count */
      pos->count += count;
   else                            /* else create new entry */
   {
      tlist_t *tmp  = Calloc (1, sizeof (tlist_t));

      tmp->value = value;
      tmp->count = count;
      tmp->next  = pos;
      *pointer 	 = tmp;
   }
   
}

int
SearchAscList (const tlist_t *SearchList, int value)
/*
 *  Search entry "value" in ascending ordered list at "SearchList",
 *  if found, return YES else FALSE.
 *
 *  SearchList - pointer on first list element
 *  value      - list entry to be searched for
 */
{
   const tlist_t *pos = SearchList;

   while (pos && pos->value < value)
      pos = pos->next;

   if (pos && pos->value == value)
      return YES;
   else
      return NO;
}

int
SearchDesList (const tlist_t *SearchList, int value)
/*
 *  Search entry "value" in descending ordered list at "SearchList",
 *  if found, return YES else FALSE.
 *
 *  SearchList - pointer on first list element
 *  value      - list entry to be searched for
 */
{
   const tlist_t *pos = SearchList;

   while (pos && pos->value > value)
      pos = pos->next;

   if (pos && pos->value == value)
      return YES;
   else
      return NO;
}

int
CountListEntries (const tlist_t *CountList)
/*
 *  Count number of entries in list at CountList, returns number of elements
 *
 *  CountList - pointer on first list element
 */
{
   const tlist_t *pos    = CountList;
   int		 counter = 0;

   while (pos)
   {
      counter++;
      pos = pos->next;
   }
   return counter;
}

tlist_t *
String2List (const char *str_list)
/*
 *  Convert a string to a ascending ordered list
 *  the string must be of the following syntax
 *  entry ::= an unsigned integer value
 *  field ::= an unsigned integer value + "-" + an unsigned integer value
 *  list  ::= entry | field
 *  list  ::= list + "," + list
 *
 *  str_list - string to convert
 *  return value: pointer to the head of the created list
 */
{
   char *pos1;
   char *pos2;
   char *input_string;
   
   int value1 = 0;
   int value2 = 0;
   
   tlist_t *my_list = NULL;
   
   if (!str_list || streq (str_list, ""))
      return NULL;

   input_string = strdup (str_list);
   
   pos1 = input_string;
   pos2 = input_string;

   if (!isdigit (*pos2)) /* string must start with a number */
      error("Parse error: digit expected!");

   while (*pos2)
   {
      while (*pos2 && isdigit (*pos2))
	 pos2++;
      
      if (*pos2 == '-') /* if field was found, read second value */
      {
	 int i;
	 
	 if (*pos2)
	 {
	    *pos2 = 0;
	    pos2++;
	 }
 	 value1 = strtol (pos1, NULL, 10);
	 pos1   = pos2;

	 if (!(*pos2) || !isdigit(*pos2)) /* a field must have a second value */
	    error("Parse error: digit expected!");

	 while (*pos2 && isdigit (*pos2))
	    pos2++;

	 if (*pos2)
	 {
	    if (*pos2 != ',')
	       error("List entries must be separated by ,");
	    *pos2 = 0;
	    pos2++;
	 }

	 value2 = strtol (pos1, NULL, 10);
	 pos1   = pos2;
	 
	 if (value1 > value2)
	 {
	    i      = value1;
	    value1 = value2;
	    value2 = i;
	 }
	 for(i = value1; i <= value2; i++) /* insert whole field in list */
	    InsertAscList (&my_list, i, 1);
      }
      else /* if no field was found */
      {
	 if (*pos2 && *pos2 != ',') /* check for correct separation character */
	    error ("List entries must be separated by ,");
	 
	 if (*pos2)
	 {
	    *pos2 = 0;
	    pos2++;
	 }
      
	 value1 = strtol (pos1, NULL, 10);
	 pos1   = pos2;
	 InsertAscList (&my_list, value1, 1); /* insert value in list */
      }
   }

   return my_list;
}

void
RemoveList (tlist_t *ListHead)
/*
 *  Remove the list from memory and free the memory
 *
 *  ListHead - pointer on list to remove
 *
 */
{
   if (ListHead)
   {
      if (ListHead->next)
	 RemoveList (ListHead->next);
      Free (ListHead);
   }
   else
      warning ("Can't free tlist <NULL>");
}

