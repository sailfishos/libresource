bin_PROGRAMS = resource-client

resource_client_SOURCES = client.c
resource_client_CFLAGS = -I$(top_srcdir)/src @DBUS_CFLAGS@ -g3 -O0
if DEBUG
resource_client_CFLAGS += -D__DEBUG__
endif
resource_client_LDADD = $(top_builddir)/src/libresource.la @DBUS_LIBS@

