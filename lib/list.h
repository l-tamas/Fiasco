/*
 *  list.h
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

#ifndef _LIST_H
#define _LIST_H

#include <stdio.h>
#include <stddef.h>

typedef struct node
{
   struct node *prev;			/* pointer to prev list element */
   struct node *next;			/* pointer to next list element */
   void	       *value;			/* pointer to value of node */
} node_t;

typedef struct list
{
   node_t *head;
   node_t *tail;
   size_t  size_of_element;		/* number of bytes to store value */
} list_t;

typedef enum {TAIL, HEAD} pos_e;

/*
 *  Alias definitions for queue and stack
 */

typedef list_t lqueue_t ;
#define alloc_queue		alloc_list
#define free_queue		free_list		
#define queue_append(q, d)	(list_insert ((q), TAIL, (d)))
#define queue_remove(q, d)	(list_remove ((q), HEAD, (d)))

typedef list_t lstack_t ;
#define alloc_stack		alloc_list
#define free_stack		free_list
#define stack_push(q, d)	(list_insert ((q), TAIL, (d)))
#define stack_pop(q, d)		(list_remove ((q), TAIL, (d)))

list_t *
alloc_list (size_t size_of_element);
void 
free_list (list_t *list);
bool_t
list_element_n (const list_t *list, pos_e pos, unsigned n, void *data);
void
list_foreach (const list_t *list, void (*function)(void *, void *),
	      void *data);
void
list_insert (list_t *list, pos_e pos, const void *data);
bool_t
list_remove (list_t *list, pos_e pos, void *data);
unsigned
list_sizeof (const list_t *list);

#endif /* not _LIST_H */

