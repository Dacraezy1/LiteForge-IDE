{ lib
, stdenv
, cmake
, qt6
}:

stdenv.mkDerivation rec {
  pname = "liteforge";
  version = "1.1.0";

  src = lib.cleanSource ../..;

  nativeBuildInputs = [
    cmake
    qt6.wrapQtAppsHook
  ];

  buildInputs = [
    qt6.qtbase
  ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
  ];

  meta = with lib; {
    description = "Featherweight, zero-bloat native C++ IDE";
    homepage = "https://github.com/Dacraezy1/LiteForge-IDE";
    license = licenses.gpl3Only;
    platforms = platforms.linux ++ platforms.darwin;
    mainProgram = "liteforge";
  };
}
