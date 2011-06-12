#
# This file is part of DGD, http://dgd-osr.sourceforge.net/
# Copyright (C) 1993-2010 Dworkin B.V.
# Copyright (C) 2010 DGD Authors (see the file Changelog for details)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
HOST=	DARWIN
DEFINES=-D$(HOST)
DEBUG=	-O -g
CCFLAGS=$(DEFINES) $(DEBUG)
CFLAGS=	-I. -I.. -I../comp $(CCFLAGS)
LIBS=
LINTFLAGS=-abcehpruz
CC=	gcc
DMAKE=	make
YACC=	yacc
PRECOMP=../comp/a.out
CONFIG=/home/xyllomer/etc/lpmud
MUDLIB=/home/xyllomer/lib/xyllomer
#PREFILES:=precomp

#SRC=	driver.c auto.c objregd.c rsrcd.c accessd.c userd.c \
#	api_objreg.c api_rsrc.c api_access.c api_user.c api_tls.c \
#	lib_connection.c lib_user.c lib_wiztool.c \
#	rsrc.c telnet.c binary.c user.c wiztool.c
SRC=

#OBJ=	driver.o auto.o objregd.o rsrcd.o accessd.o userd.o \
#	api_objreg.o api_rsrc.o api_access.o api_user.o api_tls.o \
#	lib_connection.o lib_user.o lib_wiztool.o \
#	rsrc.o telnet.o binary.o user.o wiztool.o
OBJ=

dgd: $(OBJ) $(PRECOMP) precompile lpc.o
	
lint:
	lint $(LINTFLAGS) $(CFLAGS) lpc.c

precompile:
	rm -rf c/*;rm -f dgd;rm -f list;for xx in `cat precomp`; do mkdir -p c/$$xx;$(PRECOMP) $(CONFIG) $$xx c/$$xx/precompile.c ;echo \#include \"c/$$xx/precompile.c\" >> list;echo lpc/c/$$xx/precompile.o >> dgd;echo $$xx;$(CC) -c $(CPPFLAGS) $(CFLAGS) -o c/$$xx/precompile.o c/$$xx/precompile.c;  done;echo "lpc/lpc.o" >>dgd;
	
#$(SRC):	$(PRECOMP)

$(PRECOMP):
	cd ../comp; $(DMAKE) 'DMAKE=$(DMAKE)' 'CC=$(CC)' 'CCFLAGS=$(CCFLAGS)' \
			     'YACC=$(YACC)' 'LIBS=$(LIBS)' a.out

clean:
	rm -f dgd $(SRC) $(OBJ) lpc.o


$(OBJ) lpc.o: ../dgd.h ../config.h ../host.h ../error.h ../alloc.h ../str.h
$(OBJ) lpc.o: ../array.h ../object.h ../xfloat.h ../interpret.h ../data.h

$(OBJ) lpc.o: lpc_ext.h

lpc.o:	lpc.c list $(OBJ)
