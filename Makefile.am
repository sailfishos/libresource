EXTRA_DIST = autogen.sh build-aux/git-version-gen
SUBDIRS    = src client examples tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libresource.pc libresource-glib.pc

DISTCLEANFILES = libresource.pc libresource-glib.pc *~ 

# see build-aux/git-version-gen
BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@

dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version

ACLOCAL_AMFLAGS = -I m4
