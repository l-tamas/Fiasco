## Process this file with automake to produce Makefile.in
##
## Makefile.am:	Compression library
##
## This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
## Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
##
## $Date: 2000/06/14 20:49:37 $
## $Author: hafner $
## $Revision: 5.1 $
##

noinst_LTLIBRARIES	 = libfiasco-lib.la
libfiasco_lib_la_SOURCES = arith.c bit-io.c dither.c error.c image.c \
			   list.c misc.c rpf.c
noinst_HEADERS	         = arith.h bit-io.h dither.h error.h image.h \
			   list.h macros.h misc.h rpf.h types.h
EXTRA_DIST		 = MANIFEST		
INCLUDES	         = @INCLUDES@