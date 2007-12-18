
build_list = ../web/cgi-bin/getTarball
clean_list = *.o ../web/cgi-bin/getTarball getTarball $(build_list)

include ../../Makefile.inc

# --------------------------

../web/cgi-bin/getTarball: getTarball
	@$(if $(Q), echo "  COPY   " $^ "->" $@ )
	$(Q)cp $^ $@

getTarball: getTarball.o

getTarball.o: getTarball.cpp

