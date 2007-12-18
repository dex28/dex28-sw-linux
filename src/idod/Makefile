
build_list = idod
clean_list = *.o $(build_list)

include ../Makefile.inc

LDFLAGS = -lrt

# --------------------------

idod: idod.o MD5.o

idod.o: idod.cpp

MD5.o : MD5.cpp MD5.h
