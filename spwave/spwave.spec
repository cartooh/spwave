Summary: An audio file editor. 
Name: spwave
Version: 0.9.0
Release: 1
License: distributable
Distribution: (none)
Vendor: (none)
Group: Applications/Multimedia
Source: %{name}-%{version}-%{release}.tgz
URL: https://www-ie.meijo-u.ac.jp/labs/rj001/spLibs/spwave/index.html
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Provides: %{name}
Requires: spPlugin >= 0.8.6
%if 0%{?rhel} >= 7
%global debug_package %{nil}
BuildRequires: desktop-file-utils, gtk3-devel, spBase >= 0.8.25, spAudio >= 0.7.16, spComponent >= 0.6.23, spLib >= 0.9.5
%else
BuildRequires: desktop-file-utils, gtk2-devel, spBase >= 0.8.25, spAudio >= 0.7.16, spComponent >= 0.6.23, spLib >= 0.9.5
%endif
Summary(ja): 音声ファイルエディタ

%description 
spwave is an audio file editor supporting several sound formats
including WAV, AIFF, MP3, Ogg Vorbis, raw, and more. The program is
designed for research use, so stability and usability are regarded as
important. spwave runs on multiple platforms including Windows, macOS,
and Linux.

%description -l ja
spwaveは，WAV，AIFF，MP3，Ogg Vorbisなど様々なフォーマットに対応した音声
ファイルエディタです．音声の研究に用いることを前提としており，使いやすく，
安定したプログラムを目指しています．また，spwave は，マルチプラットホーム
に対応しており，Windows，macOS，Linuxなどで動作します．

%changelog
* Fri Apr 25 2025  Hideki Banno
- Version 0.9.0-1

* Wed May 15 2024  Hideki Banno
- Version 0.8.6-2

* Fri Apr 21 2023  Hideki Banno
- Version 0.8.6-1

* Wed Mar 22 2023  Hideki Banno
- Version 0.8.5-1

* Wed Nov 11 2020  Hideki Banno
- Version 0.8.4-9

* Tue Dec 24 2019  Hideki Banno
- Version 0.8.4-8

* Sun Apr 14 2019  Hideki Banno
- Version 0.8.4-7

* Thu Nov 22 2018  Hideki Banno
- Version 0.8.4-6

* Wed Jun 01 2016  Hideki Banno
- Version 0.8.4-5

* Mon May 16 2016  Hideki Banno
- Version 0.8.4-4

* Fri Aug 29 2014  Hideki Banno
- Version 0.8.4-3

* Sun Mar 30 2014  Hideki Banno
- Version 0.8.4-1

* Mon Sep  3 2012  Hideki Banno
- Version 0.8.3-1

* Mon Nov 12 2007  Hideki Banno
- Version 0.8.1-3b

* Wed Oct 31 2007  Hideki Banno
- Version 0.8.1-2b

* Sun Oct 21 2007  Hideki Banno
- Version 0.8.0-4b

* Thu Oct  4 2007  Hideki Banno
- Version 0.8.0-3b

* Wed Oct  3 2007  Hideki Banno
- Version 0.8.0-2b

* Tue Oct  2 2007  Hideki Banno
- Version 0.8.0-1b

* Fri Jul 21 2006  Hideki Banno
- Version 0.6.12-4b

* Mon Oct 24 2005  Hideki Banno
- Version 0.6.12-3b

* Mon Sep 12 2005  Hideki Banno
- Version 0.6.12-2b

* Wed Aug 24 2005  Hideki Banno
- Version 0.6.12-1b

* Sun Jul 25 2004  Hideki Banno
- Version 0.6.11-3b

* Sun Jul 11 2004  Hideki Banno
- Version 0.6.11-2b

* Wed Feb 11 2004  Hideki Banno
- Version 0.6.11-1b

* Mon Mar 11 2002  Hideki Banno
- Version 0.6.8

* Tue Feb 5  2002  Hideki Banno
- Version 0.6.7-3b

* Tue Jan 15 2002  Hideki Banno
- Version 0.6.7-1b

* Sun Feb 25 2001  Hideki Banno
- Version 0.6.5-1

* Thu Jan  4 2001  Hideki Banno
- Version 0.6.0-1

* Wed Nov 29 2000  Hideki Banno
- Version 0.5.2-5

* Sat Oct 21  2000 Hideki Banno
- Version 0.5.2

* Fri Aug 11  2000 Hideki Banno
- Version 0.5.1

* Fri Aug 4  2000 Hideki Banno 
- Version 0.5.0

* Fri Aug 27 1999 Hideki Banno 
- Version 0.4.0

* Tue Apr 13 1999 Hideki Banno 
- 1st release

%prep
%setup -q
%{__rm} -rf %{buildroot}
%{__mkdir_p} %{buildroot}%{_prefix}
%{__mkdir_p} %{buildroot}%{_bindir}

%build
cd %{name}
%{__make} TOP=%{_prefix} SPLIBDIRNAME=%{_lib} clean
if [ -d /usr/include/gtk-3.0 ]; then
    %{__make} USE_GTK3=y TOP=%{_prefix} SPLIBDIRNAME=%{_lib}
else
    %{__make} USE_GTK2=y TOP=%{_prefix} SPLIBDIRNAME=%{_lib}
fi
cd ..

%install
cd %{name}
%{__make} TOP=%{_prefix} SPLIBDIRNAME=%{_lib} DEST="" SPDESTROOT=%{buildroot}%{_prefix} install
cd ..

%{__mkdir_p} %{buildroot}%{_datadir}/spwave
cp -r help %{buildroot}%{_datadir}/spwave/

%{__mkdir_p} %{buildroot}%{_datadir}/applications
desktop-file-install \
    --dir %{buildroot}%{_datadir}/applications \
    --add-category Application \
    --add-category AudioVideo \
    %{name}/%{name}.desktop

%{__mkdir_p} %{buildroot}%{_datadir}/icons/hicolor/16x16/apps
%{__mkdir_p} %{buildroot}%{_datadir}/icons/hicolor/32x32/apps
%{__mkdir_p} %{buildroot}%{_datadir}/icons/hicolor/48x48/apps
%{__mkdir_p} %{buildroot}%{_datadir}/icons/hicolor/256x256/apps
%{__install} %{name}/%{name}16.png %{buildroot}%{_datadir}/icons/hicolor/16x16/apps/%{name}.png
%{__install} %{name}/%{name}32.png %{buildroot}%{_datadir}/icons/hicolor/32x32/apps/%{name}.png
%{__install} %{name}/%{name}48.png %{buildroot}%{_datadir}/icons/hicolor/48x48/apps/%{name}.png
%{__install} %{name}/%{name}256.png %{buildroot}%{_datadir}/icons/hicolor/256x256/apps/%{name}.png

%clean
%{__rm} -rf %{buildroot}

%pre
%post
if [ -x %{_bindir}/update-desktop-database ] ; then
    %{_bindir}/update-desktop-database %{_datadir}/applications
fi
touch --no-create %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
    %{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor || :
fi

%preun
%postun
if [ -x %{_bindir}/update-desktop-database ] ; then
    %{_bindir}/update-desktop-database %{_datadir}/applications
fi
if [ "$1" -eq 0 ] ; then
    touch --no-create %{_datadir}/icons/hicolor
    if [ -x %{_bindir}/gtk-update-icon-cache ]; then
	%{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor || :
    fi
fi

%posttrans
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
    %{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor || :
fi

%files 
%defattr(-,root,root)
%doc %{name}/README* %{name}/CHANGES* %{name}/TODO* %{name}/LICENSE*
%{_datadir}/spwave
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%defattr(755,root,root)
%{_bindir}/*
