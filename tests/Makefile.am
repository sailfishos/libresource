TESTS = resource-test 

resource_test_SOURCES = resource-test.c ../src/resource.c

resource_test_CFLAGS  = -I$(top_srcdir)/src @DBUS_CFLAGS@
resource_test_LDADD   = -lcheck                                 \
                        @DBUS_LIBS@

memory_leak_test_SOURCES = memory-leak-test.c
security_test_SOURCES = security-test.c

security_test_CFLAGS  = -I../src/ @DBUS_CFLAGS@ @AEGIS_CFLAGS@
security_test_LDADD   = -L../src/.libs/ -lresource -lresource-glib

memory_leak_test_CFLAGS  = -I$(top_srcdir)/src @DBUS_CFLAGS@
memory_leak_test_LDADD   = $(top_builddir)/src/libresource-glib.la \
                		   $(top_builddir)/src/libresource.la \
                           @DBUS_LIBS@

noinst_PROGRAMS = resource_test security_test memory_leak_test
