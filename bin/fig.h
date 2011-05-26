/*      
 *  fig.h
 *
 *  Written by:         Martin Grimm
 *
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */
 
/*
 *  $Date: 2000/07/15 17:20:59 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#ifndef _FIG_H
#define _FIG_H

#include <stdio.h>

void
xfig_box (FILE *out, int x, int y, int height, int width, int pencolor,
	  int fillcolor, int level);
void
xfig_centerbox (FILE *out, int x, int y, int x_offset, int y_offset, int pencolor,
		int fillcolor, int level);
void
xfig_line (FILE *out, int x1, int y1, int x2, int y2, int color, int linestyle,
	   int thickness, int level);
void
xfig_triangle (FILE *out, int x, int y, int height, int width, int pencolor,
	       int fillcolor, int level);
void
xfig_circle (FILE *out, int x, int y, int r, int pencolor, int fillcolor,
	     int level);
void
xfig_header (FILE *out);

#endif /* not _FIG_H */
