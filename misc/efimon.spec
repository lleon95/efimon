Name:           efimon
Version:        0.2.0
Release:        1%{?dist}
Summary:        Efficiency Monitor (efimon)
License:        LGPL
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  meson
BuildRequires:  g++
# Add other dependencies found in your meson.build (e.g., pkgconfig(glib-2.0))
%global debug_package %{nil}

%description
EfiMon - Gets metrics regarding the power consumption of an algorithm

%prep
%autosetup

%build
%meson --wrap-mode=forcefallback -Ddeveloper-mode=false -Denable-pcm=false
%meson_build

%install
%meson_install

%check
%meson_test

%files
%{_libdir}/lib%{name}.so
%{_libdir}/pkgconfig/
%{_bindir}/efimon-*
%{_includedir}/efimon/
