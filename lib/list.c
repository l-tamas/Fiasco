/*
 *  list.c:		List operations	
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

#include "config.h"

#if HAVE_STRING_H
#	include <string.h>
#else /* not HAVE_STRING_H */
#	include <strings.h>
#endif /* not HAVE_STRING_H */

#include "types.h"
#include "macros.h"
#include "error.h"

#include "misc.h"
#include "list.h"

/*****************************************************************************

				public code
  
*****************************************************************************/

list_t *
alloc_list (size_t size_of_element)
/*
 *  List constructor:
 *  Allocate a new list.
 *  Size of list element values is given by 'size_of_element'.
 *
 *  Return value:
 *	pointer to an empty list
 */
{
   list_t *new_list = Calloc (1, sizeof (list_t));

   assert (size_of_element > 0);

   new_list->head 	     = NULL;
   new_list->tail 	     = NULL;
   new_list->size_of_element = size_of_element;

   return new_list;
}

void
free_list (list_t *list)
/*
 *  List destructor:
 *  Discard list and its elements.
 *
 *  No return value.
 *
 *  Side effects:
 *	struct 'list' is discarded
 */
{
   assert (list);
   
   while (list_remove (list, HEAD, NULL))
      ;
   Free (list);
}

void
list_insert (list_t *list, pos_e pos, const void *data)
/*
 *  Insert a new 'list' element at head ('pos' = HEAD) or
 *  tail ('pos' = TAIL) of 'list'. 
 *  'data' is a pointer to a memory segment of size
 *  'list'->size_of_element containing the value to store.
 *  The value is directly copied - no references are stored.
 *
 *  No return value.
 *
 *  Side effects:
 *	lists current tail or head is replaced by the new element
 */
{
   node_t *element;

   assert (list && data);

   element 	  = Calloc (1, sizeof (node_t));
   element->value = Calloc (1, list->size_of_element);
   memcpy (element->value, data, list->size_of_element);

   if (pos == TAIL)
   {
      element->next = NULL;
      element->prev = list->tail;
      if (list->tail)
	 list->tail->next = element;
      list->tail = element;
      if (!list->head)
	 list->head = element;
   }
   else					/* pos == HEAD */
   {
      element->prev = NULL;
      element->next = list->head;
      if (list->head)
	 list->head->prev = element;
      list->head = element;
      if (!list->tail)
	 list->tail = element;
   }
}

bool_t
list_remove (list_t *list, pos_e pos, void *data)
/*
 *  Remove 'list' element from head or tail of 'list'.
 *
 *  Return value:
 *	TRUE on success,
 *	FALSE if list is empty or
 *	      if list value data is NULL
 *
 *  Side effects:
 *	lists current head or tail is removed
 *	value of the removed list element (if not NULL) is copied to
 *      'data' (if 'data' is not NULL)
 */
{
   node_t *element;
   void	  *valueptr;

   assert (list);
   
   if (pos == TAIL)
   {
      element = list->tail;
      if (element)
      {
	 list->tail = element->prev;
	 valueptr   = element->value;
	 Free (element);
      }
      else
	 valueptr = NULL;
      if (!list->tail)			/* 'element' was last node */
	 list->head = NULL;
   }
   else					/* pos == HEAD */
   {
      element = list->head;
      if (element)
      {
	 list->head = element->next;
	 valueptr   = element->value;
	 Free (element);
      }
      else
	 valueptr = NULL;
      if (!list->head)			/* 'element' was last node */
	 list->tail = NULL;
   }

   if (valueptr)			/* copy value of node */
   {
      if (data)				
	 memcpy (data, valueptr, list->size_of_element);
      Free (valueptr);
   }
   
   return valueptr ? TRUE : FALSE;
}

bool_t
list_element_n (const list_t *list, pos_e pos, unsigned n, void *data)
/*
 *  Get value of 'list' element number 'n'.
 *  (First element is list head if 'pos' == HEAD
 *                 or list tail if 'pos' == TAIL.
 *   Accordingly, traverse the list in ascending or descending order).
 *  
 *  Return value:
 *	TRUE on success, FALSE if there is no element 'n'
 *
 *  Side effects:
 *	value of list element 'n' is copied to 'data' 
 */
{
   node_t *element;

   assert (list && data);
   
   if (pos == HEAD)
      for (element = list->head; element != NULL && n;
	   element = element->next, n--)
	 ;
   else
      for (element = list->tail; element != NULL && n;
	   element = element->prev, n--)
	 ;
      
   if (element)
   {
      memcpy (data, element->value, list->size_of_element);
      return TRUE;
   }
   else
      return FALSE;
}

unsigned
list_sizeof (const list_t *list)
/*
 *  Count number of 'list' elements.
 *
 *  Return value:
 *	number of 'list' elements.
 */
{
   node_t   *element;
   unsigned  n = 0;

   assert (list);
   
   for (element = list->head; element != NULL; element = element->next)
      n++;

   return n;
}

void
list_foreach (const list_t *list, void (*function)(void *, void *), void *data)
/*
 *  Call 'function' for each element of the 'list'.
 *  Parameters given to 'function' are a pointer to the value of the
 *  current 'list' element and the user pointer 'data'.
 *
 *  No return value.
 */
{
   node_t *element;

   assert (list && function && data);
   
   for (element = list->head; element; element = element->next)
      function (element->value, data);
}

