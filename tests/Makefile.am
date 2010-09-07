TESTS = resource-test 

resource_test_SOURCES = resource-test.c ../src/resource.c

resource_test_CFLAGS  = -I$(top_srcdir)/src @DBUS_CFLAGS@
resource_test_LDADD   = -lcheck                                 \
                        @DBUS_LIBS@

memory_leak_test_SOURCES = memory-leak-test.c

memory_leak_test_CFLAGS  = -I$(top_srcdir)/src @DBUS_CFLAGS@
memory_leak_test_LDADD   = $(top_builddir)/src/libresource-glib.la \
                		   $(top_builddir)/src/libresource.la \
                           @DBUS_LIBS@

noinst_PROGRAMS = resource_test memory_leak_test
