%define ver 20170101
Summary: Multimedia Editing and construction
Name: cinelerra
Version: 5.1
Release: %{ver}
License: GPL
Group: Applications/Multimedia
URL: http://cinelerra-cv.org/

# Obtained from :
# git clone git://git.cinelerra.org/goodguy/cinelerra.git cinelerra5
Source0: cinelerra-%{version}-%{ver}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%if 0%{?fedora}%{?centos}
%define rhat 1
%endif

BuildRequires: autoconf
BuildRequires: automake
BuildRequires: cmake
BuildRequires: binutils
BuildRequires: ctags
BuildRequires: flac-devel
BuildRequires: freetype-devel
BuildRequires: gcc-c++
BuildRequires: gettext-devel
BuildRequires: git
BuildRequires: inkscape
BuildRequires: libpng-devel
BuildRequires: libXft-devel
BuildRequires: libXinerama-devel
BuildRequires: libXv-devel
BuildRequires: nasm
BuildRequires: ncurses-devel
BuildRequires: texinfo
BuildRequires: udftools
%{?rhat:BuildRequires: alsa-lib-devel}
%{?rhat:BuildRequires: bzip2-devel}
%{?rhat:BuildRequires: xorg-x11-fonts-cyrillic}
%{?rhat:BuildRequires: xorg-x11-fonts-cyrillic}
%{?rhat:BuildRequires: xorg-x11-fonts-ISO8859-1-100dpi}
%{?rhat:BuildRequires: xorg-x11-fonts-ISO8859-1-75dpi}
%{?rhat:BuildRequires: xorg-x11-fonts-misc}
%{?rhat:BuildRequires: xorg-x11-fonts-Type1}
%{?suse:BuildRequires: alsa-devel}
%{?suse:BuildRequires: libbz2-devel}
%{?suse:BuildRequires: bitstream-vera-fonts}
%{?suse:BuildRequires: xorg-x11-fonts-core}
%{?suse:BuildRequires: xorg-x11-fonts}
%{?suse:BuildRequires: dejavu-fonts}
%{?suse:BuildRequires: libnuma-devel}
BuildRequires: xz-devel
BuildRequires: yasm
BuildRequires: zlib-devel

%description
Multimedia editing and construction

%prep
%setup
%build
./autogen.sh
%configure
%{__make}

%install
%make_install

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, -)
%{_bindir}/*
%{_libdir}/*
%{_datadir}/*

%exclude /usr/src/debug
%exclude /usr/lib/debug

%changelog

