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
          zlib
          libxml2
          asm-lsp
          asmfmt
        ] ++ (with llvmPackages_17; [
          libclang
          clang-unwrapped
          clang
          lld
          llvm
        ]);
      in
      rec {
        # `nix develop`
        devShell = pkgs.mkShell {
          inherit buildInputs;
        };
      });
}
