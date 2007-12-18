
build_list = webs dbAlba
clean_list = src/*.o *.o $(build_list)

include ../Makefile.inc

# --------------------------

CFLAGS += -DWEBS -DUEMF -DOS="LINUX" -DLINUX -Isrc

ifdef CROSS
CFLAGS += -DBUSYBOX
endif

CFLAGS += -g -DDEBUG   # Debug

CFLAGS += -DUSER_MANAGEMENT_SUPPORT     # User Management switch

CFLAGS += -DDIGEST_ACCESS_SUPPORT     # Digest Access switch

# SSL switches
# SSLPATCHFILE = src/websSSL.o
# CFLAGS += -DWEBS_SSL_SUPPORT -I/usr/include/open/ssl -DOPENSSL
# LDFLAGS += /usr/lib/libssl.a /usr/lib/libcrypto.a

# If-Modified-Support switches (requires math library, libm.a)
# CFLAGS += -DWEBS_IF_MODIFIED_SUPPORT
# LDFLAGS += /usr/lib/libm.a

# Dependencies

WEBS_CORE 	= src/asp.o src/balloc.o src/base64.o src/cgi.o src/default.o  \
			  src/ejlex.o src/ejparse.o src/form.o \
			  src/h.o src/handler.o src/mime.o src/misc.o src/page.o  \
			  src/ringq.o src/rom.o \
			  src/sock.o src/sockGen.o $(SSLPATCHFILE) \
			  src/security.o src/sym.o src/uemf.o src/url.o src/value.o \
			  src/md5c.o src/um.o src/umui.o src/websda.o src/emfdb.o \
			  src/webrom.o src/webs.o src/websuemf.o

src/asp.o:  src/asp.c src/webs.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

src/balloc.o: src/balloc.c src/uemf.h

src/base64.o:  src/base64.c src/webs.h src/wsIntrn.h  src/ej.h src/ejIntrn.h src/uemf.h

src/cgi.o:  src/webs.h src/wsIntrn.h src/uemf.h

src/default.o:  src/default.c src/webs.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

src/ejlex.o:  src/ejlex.c src/ej.h src/ejIntrn.h src/uemf.h

src/ejparse.o:  src/ejparse.c src/ej.h src/ejIntrn.h src/uemf.h

src/emfdb.o:  src/emfdb.h src/wsIntrn.h src/uemf.h

src/form.o:  src/form.c src/webs.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

src/h.o:  src/h.c src/uemf.h

src/handler.o:  src/handler.c src/webs.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

src/md5c.o:  src/md5.h src/wsIntrn.h src/uemf.h

src/mime.o:  src/mime.c src/webs.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

src/misc.o:  src/misc.c src/uemf.h

src/page.o:  src/page.c src/webs.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

src/ringq.o:  src/ringq.c src/uemf.h

src/rom.o:  src/rom.c src/webs.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

src/security.o:  src/security.c src/webs.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

src/sock.o:  src/sock.c src/uemf.h

src/sockGen.o:  src/sockGen.c src/uemf.h

src/sym.o:  src/sym.c src/uemf.h

src/uemf.o:  src/uemf.c src/uemf.h

src/um.o:  src/webs.h src/wsIntrn.h src/um.h src/uemf.h

src/umui.o:  src/webs.h src/wsIntrn.h src/um.h src/uemf.h

src/url.o:  src/url.c src/webs.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

src/value.o:  src/value.c src/uemf.h

src/webrom.o:  src/webrom.c src/webs.h src/wsIntrn.h src/uemf.h

src/webs.o:  src/webs.c src/webs.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

src/websda.o:  src/webs.h src/wsIntrn.h src/websda.h src/uemf.h

src/websuemf.o:  src/websuemf.c src/webs.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

src/websSSL.o:  src/websSSL.c src/websSSL.h src/wsIntrn.h src/ej.h src/ejIntrn.h src/uemf.h

webs.o:  webs.c src/wsIntrn.h src/webs.h src/ej.h src/ejIntrn.h src/uemf.h

alba_db.o: alba_db.c alba_db.h src/uemf.h

albatross.o: albatross.c src/webs.h

webs: webs.o albatross.o alba_db.o $(WEBS_CORE)

dbAlba.o: dbAlba.c alba_db.h src/uemf.h

dbAlba: dbAlba.o src/balloc.o src/h.o src/misc.o src/emfdb.o src/uemf.o


