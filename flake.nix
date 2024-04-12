{
  description = "vix";

  inputs = {
    nixpkgs.url = "nixpkgs/nixos-unstable";
    utils.url = github:numtide/flake-utils;
  };

  outputs = { self, nixpkgs, utils }:
    utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages."${system}";

        buildInputs = with pkgs; [
          cmake
          clang-tools
          gdb
          valgrind
          nasm
        ];
      in
      rec {
        # `nix develop`
        devShell = pkgs.mkShell {
          inherit buildInputs;
        };
      });
}
