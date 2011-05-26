## Process this file with automake to produce Makefile.in
##
## Makefile.am: WFA input routines 
##
## This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
## Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
##
## $Date: 2000/06/14 20:50:13 $
## $Author: hafner $
## $Revision: 5.1 $
##

noinst_LTLIBRARIES         = libfiasco-input.la
libfiasco_input_la_SOURCES = basis.c matrices.c mc.c nd.c read.c tree.c \
			     weights.c
noinst_HEADERS	           = basis.h matrices.h mc.h nd.h read.h tree.h \
			     weights.h
EXTRA_DIST	     	   = MANIFEST basis.c.in
CLEANFILES		   = basis.c
INCLUDES	           = @INCLUDES@

basis.c: basis.c.in
	cp basis.c.in basis.c
