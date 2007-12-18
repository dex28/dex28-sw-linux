
build_list = idodMaster
clean_list = *.o $(build_list)

include ../Makefile.inc

LDFLAGS = -lrt

# --------------------------


idodMaster: idodMaster.o passthru.o DTS.o connect.o MD5.o TAU_D.o

idodMaster.o: idodMaster.cpp DTS.h ELUFNC.h

passthru.o: passthru.cpp DTS.h ELUFNC.h

connect.o: connect.cpp DTS.h ELUFNC.h

DTS.o: DTS.cpp DTS.h ELUFNC.h

TAU_D.o: TAU_D.cpp DTS.h ELUFNC.h

MD5.o : MD5.cpp MD5.h


arm: FORCE
	@( cd .. ; ./build.sh --arm --install idodMaster )
