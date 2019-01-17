Name:       libresource

Summary:    MeeGo resource management low level C API libraries
Version:    0.23.1
Release:    0
Group:      System/Resource Policy
License:    LGPLv2.1
URL:        https://github.com/mer-packages/libresource
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dbus-glib-1)
BuildRequires:  pkgconfig(check)

%description
Resource management for MeeGo. Resource policy client library.

%package client
Summary:    MeeGo resource aware client test example
Group:      Development/Tools
Requires:   %{name} = %{version}-%{release}

%description client
Resource management for MeeGo. This is a simple test tool to play with resource management.

%package devel
Summary:    Development files for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Development files for %{name}.


%package doc
Summary:   Documentation for %{name}
Group:     Documentation
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
%{_includedir}/resource*
%{_libdir}/*.so
%{_libdir}/pkgconfig/libresource*.pc

%files doc
%defattr(-,root,root,-)
%{_docdir}/%{name}-%{version}
