/*      
 *  lctree.h
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

#ifndef _LCTREE_H
#define _LCTREE_H

#include "twfa.h"
#include "ttypes.h"
#include "wfa.h"

void
Init_LCTree (const wfa_t *wfa, LCTree_t *LCTree, toptions_t *options, int* lrw_to_lwr);
void
Build_LCTree (const wfa_t *wfa, LCTree_t *LCTree, toptions_t *options, int local_root,
	      int *lrw_to_lwr, int depth);
void
Remove_LCTree (LCTree_t *LCTree);
int
CalcLCTreeDepth (LCTree_t *LCTree, int local_root);
void
ReplaceTargetState (LCTree_t* LCTree, int local_root, int state, int motherstate);
void
JoinStates (LCTree_t *LCTree, int state, int motherstate);
void
CreateMultiState (LCTree_t *LCTree, int state);
void
RemoveInternalLC (LCTree_t* LCTree, int state);
void
RemoveLowerLC (LCTree_t* LCTree, int limit_state, int local_root);
void
AdjustLC (const wfa_t *wfa, LCTree_t *LCTree, toptions_t *options);
void
Depth_limit_LCTree (LCTree_t *LCTree, int local_root, int depth);
void
LC_limit_LCTree (LCTree_t *LCTree, int local_root);
void
CalcTreeCoordinates (LCTree_t *LCTree);
void
CalcBasisCoordinates (LCTree_t *LCTree, int y_offset);
void
GetOrigin (const wfa_t* wfa, tlist_t** state_list, int searchstate);
int
GetThick (int count);
void
DrawState (FILE *out, state_t *state, toptions_t *options, int color);
void
DrawSingleState (FILE *out, int x, int y, int state, toptions_t *options,
		 int color);
void
DrawMCState (FILE *out, int x, int y, int state, toptions_t *options, int mc1,
	     int mc2, int color);
void
DrawMultistate (FILE *out, int x, int y, int state1, int state2,
		toptions_t *options, int color);
void
DrawOrigin (FILE* out, int x, int y, tlist_t * list);
void
DrawLevels (FILE* out, LCTree_t* LCTree);
int
DrawLegend (FILE *out, const wfa_t *wfa, toptions_t *options, int frame_nr,
	    int color_image, int y_offset);
void
DrawGrid (FILE *out, const wfa_t *wfa, LCTree_t *LCTree, int color_image,
	  int *color_field, int legend_offset, toptions_t *options);
void
DrawGreyGrid (FILE *out, const wfa_t *wfa, int x_offset, int y_offset, int local_root,
	      int *color_field, toptions_t *options);
void
FillColor (int wfa_root, const wfa_t *wfa, int color, int *color_field);
void
SetColor (int wfa_root, int *lrw_to_lwr, const wfa_t *wfa, LCTree_t *LCTree, int color,
	  int depth, int *color_field);
void
GetColorField (const wfa_t *wfa, LCTree_t *LCTree, int *color_field, int *lrw_to_lwr);
int
DrawTree (FILE *outfile, LCTree_t *LCTree, toptions_t *options, int depth,
	  int *color_field);
void
DrawBasis (FILE* outfile, LCTree_t *LCTree, toptions_t *options);
int
DrawTreeLC (FILE* outfile, LCTree_t *LCTree, int depth);
void
DrawBasisLC (FILE* outfile, LCTree_t *LCTree);

#endif /* not _LCTREE_H */
