#  fiasco.spec		
#
#  Written by:		Ullrich Hafner
#		
#  This file is part of FIASCO (Fractal Image And Sequence COdec)
#  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
#
#
#  $Date: 2000/10/22 10:46:33 $
#  $Author: hafner $
#  $Revision: 5.3 $
#  $State: Exp $

%define prefix  /usr
%define version 1.3
Name: fiasco
Version: %{version}
Release: 1
Summary: Fractal Image And Sequence COdec based on Weighted Finite Automata
Group: Applications/Graphics
Copyright: (c) 2000 Ullrich Hafner
Packager: Ullrich Hafner <hafner@bigfoot.de>
Vendor: NOP
Source: fiasco-%{version}.tar.gz
BuildRoot: /var/tmp/fiasco-root
Prefix: %{prefix}

%description
This is a image and video codec based on Weighted Finite
Automata. This codec is part of Ullrich Hafner's Ph.D. thesis
'Low Bit-Rate Image and Video Coding with Weighted Finite Automata',
University Wuerzburg, 1999.

%package devel
Summary: Fractal Image And Sequence COding library
Group: Applications/Graphics

%description devel
This is a image and video coding library based on Weighted Finite
Automata. This library is part of Ullrich Hafner's Ph.D. thesis
'Low Bit-Rate Image and Video Coding with Weighted Finite Automata',
University Wuerzburg, 1999.

%package analysis
Summary: Fractal Image And Sequence COding analysis tools
Group: Applications/Graphics

%description analysis
These are two analysis tools to visualize FIASCO streams. These
tools are part of Ullrich Hafner's Ph.D. thesis 'Low Bit-Rate Image
and Video Coding with Weighted Finite Automata', University Wuerzburg,
1999.

%prep
%setup

%build
CFLAGS=$RPM_OPT_FLAGS ./configure --prefix=%{prefix}
make

%install
make install-strip prefix=$RPM_BUILD_ROOT%{prefix}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{prefix}/bin/cfiasco
%{prefix}/bin/dfiasco
%{prefix}/bin/efiasco
%{prefix}/bin/pnmpsnr
%{prefix}/share/fiasco/*
%doc README INSTALL COPYING ChangeLog TODO AUTHORS 
%doc %{prefix}/man/man1/cfiasco.1
%doc %{prefix}/man/man1/dfiasco.1
%doc %{prefix}/man/man1/efiasco.1
%doc %{prefix}/man/man1/pnmpsnr.1

%files devel
%defattr(-,root,root)
%doc README INSTALL COPYING ChangeLog TODO AUTHORS doc/README.LIB 
%{prefix}/lib/*
%{prefix}/include/*
%doc %{prefix}/man/man3/*

%files analysis
%defattr(-,root,root)
%doc README INSTALL COPYING ChangeLog AUTHORS 
%{prefix}/bin/afiasco
%{prefix}/bin/bfiasco
%doc %{prefix}/man/man1/afiasco.1
%doc %{prefix}/man/man1/bfiasco.1
