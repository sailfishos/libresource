Name:       libresource

Summary:    MeeGo resource management low level C API libraries
Version:    0.25.0
Release:    0
License:    LGPLv2
URL:        https://git.sailfishos.org/mer-core/libresource
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(glib-2.0) >= 2.40
BuildRequires:  pkgconfig(dbus-1) >= 1.8
BuildRequires:  pkgconfig(check)

%description
Resource management for MeeGo. Resource policy client library.

%package client
Summary:    MeeGo resource aware client test example
Requires:   %{name} = %{version}-%{release}

%description client
Resource management for MeeGo. This is a simple test tool to play with resource management.

%package devel
Summary:    Development files for %{name}
Requires:   %{name} = %{version}-%{release}

%description devel
Development files for %{name}.


%package doc
Summary:   Documentation for %{name}
Requires:  %{name} = %{version}-%{release}

%description doc
%{summary}.


%prep
%setup -q -n %{name}-%{version}

%build
echo "%{version}" > .tarball-version
%autogen --disable-static
%configure --disable-static
make

%install
rm -rf %{buildroot}
%make_install
rm -f %{buildroot}/%{_libdir}/*.la

mkdir -p %{buildroot}%{_docdir}/%{name}-%{version}
install -m0644 -t %{buildroot}%{_docdir}/%{name}-%{version} \
        README AUTHORS NEWS ChangeLog

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libresource*.so.*
%license COPYING

%files client
%defattr(-,root,root,-)
%{_bindir}/resource-client
%{_bindir}/fmradio

%files devel
%defattr(-,root,root,-)
%license COPYING
%{_includedir}/resource*
%{_libdir}/*.so
%{_libdir}/pkgconfig/libresource*.pc

%files doc
%defattr(-,root,root,-)
%{_docdir}/%{name}-%{version}
