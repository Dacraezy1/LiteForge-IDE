# Copyright 2026 Gentoo Authors
# Distributed under the terms of the GNU General Public License v3

EAPI=8

inherit cmake git-r3 xdg

DESCRIPTION="Featherweight, zero-bloat native C++ IDE"
HOMEPAGE="https://github.com/Dacraezy1/LiteForge-IDE"
EGIT_REPO_URI="https://github.com/Dacraezy1/LiteForge-IDE.git"

LICENSE="GPL-3"
SLOT="0"
IUSE="test wayland"
RESTRICT="!test? ( test )"

RDEPEND="
	>=dev-qt/qtbase-6.2:6[gui,widgets]
	wayland? ( dev-qt/qtwayland:6 )
"
DEPEND="${RDEPEND}"
BDEPEND=">=dev-build/cmake-3.20"

src_configure() {
	local mycmakeargs=(
		-DENABLE_TESTING=$(usex test)
		-DLITEFORGE_LTO=OFF
	)
	cmake_src_configure
}

src_test() {
	local -x QT_QPA_PLATFORM=offscreen
	cmake_src_test
}
