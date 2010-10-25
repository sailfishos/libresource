EXTRA_DIST = autogen.sh build-aux/git-version-gen
SUBDIRS    = src client examples tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libresource0.pc libresource0-glib.pc

DISTCLEANFILES = libresource0.pc libresource0-glib.pc *~ 

# see build-aux/git-version-gen
BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@

dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version

