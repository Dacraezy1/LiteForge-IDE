# Builds from the local checkout (used by flake.nix).
# For nixpkgs, use package.nix (fetchFromGitHub).
{ lib
, stdenv
, cmake
, qt6
}:

stdenv.mkDerivation {
  pname = "liteforge";
  version = "1.2.0";

  src = lib.cleanSource ../..;

  nativeBuildInputs = [ cmake qt6.wrapQtAppsHook ];
  buildInputs = [ qt6.qtbase ] ++ lib.optionals stdenv.hostPlatform.isLinux [ qt6.qtwayland ];

  cmakeFlags = [ "-DENABLE_TESTING=ON" ];

  doCheck = true;
  preCheck = "export QT_QPA_PLATFORM=offscreen";

  meta = with lib; {
    description = "Featherweight, zero-bloat native C++ IDE";
    homepage = "https://github.com/Dacraezy1/LiteForge-IDE";
    license = licenses.gpl3Only;
    platforms = platforms.unix;
    mainProgram = "liteforge";
  };
}
