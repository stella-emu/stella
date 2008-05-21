%define name    stella
%define version 2.6.1
%define rel     1

%define enable_gl 1
%define enable_sound 1
%define enable_debugger 1
%define enable_joystick 1
%define enable_cheats 1
%define enable_static 0

%define release %rel

Summary:        An Atari 2600 Video Computer System emulator
Name:           %{name}
Version:        %{version}
Release:        %{release}
Group:          Emulators
License:        GPL
URL:            http://stella.sourceforge.net
Source:         %{name}-%{version}.tar.bz2
BuildRoot:      %_tmppath/%name-%version-%release-root
BuildRequires:  SDL-devel MesaGLU-devel zlib-devel

%description
The Atari 2600 Video Computer System (VCS), introduced in 1977, was the most
popular home video game system of the early 1980's.  This emulator will run
most Atari ROM images, so that you can play your favorite old Atari 2600 games
on your PC.

%prep

%setup -q

%build
export CXXFLAGS=$RPM_OPT_FLAGS
%configure \
%if %enable_gl
  --enable-gl \
%else
  --disable-gl \
%endif
%if %enable_sound
  --enable-sound \
%else
  --disable-sound \
%endif
%if %enable_debugger
  --enable-debugger \
%else
  --disable-debugger \
%endif
%if %enable_joystick
  --enable-joystick \
%else
  --disable-joystick \
%endif
%if %enable_cheats
  --enable-cheats \
%else
  --disable-cheats \
%endif
%if %enable_static
  --enable-static \
%else
  --enable-shared \
%endif
  --docdir=%{_docdir}/stella \
  --x-libraries=%{_prefix}/X11R6/%{_lib}

%make

%install
rm -rf $RPM_BUILD_ROOT

make install-strip DESTDIR=%{buildroot}
# Mandriva menu entries
install -d -m0755 %{buildroot}%{_menudir}
cat > %{buildroot}%{_menudir}/%{name} << EOF
?package(%{name}): command="stella" \
icon="stella.png" \
needs="x11" \
title="Stella" \
longtitle="A multi-platform Atari 2600 emulator" \
section="More Applications/Emulators" \
xdg="true"
EOF

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/%{name}-%{version}

%post
%update_menus

%postun
%clean_menus

%files
%defattr(-,root,root,-)
%_bindir/*
%{_menudir}/%{name}
%{_datadir}/applications/%{name}.desktop
%_docdir/stella/*
%_datadir/icons/%{name}.png
%_datadir/icons/mini/%{name}.png
%_datadir/icons/large/%{name}.png

%changelog
* Thu May 22 2008 Stephen Anthony <stephena@users.sf.net> 2.6.1-1
- Version 2.6.1 release

* Fri May 16 2008 Stephen Anthony <stephena@users.sf.net> 2.6-1
- Version 2.6 release

* Wed Apr 9 2008 Stephen Anthony <stephena@users.sf.net> 2.5.1-1
- Version 2.5.1 release

* Fri Mar 28 2008 Stephen Anthony <stephena@users.sf.net> 2.5-1
- Version 2.5 release

* Mon Aug 27 2007 Stephen Anthony <stephena@users.sf.net> 2.4.1-1
- Version 2.4.1 release
