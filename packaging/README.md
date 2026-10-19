# Packaging

This directory holds the recipes with which ADS is distributed by the package
managers, one subdirectory per channel, together with the scripts that fill
them at each release and a small project that uses the installed library
(`test/`), which the recipes build against the package they have just made.
All the recipes take the same source, the tarball `ADS-X.Y.Z.tar.gz` that the
workflow `release.yml` attaches to the GitHub release of the tag `vX.Y.Z`
together with its sha256 (cf. the section on the release below). Since the
library is header-only, the packages do not depend on the architecture; each
of them installs the headers under `include/ADS`, the CMake package `ADS`
(target `ADS::ADS`) under `share/cmake/ADS` and the pkg-config file `ads.pc`
under `share/pkgconfig`.

| Channel        | Files                         | Name                |
|----------------|-------------------------------|---------------------|
| vcpkg          | `vcpkg/ports/ads/`            | `ads`               |
| conda-forge    | `conda/recipe/recipe.yaml`    | `cpp-ads`           |
| Homebrew       | `homebrew/Formula/ads.rb`     | `ads`               |
| Debian, PPA    | `debian/`, `deb.sh`           | `libads-dev`        |
| Conan Center   | `conan/`                      | `ads`               |
| Spack          | `spack/packages/ads/`         | `ads`               |
| Nix            | `nix/package.nix`, `flake.nix`| `ads`               |
| AUR            | `aur/PKGBUILD`, `aur/.SRCINFO`| `ads`               |
| MSYS2          | `msys2/PKGBUILD`              | `mingw-w64-*-ads`   |
| xmake-repo     | `xmake/packages/a/ads/`       | `ads`               |

The name on conda-forge is `cpp-ads`, following the convention of the C++
libraries there, since `ads` is already taken by a Python client of the NASA
Astrophysics Data System, while on the other channels `ads` was still free
(as far as their indexes showed) when the recipes were written. The workflow
`packaging.yml` builds and tests the Homebrew formula on macOS, the vcpkg
port and the MSYS2 package on Windows and the AUR package on Arch Linux, each
from the tarball of the commit, whenever a pull request touches the
packaging.

## The release

The recipes refer to the tarball of the release with a placeholder checksum,
which is filled once the tarball exists, as follows.

1. The section `[Unreleased]` of `CHANGELOG.md` becomes `## [X.Y.Z] -
   <date>`, and the commit is tagged `vX.Y.Z` and pushed; the workflow
   `release.yml` publishes the release, with the notes taken from that
   section, and attaches `ADS-X.Y.Z.tar.gz` and `ADS-X.Y.Z.tar.gz.sha256`.
2. `packaging/bump.sh vX.Y.Z` downloads the tarball and writes its version
   and its checksums into the recipes (the sha256, the sha512 of vcpkg and
   the SRI hash of Nix); the recipes that keep all the versions (Conan,
   Spack, xmake) get a new entry, the others move to the new version with
   their release number back to 1, `debian/changelog` gets a new entry, and
   the snippets of FetchContent and CPM.cmake in the README of the
   repository take the new version. The result is committed on `main`.
3. Each channel below is then updated from the committed recipes; only the
   first release needs a submission to each channel, since several of them
   (conda-forge, Homebrew, Spack, Nix, vcpkg) have bots or reviewers that
   follow the later releases, possibly with a pull request to approve.

Note that the tarball must not change after the release, since all the
checksums depend on it; hence, a release that has to be redone takes a new
tag.

## vcpkg

The port is tested as an overlay with

    vcpkg install ads --overlay-ports=packaging/vcpkg/ports

and the submission to the curated registry goes through a fork of
`microsoft/vcpkg`:

    git clone https://github.com/<user>/vcpkg && cd vcpkg
    ./bootstrap-vcpkg.sh
    cp -R <ADS>/packaging/vcpkg/ports/ads ports/
    ./vcpkg format-manifest ports/ads/vcpkg.json
    ./vcpkg install ads
    git add ports/ads && git commit -m "[ads] new port"
    ./vcpkg x-add-version ads
    git add versions && git commit --amend --no-edit

and a pull request to `microsoft/vcpkg` with the checklist of new ports
filled in. The CI of vcpkg builds the port on all its triplets, and the
review usually takes one to three weeks; the reviewers may ask for
`vcpkg_from_github` (the archive of the tag, with `REF v${VERSION}` and the
SHA512 of that archive) in place of the asset of the release, which is a
two-line change of `portfile.cmake`. A later release is a pull request with
the new `portfile.cmake`, `vcpkg.json` and `x-add-version`.

## conda-forge

A new package enters through `conda-forge/staged-recipes`:

    git clone https://github.com/<user>/staged-recipes
    mkdir staged-recipes/recipes/cpp-ads
    cp packaging/conda/recipe/recipe.yaml staged-recipes/recipes/cpp-ads/

and a pull request, whose CI builds the recipe with rattler-build and runs
its tests (the content of the package, then `packaging/test` with CMake and
pkg-config); the linter is `conda-smithy recipe-lint --conda-forge`. One then
asks for a review with a comment `@conda-forge/help-c-cpp, ready for
review!`, which typically takes one to four weeks. After the merge, the
feedstock `conda-forge/cpp-ads-feedstock` is created with `dmeoli` as its
maintainer, and the autotick bot opens the pull request of each later release
by itself, since it watches the releases on GitHub.

## Homebrew

`homebrew-core` accepts only projects with some notability (at least 30
forks, 30 watchers or 75 stars on GitHub) and usually refuses the new ones
submitted by their author, and therefore the formula goes first to a separate
tap: a GitHub repository
`dmeoli/homebrew-ads` with the formula in `Formula/ads.rb`, after which

    brew install dmeoli/ads/ads

installs the library on macOS and on Linux. Before pushing the formula one
runs `brew style Formula/ads.rb`, `brew install --build-from-source
dmeoli/ads/ads`, `brew test dmeoli/ads/ads` and `brew audit --strict --online
dmeoli/ads/ads`; a workflow running `brew test-bot` in the tap
can also build the bottles. Once ADS is notable, the same formula
goes to `homebrew-core` as `Formula/a/ads.rb` with a pull request, whose CI
builds it on the supported versions of macOS, and later versions are then
bumped by `brew bump-formula-pr`.

## Debian and Ubuntu

The Debian sources stay in `packaging/debian`, outside the root of the
tarball, as Debian asks of upstream projects, and `deb.sh` puts them into the
unpacked tarball. A local build of `libads-dev`, with the tests of the book
run by `dh_auto_test` (skipped with `DEB_BUILD_OPTIONS=nocheck`), is

    packaging/deb.sh ADS-X.Y.Z.tar.gz binary
    lintian -I --pedantic build-deb/*.changes

For a Launchpad PPA, once a PPA (say `ppa:dmeoli/ads`) exists under the
account with the GPG key registered, the signed source package of a series
and its upload are

    packaging/deb.sh ADS-X.Y.Z.tar.gz resolute
    dput ppa:dmeoli/ads build-deb/ads_X.Y.Z-1~resolute1_source.changes

and the same with `noble` for Ubuntu 24.04, whose GCC 13 compiles the tests;
a second upload of the same version to the same series takes `n = 2` (`deb.sh
<tarball> <series> 2`), which leaves the original tarball out. Launchpad
builds the package within an hour and mails the result. Entering Debian
itself takes an Intent To Package (a bug against `wnpp`, filed with
`reportbug wnpp`), whose number goes into the first entry of
`debian/changelog` as `Closes: #<number>` (this is the only remark of lintian
on the current sources), an upload to `mentors.debian.net` and a Debian
Developer who sponsors it; the review there is thorough and may take months.

## Conan Center

The recipe follows the layout of `conan-io/conan-center-index`, where it goes
as `recipes/ads/` (the files `config.yml` and `all/` of `packaging/conan`):

    conan create recipes/ads/all --version X.Y.Z --build=missing

builds the package and runs `test_package` with a profile in C++20 (with
C++17 the recipe declares the configuration invalid). The pull request is
built by the CI of Conan Center on Linux, macOS and Windows; the review of a
new recipe is slow (several weeks is common). A later version adds an entry
to `config.yml` and `all/conandata.yml`, which `bump.sh` writes.

## Spack

The package goes to `spack/spack-packages` as
`repos/spack_repo/builtin/packages/ads/package.py`; in a clone of Spack with
that repository,

    spack style --fix && spack audit packages ads
    spack install --test=root ads

checks the recipe, then builds and runs the 1504 tests of the book. The pull
request is reviewed by the maintainers of Spack, usually within a few weeks;
`maintainers("dmeoli")` makes the user the reviewer of the later changes to
the package. Each new release adds a line `version(...)`, which `bump.sh`
writes.

## Nix

The flake at the root of the repository builds the checkout, so that `nix
build github:dmeoli/ADS` and `nix flake check github:dmeoli/ADS` work with no
submission at all. For nixpkgs, in a fork of `NixOS/nixpkgs`,

    mkdir -p pkgs/by-name/ad/ads
    cp <ADS>/packaging/nix/package.nix pkgs/by-name/ad/ads/
    nixfmt pkgs/by-name/ad/ads/package.nix
    nix-build -A ads

with the user added to `maintainers/maintainer-list.nix` in a first commit
("maintainers: add dmeoli") and listed in `meta.maintainers`, and the package
in a second commit titled "ads: init at X.Y.Z". The review of a new package
in nixpkgs may take weeks; later releases are then bumped by `r-ryantm`.

## AUR

The AUR has no review: with an account on `aur.archlinux.org` and an SSH key
registered there,

    git clone ssh://aur@aur.archlinux.org/ads.git aur-ads
    cp packaging/aur/PKGBUILD aur-ads/ && cd aur-ads
    makepkg --printsrcinfo > .SRCINFO
    makepkg -si && namcap PKGBUILD ads-*.pkg.tar.zst
    git add PKGBUILD .SRCINFO && git commit -m "ads X.Y.Z" && git push

publishes the package at once (the `.SRCINFO` of `bump.sh` is the same file,
but on Arch `makepkg --printsrcinfo` is the reference). The `check()` builds
and runs the tests of the book, which `makepkg --nocheck` skips.

## MSYS2

The package goes to `msys2/MINGW-packages` as `mingw-w64-ads/PKGBUILD`; in an
MSYS2 shell,

    cd mingw-w64-ads && makepkg-mingw --cleanbuild --syncdeps --force

builds it for the current environment (UCRT64, CLANG64, and so on). The CI of
MINGW-packages builds the environments listed in `mingw_arch`, and a new
package is usually merged within days.

## xmake-repo

The package goes to `xmake-io/xmake-repo`, on the branch `dev`, as
`packages/a/ads/xmake.lua`; in a clone of that repository,

    xmake l scripts/test.lua -vD --shallow ads

builds it and runs its `on_test`, which compiles a snippet against
`ADS/sequence/List.h` in C++20. The pull request goes against `dev`, and the
maintainers merge it usually within days; a later version is a new line
`add_versions(...)`, which `bump.sh` writes.

## CMake without a package manager

A CMake project can also take ADS from its sources at configure time, with
FetchContent or CPM.cmake, as shown in the section on its use in the README
of the repository.