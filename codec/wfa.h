/*
 *  wfa.h
 *
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/18 15:44:57 $
 *  $Author: hafner $
 *  $Revision: 5.3 $
 *  $State: Exp $
 */

#ifndef _WFA_H
#define _WFA_H

#define MAXEDGES  5
#define MAXSTATES 6000
#define MAXLABELS 2			/* only bintree supported anymore */
#define MAXLEVEL  22 

#define FIASCO_BINFILE_RELEASE   2
#define FIASCO_MAGIC	         "FIASCO" /* FIASCO magic number */
#define FIASCO_BASIS_MAGIC       "Fiasco" /* FIASCO initial basis */

#define NO_EDGE		-1
#define RANGE		-1
#define NO_RANGE	 0

#define CHILD		 1
#define LEAF		 0

#define MAX_PROB	 9
#define MIN_PROB	 1

/*
 *  WFA state types:
 *	0:		 state is not allowed to be used in an
 *			 approximation and it's image is not needed
 *			 for ip computations.
 *	AUXILIARY_MASK:  state is required for computation of ip's but is not
 *			 allowed to be used in an approximation.
 *	USE_DOMAIN_MASK: state is allowed to be used in an approximation.
 */
enum state_types {AUXILIARY_MASK = 1 << 0, USE_DOMAIN_MASK = 1 << 1};

#define isedge(x)	   ((x) != NO_EDGE)
#define isdomain(x)	   ((x) != NO_EDGE)
#define isrange(x)	   ((x) == RANGE)
#define ischild(x)	   ((x) != RANGE)
#define isauxiliary(d,wfa) ((wfa)->domain_type[d] & AUXILIARY_MASK)
#define usedomain(d, wfa)  ((wfa)->domain_type[d] & USE_DOMAIN_MASK)
#define need_image(d,wfa)  (isauxiliary ((d), (wfa)) || usedomain ((d), (wfa)))

typedef enum mc_type {NONE, FORWARD, BACKWARD, INTERPOLATED} mc_type_e;
typedef enum frame_type {I_FRAME, P_FRAME, B_FRAME} frame_type_e;
typedef enum header {HEADER_END, HEADER_TITLE, HEADER_COMMENT} header_type_e;

typedef struct mv
/*
 *  Motion vector components
 */
{
   mc_type_e type;			/* motion compensation type */
   int       fx, fy;			/* forward vector coordinates */
   int       bx, by;			/* backward vector coordinates */
} mv_t;

typedef struct range_info
{
   unsigned x, y;			/* coordinates of upper left corner */
   unsigned level;			/* bintree level of range */
} range_info_t;

#include "image.h"
#include "rpf.h"
#include "bit-io.h"

typedef struct wfa_info
{
   char	    *wfa_name;			/* filename of the WFA */
   char	    *basis_name;		/* filename of the initial basis */
   char     *title;			/* title of FIASCO stream */
   char     *comment;			/* comment for FIASCO stream */
   
   unsigned  max_states;		/* max. cardinality of domain pool */
   unsigned  chroma_max_states;		/* max. cardinality of domain pool for
					   chroma band coding */
   bool_t    color;			/* color image */
   unsigned  width;			/* image width */
   unsigned  height;			/* image height */
   unsigned  level;			/* image level */
   rpf_t    *rpf;			/* Standard reduced precision format */
   rpf_t    *dc_rpf;			/* DC reduced precision format */
   rpf_t    *d_rpf;			/* Delta reduced precision format */
   rpf_t    *d_dc_rpf;			/* Delta DC reduced precision format */
   unsigned  frames;			/* number of frames in the video */
   unsigned  fps;			/* number of frames per second */
   unsigned  p_min_level;		/* min. level of prediction */
   unsigned  p_max_level;		/* max. level of prediction */
   unsigned  search_range;		/* motion vector interval */
   bool_t    half_pixel;		/* usage of half pixel precision */
   bool_t    cross_B_search;		/* usage of Cross-B-Search */
   bool_t    B_as_past_ref;		/* usage of B frames as ref's */
   unsigned  smoothing;			/* smoothing of image along borders */
   unsigned  release;			/* FIASCO file format release */
} wfa_info_t;

typedef struct wfa
/*
 *  Used to store all informations and data structures of a WFA
 */
{
   wfa_info_t	*wfainfo;		/* misc. information about the WFA */
   frame_type_e frame_type;		/* intra, predicted, bi-directional */
   unsigned	states;			/* number of states */
   unsigned	basis_states;		/* number of states in the basis */
   unsigned	root_state;		/* root of the tree */
   real_t	*final_distribution;    /* one pixel images */
   byte_t	*level_of_state;	/* level of the image part which is
					   represented by the current state */
   byte_t	*domain_type;		/* Bit_0==1: auxilliary state
					   Bit_1==1: used for Y compr */
   mv_t		(*mv_tree)[MAXLABELS];	/* motion vectors */
   word_t	(*tree)[MAXLABELS];	/* bintree partitioning */
   u_word_t	(*x)[MAXLABELS];	/* range coordinate */
   u_word_t	(*y)[MAXLABELS];	/* range coordinate */
   word_t	(*into)[MAXLABELS][MAXEDGES + 1];   /* domain references */
   real_t	(*weight)[MAXLABELS][MAXEDGES + 1]; /* lin.comb. coefficients */
   word_t	(*int_weight)[MAXLABELS][MAXEDGES + 1]; /* bin. representation */
   word_t	(*y_state)[MAXLABELS];	/* bintree of Y component */
   byte_t	(*y_column)[MAXLABELS];	/* array for Y component references */
   byte_t	(*prediction)[MAXLABELS]; /* DC prediction */
   bool_t	(*delta_state);		/* delta state */
} wfa_t;

#endif /* not _WFA_H */

