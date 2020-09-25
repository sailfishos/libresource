#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)

AC_INIT([libresource],
	m4_esyscmd([build-aux/git-version-gen .tarball-version]),
	[multimedia at ok.research.nokia.com])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS

AC_HEADER_STDC
AC_INCLUDES_DEFAULT
AC_CONFIG_HEADERS([config.h])

#AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE


m4_define(version_major, `echo $VERSION | cut -d. -f1 | cut -d- -f1`)
m4_define(version_minor, `echo $VERSION | cut -d. -f2 | cut -d- -f1`)
m4_define(version_patch, `echo $VERSION | cut -d. -f3 | cut -d- -f1`)

AC_SUBST(VERSION)
AC_SUBST(VERSION_MAJOR, version_major)
AC_SUBST(VERSION_MINOR, version_minor)
AC_SUBST(VERSION_PATCH, version_patch)
AC_SUBST(VERSION_FULL, version_major.version_minor.version_patch)

# libtool API versioning
LT_INIT
LIBRESOURCE_VERSION_INFO="0:18:0"
AC_SUBST(LIBRESOURCE_VERSION_INFO)

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

# shave
SHAVE_INIT([build-aux], [enable])

AC_CONFIG_FILES([build-aux/shave
		 build-aux/shave-libtool
		 libresource.pc
		 libresource-glib.pc
		 Makefile
		 src/Makefile
		 client/Makefile
		 examples/Makefile
		 tests/Makefile
])

AC_OUTPUT

echo "
 == $PACKAGE_NAME ==
    Version:                $VERSION
        API:                ${LIBRESOURCE_VERSION_INFO//:/.}
    Compiler:               ${CC}
    CFLAGS:                 ${CFLAGS}

    Debug enabled:          ${debug}
    With example:           ${have_dbus_glib}
"
