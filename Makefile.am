## Process this file with automake to produce Makefile.in
##
## Makefile.am:	Main package			uh, Fri Aug  2 1996
##
## This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
## Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
##
## $Date: 2000/06/14 20:48:52 $
## $Author: hafner $
## $Revision: 5.1 $
##

ACLOCAL_AMFLAGS  = -I /usr/local/share/aclocal

SUBDIRS = data doc lib input output codec bin

EXTRA_DIST   = MANIFEST fiasco.spec system.fiascorc		
pkgdata_DATA = system.fiascorc
include_HEADERS = fiasco.h


LIBTOOL_DEPS = @LIBTOOL_DEPS@

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck