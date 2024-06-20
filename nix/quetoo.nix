{
  stdenv,
  lib,
  darwin,
  autoreconfHook,
  pkg-config,
  Objectively,
  ObjectivelyMVC,
  openal,
  physfs,
  libsndfile,
  glib,
  ncurses,
  libxml2,
  SDL2,
  SDL2_image,
  SDL2_ttf,
  quetoo-data,
}:
stdenv.mkDerivation {
  pname = "quetoo";
  version = "20240324.0";

  meta = {
    description = "A free first person shooter based on id Tech2";
    homepage = "http://quetoo.org";
    license = lib.licenses.gpl2;
  };

  src = ../.;

  configureFlagsArray = [
    "CFLAGS=-I${SDL2_ttf}/include/SDL2 -I${SDL2_image}/include/SDL2"
  ];
  makeFlags = [
    "LIBTOOLFLAGS=--tag=CXX"
  ];
  nativeBuildInputs = [
    autoreconfHook
    pkg-config
  ];
  buildInputs =
    [
      Objectively
      ObjectivelyMVC
      physfs
      openal
      libsndfile.dev
      glib
      ncurses.dev
      libxml2.dev
      SDL2
      SDL2_image
      SDL2_ttf
    ]
    ++ lib.optionals stdenv.isDarwin
    (with darwin.apple_sdk.frameworks; [
      AppKit
    ]);
  propagatedBuildInputs = [
    quetoo-data
  ];
  postInstall = ''
    mkdir -p "$out/share"
    ln -s ${quetoo-data} "$out/share/quetoo"
  '';
}
