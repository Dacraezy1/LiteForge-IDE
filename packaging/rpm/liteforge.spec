Name:           liteforge
Version:        1.2.0
Release:        1%{?dist}
Summary:        Featherweight, zero-bloat native C++ IDE
License:        GPL-3.0-only
URL:            https://github.com/Dacraezy1/LiteForge-IDE
Source0:        %{url}/archive/refs/tags/v%{version}/LiteForge-IDE-%{version}.tar.gz

BuildRequires:  cmake >= 3.20
BuildRequires:  gcc-c++
BuildRequires:  cmake(Qt6Core)
BuildRequires:  cmake(Qt6Gui)
BuildRequires:  cmake(Qt6Widgets)

%description
LiteForge is a small, fast code editor and IDE written in C++20 and Qt 6.
Every panel is built on first use to keep memory low, and themes, keybindings
and settings are plain JSON files.

%prep
%autosetup -n LiteForge-IDE-%{version}

%build
%cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=ON
%cmake_build

%install
%cmake_install

%check
export QT_QPA_PLATFORM=offscreen
%ctest

%files
%license LICENSE
%doc README.md
%{_bindir}/liteforge
%{_datadir}/applications/liteforge.desktop
%{_datadir}/icons/hicolor/scalable/apps/liteforge.svg
%{_datadir}/metainfo/liteforge.metainfo.xml
%{_docdir}/liteforge/

%changelog
* Sat Nov 21 2026 Dacraezy1 <https://github.com/Dacraezy1/LiteForge-IDE> - 1.2.0-1
- Release 1.2.0: lower memory use, faster startup, customisable themes/keybindings
