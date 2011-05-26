/*      
 *  ttypes.h
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

#ifndef _TTYPES_H
#define _TTYPES_H

#include "types.h"
#include "wfa.h"

#define DEFAULT            -1 /* default value for xfig-files */

/*
 * coordinates
 */

#define X_SHEET         14000 /* width and height of a DIN A4 sheet */
#define Y_SHEET          9900

#define X_TREE_MARGIN    1500 /* margins between sheet-border and tree */
#define Y_TREE_MARGIN     500
#define X_BASIS_MARGIN     500 /* x-margin between basis and left sheet-border */
#define Y_BASIS_MARGIN     300 /* y-margin between basis and upper
				 sheet-border/tree/LC (depends on what shown) */
#define Y_LC_MARGIN       500 /* y-margin between tree and most upper LC */
#define X_GRID_MARGIN     500 /* x-margin between grid and left sheet-border/tree
				 (depends on what shown) */
#define Y_GRID_MARGIN     500 /* y-margin between grid and upper sheet-border */
#define X_LEGEND_MARGIN   400 /* x-margin between legend and left sheet-border */
#define Y_LEGEND_MARGIN   500 /* y-margin between legend and upper
				 sheet-border/tree/LC (depends on what shown) */

#define X_STATE_DISTANCE  180 /* x-distance between the centers of two states
				 in lwr-order */
#define Y_STATE_DISTANCE  600 /* y-distance between the levels in the tree */

#define Y_BASIS_DISTANCE   500 /* y-distance between the center of the states
				 in the basis */
#define Y_LC_DISTANCE      30 /* y-distance between two LC-lines */
#define Y_LEGEND_DISTANCE 150 /* height of one text-line in the legend */
#define Y_GRID_DISTANCE   300 /* y-distance between two grids
				 (used with color images) */

#define LC_RADIUS          40 /* the width and height of the line joining
				 horizontal and vertical LC-lines */

#define STATE_RADIUS       75 /* the radius of a state-circle */
#define MULTISTATE_HEIGHT 200 /* the height/width of a multistate-triangle */
#define MULTISTATE_WIDTH  150

#define SHADOW_OFFSET      10 /* the x/y-offset betweenobject and  shadow */

#define X_GRID_SCALE       10 /* scaling of the grid, a factor of 15 means
				 1:1 in xfig with a zoom factor of 1 */
#define Y_GRID_SCALE       10

/*
 * line widths
 */

#define MAX_COUNT_1 2 /* defines, how many LC-lines are allowed in
			 each MAX_COUNT-group */
#define MAX_COUNT_2 4
#define MAX_COUNT_3 8

#define COUNT_1_THICK 1 /* defines the thickness (in 1/80 inch) of the lines */
#define COUNT_2_THICK 2 /* of the corresponding MAX_COUNT-group. */
#define COUNT_3_THICK 3
#define COUNT_4_THICK 4 /* the thickness of lines that are too large
			   for any group */

/*
 * levels
 */

#define TEXT_LEVEL    100 /* levels, where the different objects are drawn on */ 
#define STATE_LEVEL   200 /* levels are from 0 to 999, 0 means the most top
			     level */
#define LC_TREE_LEVEL 250
#define LC_BASIS_LEVEL 260
#define EDGE_LEVEL    300
#define LEVEL_LEVEL   400
#define GRID_LEVEL    100
#define LEGEND_LEVEL  100

/*
 * colors
 */

#define BLACK   0 /* predefined colors in x-fig */
#define BLUE    1
#define GREEN   2
#define CYAN    3
#define RED     4
#define MAGENTA 5
#define YELLOW  6
#define WHITE   7

/* colors of the linear combination lines to tree/basis */
#define LEFT_LC_TREE_COLOR BLACK
#define RIGHT_LC_TREE_COLOR BLACK

#define LEFT_LC_BASIS_COLOR BLACK
#define RIGHT_LC_BASIS_COLOR BLACK

/* #define LEFT_LC_TREE_COLOR CYAN */
/* #define RIGHT_LC_TREE_COLOR BLUE */

/* #define LEFT_LC_BASIS_COLOR MAGENTA */
/* #define RIGHT_LC_BASIS_COLOR RED */

/* colors of the linear combination lines to tree/basis */
#define LEFT_LC_TREE_STYLE SOLID
#define RIGHT_LC_TREE_STYLE SOLID

#define LEFT_LC_BASIS_STYLE SOLID
#define RIGHT_LC_BASIS_STYLE SOLID

/* border/fill-colors to use for different objects */
#define LEVEL_COLOR GREEN
#define TEXT_COLOR BLACK
#define TREE_COLOR BLACK

#define SHADOW_COLOR BLACK

#define STATE_COLOR BLACK
#define STATE_FILL_COLOR WHITE

#define MV_STATE_COLOR BLACK
#define MV_STATE_FILL_COLOR WHITE
#define LEFT_MV_COLOR RED    /* the arrows shown above a mv-state */
#define RIGHT_MV_COLOR GREEN

#define MULTISTATE_COLOR BLACK
#define MULTISTATE_FILL_COLOR WHITE

#define LEGEND_FRAME_COLOR BLACK
#define LEGEND_FILL_COLOR WHITE

#define GRID_LINE_COLOR BLACK
#define GRID_FILL_COLOR WHITE /* color for parts not shown in the tree */
#define GRID_SELECTED_COLOR RED /* if no colorization, colorize shown parts
				   in this color */

#define MAX_COLOR_DEPTH 4 /* if colorization,
			     split colors up to this depth      */
                         /* you need to define MAXLABELS^MAX_COLOR_DEEP colors */
                         /* in GRID_COLOR(x) to use this correctly             */
#define GRID_COLOR(x) ((x==0)?   1 :\
		       (x==1)?   2 :\
		       (x==2)?   3 :\
		       (x==3)?   4 :\
		       (x==4)?   5 :\
		       (x==5)?   6 :\
		       (x==6)?   8 :\
		       (x==7)?   9 :\
		       (x==8)?  10 :\
		       (x==9)?  11 :\
		       (x==10)? 12 :\
		       (x==11)? 13 :\
		       (x==12)? 14 :\
		       (x==13)? 15 :\
		       (x==14)? 16 :\
		                17 )

/*
 * line styles
 */

#define SOLID		0
#define DASHED		1
#define DOTTED		2
#define DASH_DOTTED	3
#define DASH_2_DOTTED	4
#define DASH_3_DOTTED	5

/* struct for storing the necessary values of a state */
typedef struct state
{
   int state_nr; /* state-number in lrw-order */
   int depth;     /* depth of state (0 means root) */
   int x;        /* coordinates of the state in the xfig-drawing */
   int y;
   int child[MAXLABELS]; /* childs of the state */
   int mc[MAXLABELS];    /* motion compensation of the state */
   int multistate;       /* if this state stands for a group of states,
			    this means the state with the lowest number
			    (lrw-order), else RANGE */
   tlist_t *tree_out[MAXLABELS]; /* list of linear combinations to a tree state */
                                /* list->value contains the number of the
				   target state */
                                /* list->count contains the number of lines to
				   the target state */
   tlist_t *basis_out[MAXLABELS]; /* list of linear combinations to a basis state */
                                /* list->value contains the number of the
				   target state */
                                /* list->count contains the number of lines to
				   the target state */
   tlist_t *tree_in[MAXLABELS];  /* list of linear combinations in this state */
                                /* list->value contains the y-coordinate of
				   the left-line, if tree state contains the
				   x-coordinate of the down-line, if basis state 
				   list->count contains the number of lines to
				   the target state */
   int tree_out_nr[MAXLABELS];  /* number of all list->count in tree_out */
   int basis_out_nr[MAXLABELS];  /* number of all list->count in basis_out */
   int tree_in_nr[MAXLABELS];   /* number of all list->count in tree_in */
} state_t;

/* the tree-structure for drawing the xfig-tree */
/* the nodes are stored in lwr-order */
typedef struct LCTree
{
   int root_state;      /* root-state of the tree */
   tlist_t *root_origin; /* list of the nodes from the original root
			   of the wfa-tree to the root of this tree
			   states are numbered in lrw-order (wfa) */
   state_t *states;     /* array of states */
   int nr_of_states;    /* number of the states in tree */
   int basis_states;    /* number of basis states */
} LCTree_t;

/* }}} */

#endif /* not _TTYPES_H */
