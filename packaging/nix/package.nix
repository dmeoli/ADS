# The derivation of ADS for nixpkgs, as pkgs/by-name/ad/ads/package.nix
{
  lib,
  stdenv,
  fetchurl,
  cmake,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "ads";
  version = "0.1.0";

  src = fetchurl {
    url = "https://github.com/dmeoli/ADS/releases/download/v${finalAttrs.version}/ADS-${finalAttrs.version}.tar.gz";
    hash = "sha256-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
  };

  nativeBuildInputs = [ cmake ];

  cmakeFlags = [
    (lib.cmakeBool "ADS_BUILD_TESTS" finalAttrs.doCheck)
    (lib.cmakeFeature "ADS_WITH_GMP" "OFF")
    (lib.cmakeFeature "ADS_WITH_BLAS" "OFF")
    (lib.cmakeFeature "ADS_WITH_BOOST" "OFF")
    (lib.cmakeFeature "ADS_WITH_HIGHWAY" "OFF")
    (lib.cmakeFeature "ADS_WITH_LEMON" "OFF")
    (lib.cmakeFeature "ADS_WITH_MPI" "OFF")
    (lib.cmakeFeature "ADS_WITH_STDPAR" "OFF")
    (lib.cmakeFeature "ADS_WITH_STDEXEC" "OFF")
    (lib.cmakeFeature "ADS_WITH_TASKFLOW" "OFF")
    (lib.cmakeFeature "ADS_WITH_KOKKOS" "OFF")
    (lib.cmakeFeature "ADS_WITH_BOOST_MPI" "OFF")
    (lib.cmakeFeature "ADS_WITH_HWLOC" "OFF")
  ];

  doCheck = true;

  meta = {
    description = "Header-only C++20 algorithms and data structures";
    homepage = "https://github.com/dmeoli/ADS";
    changelog = "https://github.com/dmeoli/ADS/blob/v${finalAttrs.version}/CHANGELOG.md";
    license = lib.licenses.boost;
    maintainers = [ ];
    platforms = lib.platforms.all;
  };
})
