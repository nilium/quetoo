{lib', ...}:
lib'.mkFlake' {
  perSystem = {
    self',
    pkgs',
    ...
  }: {
    packages = let
      Objectively = pkgs'.callPackage ./objectively.nix {};
      ObjectivelyMVC = pkgs'.callPackage ./objectively-mvc.nix {inherit (self'.packages) Objectively;};
      quetoo-data = pkgs'.callPackage ./quetoo-data.nix {};
      quetoo = pkgs'.callPackage ./quetoo.nix {
        inherit Objectively ObjectivelyMVC quetoo-data;
        physfs =
          if pkgs'.stdenv.isDarwin
          then
            pkgs'.physfs.overrideAttrs (final: prev: {
              nativeBuildInputs = prev.nativeBuildInputs ++ [pkgs'.fixDarwinDylibNames];
            })
          else pkgs'.physfs;
      };
    in {
      inherit Objectively ObjectivelyMVC quetoo quetoo-data;
      default = quetoo;
    };
  };
}
