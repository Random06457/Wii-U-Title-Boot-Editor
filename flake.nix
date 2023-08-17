{
  description = "Wii U Meta Folder Editor";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-23.05";

  outputs = { self, nixpkgs }:
    let system = "x86_64-linux"; in
    let pkgs = (import nixpkgs { inherit system; }); in
  {
    packages.${system}.default = (import ./default.nix { inherit pkgs; });
  };
}
