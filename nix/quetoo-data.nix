{
  stdenv,
  lib,
  fetchgit,
}:
stdenv.mkDerivation {
  pname = "quetoo-data";
  version = "20231123.0";

  meta = {
    license = lib.licenses.cc-by-sa-40;
  };

  src = fetchgit {
    url = "https://github.com/jdolan/quetoo-data";
    rev = "b5f1ba0eef310b359c2a06d81b3e72952b883b7e";
    sparseCheckout = [
      "target"
    ];
    hash = "sha256-nzOg+6EbfV5ReeCprAg4MPkbL3+OofZIcQyakWGDnzk=";
  };

  phases = ["unpackPhase" "installPhase"];

  installPhase = ''
    cp -r target "$out"
  '';
}
