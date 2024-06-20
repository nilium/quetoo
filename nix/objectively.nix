{
  stdenv,
  lib,
  darwin,
  fetchFromGitHub,
  autoreconfHook,
  pkg-config,
  check,
  curl,
  iconv,
}:
stdenv.mkDerivation {
  pname = "Objectively";
  version = "20240324.0";

  meta = {
    description = "Object oriented framework and core library for GNU C";
    homepage = "https://github.com/jdolan/Objectively";
    license = lib.licenses.zlib;
  };

  src = fetchFromGitHub {
    owner = "jdolan";
    repo = "Objectively";
    rev = "00dd3c1ffde3697e6945510911de8e5bb322c735";
    hash = "sha256-tMzBG5h7MVLKme/FtAsX+xb7q4n+fGA0bPGTARHNAiM=";
  };

  makeFlags = ["LIBTOOLFLAGS=--tag=CXX"];
  nativeBuildInputs = [
    autoreconfHook
    pkg-config
  ];
  buildInputs =
    [
      check
      curl.dev
      iconv.dev
    ]
    ++ lib.optionals stdenv.isDarwin
    (with darwin.apple_sdk.frameworks; [Foundation]);
}
