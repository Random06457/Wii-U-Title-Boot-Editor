{ pkgs ? import <nixpkgs> {} }:

with pkgs;

stdenv.mkDerivation {
    # dlopen gets overriden by ASAN, causing it to not use the proper rpath, compiling without ASAN works fine
    LD_LIBRARY_PATH="/run/opengl-driver/lib:/run/opengl-driver-32/lib:" + (lib.makeLibraryPath [ pipewire ]);
    NIX_HARDENING_ENABLE="";

    pname = "Wii-U-Title-Boot-Editor";
    version = "2.0.0";

    src = ./.;

    nativeBuildInputs = [
        gcc13
        meson
        ninja
        cmake
    ];

    buildInputs = [
        pkg-config
        SDL2
        fmt_9
        stb
        curl
    ];
}
