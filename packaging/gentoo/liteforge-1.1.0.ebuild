EAPI=8

inherit cmake

DESCRIPTION="Featherweight, zero-bloat native C++ IDE"
HOMEPAGE="https://github.com/Dacraezy1/LiteForge-IDE"
SRC_URI="https://github.com/Dacraezy1/LiteForge-IDE/archive/refs/tags/v${PV}.tar.gz -> ${P}.tar.gz"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~arm64"
IUSE="test"
RESTRICT="!test? ( test )"

RDEPEND="
    dev-qt/qtbase:6[gui,widgets]
"
DEPEND="${RDEPEND}"
BDEPEND="
    dev-build/cmake
"

S="${WORKDIR}/LiteForge-IDE-${PV}"

src_configure() {
    local mycmakeargs=(
        -DCMAKE_BUILD_TYPE=Release
        -DENABLE_TESTING=$(usex test)
    )
    cmake_src_configure
}
