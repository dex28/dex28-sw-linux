
CFLAGS = -O3 -DDEBUG_MODE

playxsvf : lenval.o ports.o micro.o 
	gcc -o playxsvf lenval.o ports.o micro.o 

lenval.o : lenval.h lenval.c

ports.o : ports.h ports.c

micro.o : micro.h micro.c
