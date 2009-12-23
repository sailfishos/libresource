libdir = ${src_dir}
lib_LTLIBRARIES = libresource.la

libresource_la_SOURCES = resource.c \
                         res-msg.c res-conn.c res-proto.c res-set.c \
                         dbus-proto.c dbus-msg.c \
                         internal-proto.c internal-msg.c
libresource_la_CFLAGS = @DBUS_CFLAGS@ -I$(top_srcdir)
if DEBUG
libresource_la_CFLAGS += -D__DEBUG__
endif
libresource_la_LDFLAGS = /usr/lib -version-info $(subst .,:,$(VERSION))
libresource_la_LIBADD = @DBUS_LIBS@

pkgincludedir = $(includedir)/resource
pkginclude_HEADERS = resource.h res-conn.h res-proto.h res-set.h res-msg.h
libdir = /usr/lib

MAINTAINERCLEANFILES = Makefile.in

clean-local:
	rm -f *~

