%define name    stella
%define version 3.6
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
BuildRequires:  SDL-devel MesaGLU-devel

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
  --force-builtin-libpng --force-builtin-zlib \
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
* Fri Mar 16 2012 Stephen Anthony <stephena@users.sf.net> 3.6-1
- Version 3.6 release

* Sat Feb 4 2012 Stephen Anthony <stephena@users.sf.net> 3.5.5-1
- Version 3.5.5 release

* Thu Dec 29 2011 Stephen Anthony <stephena@users.sf.net> 3.5-1
- Version 3.5 release

* Sat Jun 11 2011 Stephen Anthony <stephena@users.sf.net> 3.4.1-1
- Version 3.4.1 release

* Sun May 29 2011 Stephen Anthony <stephena@users.sf.net> 3.4-1
- Version 3.4 release

* Fri Nov 12 2010 Stephen Anthony <stephena@users.sf.net> 3.3-1
- Version 3.3 release

* Wed Aug 25 2010 Stephen Anthony <stephena@users.sf.net> 3.2.1-1
- Version 3.2.1 release

* Fri Aug 20 2010 Stephen Anthony <stephena@users.sf.net> 3.2-1
- Version 3.2 release

* Mon May 3 2010 Stephen Anthony <stephena@users.sf.net> 3.1.2-1
- Version 3.1.2 release

* Mon Apr 26 2010 Stephen Anthony <stephena@users.sf.net> 3.1.1-1
- Version 3.1.1 release

* Thu Apr 22 2010 Stephen Anthony <stephena@users.sf.net> 3.1-1
- Version 3.1 release

* Fri Sep 11 2009 Stephen Anthony <stephena@users.sf.net> 3.0-1
- Version 3.0 release

* Thu Jul 4 2009 Stephen Anthony <stephena@users.sf.net> 2.8.4-1
- Version 2.8.4 release

* Thu Jun 25 2009 Stephen Anthony <stephena@users.sf.net> 2.8.3-1
- Version 2.8.3 release

* Tue Jun 23 2009 Stephen Anthony <stephena@users.sf.net> 2.8.2-1
- Version 2.8.2 release

* Fri Jun 19 2009 Stephen Anthony <stephena@users.sf.net> 2.8.1-1
- Version 2.8.1 release

* Tue Jun 9 2009 Stephen Anthony <stephena@users.sf.net> 2.8-1
- Version 2.8 release

* Tue May 1 2009 Stephen Anthony <stephena@users.sf.net> 2.7.7-1
- Version 2.7.7 release

* Tue Apr 14 2009 Stephen Anthony <stephena@users.sf.net> 2.7.6-1
- Version 2.7.6 release

* Fri Mar 27 2009 Stephen Anthony <stephena@users.sf.net> 2.7.5-1
- Version 2.7.5 release

* Mon Feb 9 2009 Stephen Anthony <stephena@users.sf.net> 2.7.3-1
- Version 2.7.3 release

* Tue Jan 27 2009 Stephen Anthony <stephena@users.sf.net> 2.7.2-1
- Version 2.7.2 release

* Mon Jan 26 2009 Stephen Anthony <stephena@users.sf.net> 2.7.1-1
- Version 2.7.1 release

* Sat Jan 17 2009 Stephen Anthony <stephena@users.sf.net> 2.7-1
- Version 2.7 release

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
