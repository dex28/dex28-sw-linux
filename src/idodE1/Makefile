
CCFLAGS=-Ilibpri-1.0-RC2 -Llibpri-1.0-RC2

idodE1: idodE1.o libpri-1.0-RC2/libpri.a
	g++ -o idodE1 idodE1.o -lpri ${CCFLAGS}
	cp idodE1 /pub/CF/albatros
	cp -R libpri-1.0-RC2/*.so* /pub/CF/albatros

idodE1.o: idodE1.cpp
	g++ -O3 -c idodE1.cpp ${CCFLAGS}

libpri-1.0-RC2/libpri.a:
	cd libpri-1.0-RC2
	make
	cd ..

clean :
	rm *.o
