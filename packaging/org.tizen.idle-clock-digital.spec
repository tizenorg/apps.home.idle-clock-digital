Name:	org.tizen.idle-clock-digital
Summary:	idle-clock-digital application (EFL)
Version:	0.1.45
Release:	0
Group:	TO_BE/FILLED_IN
License:	Flora Software License
Source0:	%{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(deviced)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(minicontrol-provider)
BuildRequires:  pkgconfig(libxml-2.0)

BuildRequires:  cmake
BuildRequires:  edje-bin
BuildRequires:  embryo-bin
BuildRequires:  gettext-devel
BuildRequires:	hash-signer


%ifarch %{arm}
%define ARCH arm
%else
%define ARCH emulator
%endif

%description
idle-clock-digital.

%prep
%setup -q

%define PREFIX /usr/apps/org.tizen.idle-clock-digital

%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif
RPM_OPT=`echo $CFLAGS|sed 's/-Wp,-D_FORTIFY_SOURCE=2//'`
export CFLAGS=$RPM_OPT

cmake  -DCMAKE_INSTALL_PREFIX="%{PREFIX}" -DARCH=%{ARCH}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}
%define tizen_sign 1
%define tizen_sign_base /usr/apps/org.tizen.idle-clock-digital
%define tizen_sign_level public
%define tizen_author_sign 1
%define tizen_dist_sign 1

%post
GOPTION="-g 5000 -f"
SOPTION="-s litewhome"

/usr/bin/vconftool set -tf int db/idle-clock/digital/showdate 1 -u 5000 -s org.tizen.idle-clock-digital
/usr/bin/vconftool set -tf int db/idle-clock/digital/clock_font 1 -g 5000 -s org.tizen.idle-clock-digital
/usr/bin/vconftool set -tf int db/idle-clock/digital/clock_font_color 8 -g 5000 -s org.tizen.idle-clock-digital

%files
%manifest org.tizen.idle-clock-digital.manifest
%defattr(-,root,root,-)
%{PREFIX}/*
%{PREFIX}/bin/*
%{PREFIX}/res/*
#%{PREFIX}/data/*
/etc/opt/upgrade/*
/etc/smack/accesses2.d/org.tizen.idle-clock-digital.rule
/usr/share/packages/org.tizen.idle-clock-digital.xml
/usr/apps/org.tizen.idle-clock-digital/shared/res/icons/default/small/org.tizen.idle-clock-digital.png
/usr/share/license/%{name}
