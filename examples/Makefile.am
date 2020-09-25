bin_PROGRAMS = fmradio

fmradio_SOURCES = fmradio.c
fmradio_CFLAGS = -I$(top_srcdir)/src $(GLIB_CFLAGS) @DBUS_CFLAGS@ -g3 -O0
if DEBUG
fmradio_CFLAGS += -D__DEBUG__
endif
fmradio_LDADD = $(top_builddir)/src/libresource-glib.la \
                $(top_builddir)/src/libresource.la \
                $(DBUS_LIBS)
