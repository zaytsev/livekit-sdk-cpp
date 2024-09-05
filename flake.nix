{
  description = "A CMake project built with Nix";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    fenix = {
      url = "github:nix-community/fenix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    fenix,
  }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};

        # pkgs = import nixpkgs {inherit system;};
        fenixPkgs = fenix.packages.${system};
        rustToolchain = fenixPkgs.stable.toolchain;

        livekitWorkspace = builtins.fromTOML (builtins.readFile ./Cargo.toml);
        livekitFfiProject = builtins.fromTOML (builtins.readFile ./app/Cargo.toml);

        livekitFfiPkgName = livekitFfiProject.package.name;
        livekitFfiPkgVersion = livekitWorkspace.workspace.package.version;

        livekit-ffi =
          (pkgs.makeRustPlatform {
            cargo = rustToolchain;
            rustc = rustToolchain;
          })
          .buildRustPackage {
            pname = livekitFfiPkgName;
            version = livekitFfiPkgVersion;
            src = ./client-sdk-rust;
            cargoLock = {
              lockFile = ./client-sdk-rust/Cargo.lock;
            };
            buildAndTestSubdir = "livekit-ffi";

            # Add this postInstall phase to copy the header file
            postInstall = ''
              mkdir -p $out/include
              cp $src/livekit-ffi/include/livekit_ffi.h $out/include/
            '';
          };
      in {
        packages.livekit-ffi = livekit-ffi;
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "livekit-sdk-cpp";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            protobuf
          ];

          buildInputs = with pkgs; [
            protobuf
            livekit-ffi
          ];

          configurePhase = ''
            cmake -B build -S . -G Ninja \
              -DLIVEKIT_FFI_LIB=${livekit-ffi}/lib/liblivekit_ffi.so \
              -DLIVEKIT_FFI_INCLUDE=${livekit-ffi}/include
          '';

          buildPhase = ''
            cmake --build build
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp build/my-cmake-project $out/bin/
          '';
        };

        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            gcc
            gdb
            protobuf
            pkg-config
            perl

            rustToolchain
            rust-cbindgen

            stdenv.cc.cc
          ];

          buildInputs = with pkgs; [
            llvmPackages.libclang.lib
            clang
            protobuf
            xorg.libX11
            xorg.libXext
            libGL
          ];

          shellHook = with pkgs; ''
            export LIBCLANG_PATH="${llvmPackages.libclang.lib}/lib"
          '';
        };
      }
    );
}
