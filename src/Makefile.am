#libdir = ${src_dir}
lib_LTLIBRARIES = libresource.la libresource-glib.la

libresource_la_SOURCES = res-msg.c res-conn.c res-proto.c res-set.c \
                         dbus-proto.c dbus-msg.c \
                         internal-proto.c internal-msg.c
libresource_la_CFLAGS = @DBUS_CFLAGS@ -I$(top_srcdir) -fvisibility=hidden
if DEBUG
libresource_la_CFLAGS += -D__DEBUG__
endif
libresource_la_LDFLAGS = -version-info @LIBRESOURCE_VERSION_INFO@
libresource_la_LIBADD = @DBUS_LIBS@

libresource_glib_la_SOURCES = resource.c resource-glib-glue.c
libresource_glib_la_CFLAGS = @DBUS_CFLAGS@ -I$(top_srcdir)/src \
                             -fvisibility=hidden
if DEBUG
libresource_glib_la_CFLAGS += -D__DEBUG__
endif
libresource_glib_la_LDFLAGS = -version-info @LIBRESOURCE_VERSION_INFO@
libresource_glib_la_LIBADD = $(top_builddir)/src/libresource.la @DBUS_LIBS@


pkgincludedir = $(includedir)/resource
pkginclude_HEADERS = resource.h res-types.h res-conn.h res-proto.h res-set.h \
                     res-msg.h

MAINTAINERCLEANFILES = Makefile.in

clean-local:
	rm -f *~

