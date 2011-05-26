## Process this file with automake to produce Makefile.in
##
## Makefile.am:	WFA codec			uh, Fri Aug  2 1996
##
## This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
## Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
##
## $Date: 2000/06/14 20:50:51 $
## $Author: hafner $
## $Revision: 5.1 $
##

lib_LTLIBRARIES	        = libfiasco.la
libfiasco_la_SOURCES	= approx.c bintree.c coder.c coeff.c control.c \
			  decoder.c dfiasco.c domain-pool.c ip.c \
			  motion.c mwfa.c \
			  options.c prediction.c subdivide.c tiling.c wfalib.c
libfiasco_la_LIBADD	= ../lib/libfiasco-lib.la \
			  ../input/libfiasco-input.la \
			  ../output/libfiasco-output.la
libfiasco_la_LDFLAGS	= -version-info 1:0:0
noinst_HEADERS		= approx.h bintree.h cwfa.h coder.h coeff.h control.h \
			  decoder.h dfiasco.h domain-pool.h ip.h \
			  motion.h mwfa.h options.h prediction.h subdivide.h \
			  tiling.h wfalib.h wfa.h
EXTRA_DIST		= MANIFEST
INCLUDES		= @INCLUDES@
