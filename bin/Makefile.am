## Process this file with automake to produce Makefile.in
##
## Makefile.am:	WFA binaries			uh, Fri Aug  2 1996
##
## This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
## Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
##
## $Date: 2000/07/18 19:41:57 $
## $Author: hafner $
## $Revision: 5.3 $
##

bin_PROGRAMS	      = @AFIASCO@ bfiasco cfiasco dfiasco efiasco pnmpsnr 
EXTRA_PROGRAMS        = afiasco

afiasco_SOURCES       = background.c callbacks.c dialog.c drawing.c icons.c \
		        view.c xwfa.c 
afiasco_LDADD         = ../codec/libfiasco.la $(GTK_LIBS)
afiasco_DEPENDENCIES  = ../codec/libfiasco.la
afiasco_LDFLAGS       = -static

bfiasco_SOURCES       = binerror.c fig.c  getopt.c getopt1.c lctree.c \
			params.c tlist.c twfa.c
bfiasco_LDADD         = ../codec/libfiasco.la
bfiasco_DEPENDENCIES  = ../codec/libfiasco.la
bfiasco_LDFLAGS       = -static

cfiasco_SOURCES       = binerror.c cwfa.c getopt.c getopt1.c params.c
cfiasco_LDADD         = ../codec/libfiasco.la
cfiasco_DEPENDENCIES  = ../codec/libfiasco.la
cfiasco_LDFLAGS       = -static

dfiasco_SOURCES       = binerror.c dwfa.c getopt.c getopt1.c params.c \
			@DFIASCO_EXTRA_SOURCES@
dfiasco_LDADD         = @DFIASCO_EXTRA_LIBS@ ../codec/libfiasco.la @X_LIBS@ \
			@X_PRE_LIBS@ @XEXT_LIB@ @LIB_X11@ @X_EXTRA_LIBS@
dfiasco_DEPENDENCIES  = @DFIASCO_EXTRA_LIBS@ ../codec/libfiasco.la
dfiasco_LDFLAGS       = -static

efiasco_SOURCES       = binerror.c fiasco2fiasco.c getopt.c getopt1.c params.c
efiasco_LDADD         = ../codec/libfiasco.la 
efiasco_DEPENDENCIES  = ../codec/libfiasco.la
efiasco_LDFLAGS       = -static

pnmpsnr_SOURCES       = pnmpsnr.c binerror.c
pnmpsnr_LDADD         = ../codec/libfiasco.la 
pnmpsnr_DEPENDENCIES  = ../codec/libfiasco.la
pnmpsnr_LDFLAGS       = -static

noinst_HEADERS	      = buttons.h display.h getopt.h params.h binerror.h \
			background.h callbacks.h dialog.h drawing.h icons.h \
		        view.h fig.h tlist.h twfa.h lctree.h ttypes.h xwfa.h
EXTRA_dfiasco_SOURCES = buttons.c display.c
EXTRA_DIST	      = MANIFEST
INCLUDES	      = @INCLUDES@ $(GTK_CFLAGS) @X_CFLAGS@

