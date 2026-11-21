# nixpkgs-style derivation (pkgs/by-name/li/liteforge/package.nix).
# After the v1.2.0 tag exists, replace the hash:  nix-prefetch-github Dacraezy1 LiteForge-IDE --rev v1.2.0
{ lib
, stdenv
, fetchFromGitHub
, cmake
, qt6
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "liteforge";
  version = "1.2.0";

  src = fetchFromGitHub {
    owner = "Dacraezy1";
    repo = "LiteForge-IDE";
    rev = "v${finalAttrs.version}";
    hash = lib.fakeHash;
  };

  nativeBuildInputs = [ cmake qt6.wrapQtAppsHook ];
  buildInputs = [ qt6.qtbase ] ++ lib.optionals stdenv.hostPlatform.isLinux [ qt6.qtwayland ];

  cmakeFlags = [ "-DENABLE_TESTING=ON" ];

  doCheck = true;
  preCheck = "export QT_QPA_PLATFORM=offscreen";

  meta = {
    description = "Featherweight, zero-bloat native C++ IDE";
    homepage = "https://github.com/Dacraezy1/LiteForge-IDE";
    license = lib.licenses.gpl3Only;
    platforms = lib.platforms.unix;
    mainProgram = "liteforge";
  };
})
