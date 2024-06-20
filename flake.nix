{
  description = "A free first person shooter based on id Tech2";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";

    # Being lazy and using my flake lib. This is in no way required.
    ntk.url = "github:nilium/dot-nix/lib";
    ntk.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = inputs @ {ntk, ...}: ntk.lib.merge inputs [./nix];
}
