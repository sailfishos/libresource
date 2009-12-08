bin_PROGRAMS = resource-client

resource_client_SOURCES = client.c $(top_srcdir)/src/resource.h
resource_client_CFLAGS = -I$(top_srcdir)/src @DBUS_CFLAGS@ @DBUS_CFLAGS@
if DEBUG
resource_client_CFLAGS += -D__DEBUG__
endif
playback_client_LDADD = $(top_builddir)/src/libresource.la @DBUS_LIBS@

