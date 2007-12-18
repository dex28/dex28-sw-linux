
build_list = fw_led fw_env ntp_client echo_loop who nologin chkconfig random
clean_list = *.o $(build_list)

include ../Makefile.inc

# --------------------------

fw_led: fw_led.o

fw_led.o: fw_led.cpp

fw_env: fw_env.o

fw_env.o: fw_env.cpp

ntp_client: ntp_client.o

ntp_cleint.o: ntp_client.cpp

echo_loop: echo_loop.o

echo_loop.o: echo_loop.cpp

who : who.o

who.o : who.c

nologin : nologin.o

nologin.o : nologin.cpp

chkconfig : chkconfig.o

chkdonfig.o : chkconfig.cpp

random : random.o

random.o : random.cpp
