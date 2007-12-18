
build_list = rtpPing rtpPingSrv
clean_list = *.o $(build_list)

include ../Makefile.inc

# --------------------------

LDFLAGS = -lpthread

rtpPing : rtpPing.o

rtpPing.o : rtpPing.cpp

rtpPingSrv : rtpPingSrv.o

rtpPingSrv.o: rtpPingSrv.cpp
