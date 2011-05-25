/*
 *  params.h
 *
 *  Written by:		Stefan Frank
 *			Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:51:17 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#ifndef PARAMS_H
#define PARAMS_H

#include <stdio.h>

typedef union pdata_t			/* Allow for different */
{					/* parameter types. */
   int    b;
   int	  i;
   float  f;
   char	 *s;
} pdata_t;

typedef enum {PFLAG = 1, PINT, PFLOAT, PSTR, POSTR} param_e;

typedef struct param_t
{
   char	   *name;			/* Parameter name */
   char	   *argument_name;		/* Argument name */
   char	    optchar;			/* Corresponding command line switch */
   param_e  type;			/* Parameter type */
   pdata_t  value;			/* Parameter value */
   char	   *default_value;		/* Parameters default value */
   char	   *use;			/* One line usage. Must contain %s,
					   which will be replaced by 'name'. */
} param_t;

int
parseargs (param_t *usr_params, int argc, char **argv, const char *synopsis,
	   const char *comment, const char *non_opt_string, const char *path,
	   const char *sys_file_name, const char *usr_file_name);
void
write_parameters (const param_t *params, FILE *output);
void
ask_and_set (param_t *params, const char *name, const char *msg);
void *
parameter_value (const param_t *params, const char *name);

#endif /* not PARAMS_H */
