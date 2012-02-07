# Makefile for:
#	coff2noff -- converts a normal MIPS executable into a Nachos executable
#
# This is a GNU Makefile.  It must be used with the GNU make program.
#
#  Use "make" to build the executable(s)
#  Use "make clean" to remove .o files
#  Use "make distclean" to remove all files produced by make, including
#     the executable
#
#
# Copyright (c) 1992-1996 The Regents of the University of California.
# All rights reserved.  See copyright.h for copyright notice and limitation 
# of liability and disclaimer of warranty provisions.
#
#
#############################################################################
# Makefile.dep contains all machine-dependent definitions
#############################################################################
include Makefile.dep

CC=gcc
CFLAGS= $(HOSTCFLAGS) -DRDATA
LD=gcc
RM = /bin/rm
MV = /usr/bin/mv

ifeq ($(hosttype),unknown)
buildtargets = unknownhost
else
buildtargets = coff2noff.$(hosttype)
endif

all: $(buildtargets)

# converts a COFF file to Nachos object format
coff2noff.$(hosttype): coff2noff.o
	$(LD) coff2noff.o -o coff2noff.$(hosttype)

clean:
	$(RM) -f coff2noff.o

distclean: clean
	$(MV) coff2noff.c temp.c
	$(RM) -f coff2noff.*
	$(MV) temp.c coff2noff.c


unknownhost:
	@echo Host type could not be determined.
	@echo make is terminating
	@echo Edit Makefile.dep and try again.
