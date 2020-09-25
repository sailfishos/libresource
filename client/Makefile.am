AM_CPPFLAGS= \
	-I$(top_srcdir)/src \
	$(DBUS_CFLAGS)

bin_PROGRAMS = resource-client


resource_client_SOURCES = client.c time-stat.c time-stat.h
resource_client_CFLAGS = -g3 -O0 -std=c99 -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE # why?
if DEBUG
resource_client_CFLAGS += -D__DEBUG__
endif
resource_client_LDADD = $(top_builddir)/src/libresource.la @DBUS_LIBS@ -lm -lrt

