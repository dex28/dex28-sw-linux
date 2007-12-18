
build_list = ../web/JAVA/alba.jar
clean_list = ../web/JAVA/alba.jar alba.jar alba-unsigned.jar *.class $(build_list)

include ../../Makefile.inc

# --------------------------

../web/JAVA/alba.jar: alba.jar
	@$(if $(Q), echo "  COPY   " $^ "->" $@ )
	$(Q)cp $^ $@

alba.jar : alba-unsigned.jar
	@$(if $(Q), echo "  SIGN   " $^ "->" $@ )
	$(Q)cp $^ $@
	$(Q)/usr/java/default/bin/jarsigner -keystore iptc.keystore -storepass 3ybfaSFF $@ iptc

alba-unsigned.jar : Albatross.class GraphView.class PhonePort.class SocketConn.class

Albatross.class : Albatross.java

GraphView.class : GraphView.java

PhonePort.class : PhonePort.java

SocketConn.class : SocketConn.java

# --------------------------

listkey : iptc.keystore
	@$(if $(Q), echo "  LISTK  " $^ "->" $@ )
	$(Q)/usr/java/default/bin/keytool -list -v -alias iptc -keystore iptc.keystore -storepass 3ybfaSFF

iptc.keystore :
	@$(if $(Q), echo "  GENK   " $^ "->" $@ )
	$(Q)/usr/java/default/bin/keytool -genkey -validity 3650 \
        -dname "CN=IPTC Tech. Comm. AB, OU=DEX28, O=IPTC, L=Stockholm, ST=Sweden, C=SE" \
        -alias iptc -keystore iptc.keystore -storepass 3ybfaSFF -keypass 3ybfaSFF

