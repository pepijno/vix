{
  description = "vix";

  inputs = {
    nixpkgs.url = "nixpkgs/nixos-24.11";
    utils.url = github:numtide/flake-utils;
  };

  outputs = { self, nixpkgs, utils }:
    utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages."${system}";

        buildInputs = with pkgs; [
          valgrind
          nasm
          asm-lsp
          asmfmt
          qbe
          gdb
          zig
          zls
        ];
      in
      rec {
        # `nix develop`
        devShell = pkgs.mkShell {
          inherit buildInputs;
        };
      });
}
