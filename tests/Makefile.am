TESTS = resource-test

resource_test_SOURCES = resource-test.c ../src/resource.c

resource_test_CFLAGS  = -I$(top_srcdir)/src @DBUS_CFLAGS@
resource_test_LDADD   = -lcheck                                 \
                        @DBUS_LIBS@

noinst_PROGRAMS = resource_test

