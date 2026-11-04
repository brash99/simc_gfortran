## This makefile must be executed with gmake (gnu make).

## This tells make not to delete these target files on error/interrupt (see man page)
.PRECIOUS: *.o 
RM        = rm -f 
SHELL     = /bin/sh

OBJ	= make_root_tree.o
 
my_objs	=  $(OBJ)

my_deps = $(my_objs:.o=.d)

MYOS := $(subst -,,$(shell uname))

#For use with gfortran compiler
# -fno-automatic - all program storage treated as static
ifeq ($(MYOS),Linux)
  FFLAGSA=-
  INCLUDES=
  FFLAGS = -O3 -fno-automatic
  OTHERLIBS = 
  FC  := gfortran
  F77 := gfortran
  LINKER := $(F77)
endif

ifeq ($(MYOS),Darwin)
  FFLAGS = -O3 -fno-automatic -mmacosx-version-min=27.0 -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
  OTHERLIBS =
  FC  := gfortran
  F77 := gfortran
  LINKER := g++
  FORTRANLIBS := $(shell $(FC) -print-file-name=libgfortran.dylib) $(shell $(FC) -print-file-name=libquadmath.dylib)
endif

ROOTCONFIG ?= root-config
CXX=g++
CXXFLAGS0 = -O3 -std=c++17
CXXFLAGS=$(CXXFLAGS0) $(DROOT)
DMYROOT= -DMYROOT
ROOTLIBS     := $(shell $(ROOTCONFIG) --libs)
ROOTINCLUDE  := -I $(shell $(ROOTCONFIG) --incdir)

none: make_root_tree

all: make_root_tree

make_root_tree: $(my_objs) froot.co
	$(LINKER) $(LDFLAGS) -o make_root_tree $(my_objs) $(OTHERLIBS) froot.co $(ROOTLIBS) $(FORTRANLIBS) $(OTHERLIBS) $(CERNLIBS)

%.o: %.f
	$(FC) -c $(FFLAGS) $<

%.co: %.c
	$(CXX) -c $(CXXFLAGS) $(DMYROOT) $(ROOTINCLUDE) -o $@ $<

clean:
	$(RM) *.o *.co make_root_tree

