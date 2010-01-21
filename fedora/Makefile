PACKAGE := libresource
VERSION := 0.1

TOPDIR  := $(shell while [ ! -d fedora -a "`pwd`" != "/" ]; do cd ..; done; \
		   [ -d fedora ] && pwd || exit 1)

ARCH     := i386
RPM      := $(PACKAGE)-$(VERSION).$(ARCH).rpm
SPECFILE := $(TOPDIR)/fedora/$(PACKAGE).spec
TARBALL  := $(TOPDIR)/fedora/$(PACKAGE)-$(VERSION).tar.gz
TARDIR  := $(notdir $(TOPDIR))

STAMP   := $(shell date +%Y-%m-%d-%H%M%S)
TMPDIR  := /tmp/$(PACKAGE)-$(USER)/$(STAMP)


all: rpms cleanup

# rule to build RPM(s) for this package
rpms: rmtar $(SPECFILE) $(TARBALL)
	mkdir -p $(TMPDIR)/{BUILD,RPMS,SOURCES,SPECS} && \
	cp $(SPECFILE) $(TMPDIR)/SPECS && \
	cp $(TARBALL) $(TMPDIR)/SOURCES && \
	rpmbuild --define '_topdir $(TMPDIR)' \
	    -bb $(TMPDIR)/SPECS/$(PACKAGE).spec && \
	cp $(TMPDIR)/RPMS/$(ARCH)/*.rpm $(TOPDIR)/fedora

cleanup:
	rm -fr $(TMPDIR) && \
	rm -f $(SPECFILE)
	rm -f $(TARBALL)

# rules to patch the SPEC file and build a source tarball
$(SPECFILE): $(SPECFILE).in
	cat $< | \
	    sed 's/##__VERSION__##/$(VERSION)/g;s/##__TARDIR__##/$(TARDIR)/g'| \
	    sed 's/##__ARCH__##/$(ARCH)/g' \
	    > $@

$(TARBALL):
	pushd $(TOPDIR) && make distclean || : && ./autogen.sh && popd && \
	pushd $(TOPDIR)/.. && \
	tar -cvzf /tmp/$(notdir $@) \
	    --exclude '.*/fedora/.*.tar.gz' $(notdir $(TARDIR)) && \
	mv /tmp/$(notdir $@) $@

rmtar:
	rm -f $(TARBALL)
