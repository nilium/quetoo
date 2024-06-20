{
  stdenv,
  lib,
  darwin,
  fetchFromGitHub,
  autoreconfHook,
  pkg-config,
  check,
  Objectively,
  SDL2,
  SDL2_image,
  SDL2_ttf,
  ...
}:
stdenv.mkDerivation {
  pname = "ObjectivelyMVC";
  version = "20240407.0";

  meta = {
    description = "Object oriented MVC framework for OpenGL, SDL2 and GNU C";
    homepage = "https://github.com/jdolan/ObjectivelyMVC";
    license = lib.licenses.zlib;
  };

  src = fetchFromGitHub {
    owner = "jdolan";
    repo = "ObjectivelyMVC";
    rev = "43cd3a5750d136dac2cf795733fbbc868ba55302";
    hash = "sha256-mzHiNSHTlLPh13jlycdf5KaTN1abN5+kthtaO3A6yjY=";
  };

  nativeBuildInputs = [
    autoreconfHook
    pkg-config
  ];
  buildInputs =
    [
      check
      Objectively
      SDL2
      SDL2_image
      SDL2_ttf
    ]
    ++ lib.optionals stdenv.isDarwin
    (with darwin.apple_sdk.frameworks; [
      CoreServices
      OpenGL
      SystemConfiguration
    ]);
}
