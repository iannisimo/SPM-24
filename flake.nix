{
  description = "A template for Nix based C++ project setup.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/24.05";

    utils.url = "github:numtide/flake-utils";
    utils.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, ... }@inputs: inputs.utils.lib.eachSystem [
    "x86_64-linux" "i686-linux" "aarch64-linux" "x86_64-darwin"
  ] (system: 
  let 
    pkgs = import nixpkgs {inherit system;}; 
  in {
    devShell = pkgs.mkShell rec {
      name = "SPM-prj-24";

      packages = with pkgs; [
        llvmPackages.clang
        llvmPackages.openmp
        cmake
        mpi
        openmpi
        cmakeCurses
        valgrind
        (python3.withPackages (ps: with ps; [ matplotlib pandas ipykernel jupyter ]))
      ];
    shellHook = ''
        export CPATH=$CPATH:$PWD/include/fastflow:$PWD/include/miniz
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${pkgs.openmpi}/lib
    '';
    };
  });
}
