# ADS as a Nix flake: `nix build github:dmeoli/ADS` installs the headers,
# the CMake package and ads.pc; `nix flake check` also runs the tests
{
  description = "Algorithms and data structures, header-only C++20";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      each =
        f: nixpkgs.lib.genAttrs systems (s: f nixpkgs.legacyPackages.${s});
    in
    {
      packages = each (pkgs: {
        default =
          (pkgs.callPackage ./packaging/nix/package.nix { }).overrideAttrs
            {
              version = "main";
              src = self;
            };
      });
      checks = each (pkgs: {
        default = self.packages.${pkgs.stdenv.hostPlatform.system}.default;
      });
    };
}
