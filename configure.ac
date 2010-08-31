#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)


m4_define(TDIR, esyscmd(/bin/bash -c pwd | tr \\n /))
m4_define(PKGNAM, [libresource])
m4_define(PKGSRC, esyscmd(grep -m 1 -o "PKGNAM (.*)" TDIR/debian/changelog | tr -d \\n))
m4_define(PKGVER, regexp(PKGSRC, PKGNAM[ ([1234]*:*\([0123456789.]+\).*)], [\1]))


AC_INIT(PKGNAM, PKGVER, [multimedia@ok.research.nokia.com])
AC_CONFIG_SRCDIR([src])
#AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_GNU_SOURCE


AC_HEADER_STDC
AC_INCLUDES_DEFAULT

# libtool
LT_CURRENT=0
LT_REVISION=1
LT_AGE=0
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)



# check pkgconfig
AC_CHECK_PROG(have_pkg_config, pkg-config, yes, no)

AS_IF([test x"$have_pkg_config" = "xno"], AC_MSG_ERROR([pkg-config is required to install this library]))

# Checks for DBUS.
PKG_CHECK_MODULES(DBUS, dbus-glib-1 >= 0.70 dbus-1 >= 0.70)
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

# options
AC_ARG_ENABLE([debug],
AS_HELP_STRING([--enable-debug],[Enable debug (log) @<:@default=false@:>@]),
[case "${enableval}" in
  yes) debug=true ; CFLAGS="-ggdb -O0 $CFLAGS" ;;
  no)  debug=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],[debug=false])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])


# check for aegis
aegis_enabled="no"
AC_CHECK_LIB(creds, creds_find, [AEGIS_CFLAGS="-DLIBRESOURCE_AEGIS";
                                 AEGIS_LIBS="-lcreds";
                                 aegis_enabled="yes"], 
                                [AC_MSG_WARN([Building without aegis support])])
AC_SUBST(AEGIS_CFLAGS)
AC_SUBST(AEGIS_LIBS)

AC_OUTPUT([
	libresource0.pc
	libresource0-glib.pc
	Makefile
	src/Makefile
	client/Makefile
	examples/Makefile
	tests/Makefile
])

echo "
 == $PACKAGE_NAME $VERSION ==

    Compiler:               ${CC}
    CFLAGS:                 ${CFLAGS}

    Debug enabled:          ${debug}
    With example:           ${have_dbus_glib}
    Aegis enabled:          ${aegis_enabled}
"
