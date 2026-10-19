#!/usr/bin/env bash
###############################################################################
#                           packaging/deb.sh                                  #
#                                                                             #
# Builds the Debian package of ADS from a release tarball: it unpacks the     #
# tarball as ads_<version>.orig.tar.gz, adds packaging/debian, and runs       #
# dpkg-buildpackage. Nothing is uploaded: for a PPA it prints the dput.       #
#                                                                             #
#   packaging/deb.sh <tarball> binary          libads-dev, unsigned           #
#   packaging/deb.sh <tarball> <series> [n]    the signed source package of   #
#                                              0.1.0-1~<series><n> for a PPA  #
#                                                                             #
# The output goes to build-deb/, next to the tarball's copy.                  #
###############################################################################

set -euo pipefail

tarball=$(realpath "$1")
what=${2:-binary}
n=${3:-1}
here=$(cd "$(dirname "$0")" && pwd)

name=$(basename "$tarball" .tar.gz)          # ADS-0.1.0
version=${name#ADS-}
out=$PWD/build-deb
rm -rf "$out"
mkdir -p "$out"
cp "$tarball" "$out/ads_$version.orig.tar.gz"
tar -xzf "$tarball" -C "$out"
src=$out/$name
rm -rf "$src/debian"
cp -R "$here/debian" "$src/debian"

cd "$src"
if [ "$what" = binary ]; then
    dpkg-buildpackage -b -us -uc
    echo "built: $(ls "$out"/*.deb)"
    exit 0
fi

# a PPA build: the version and the distribution of the series, the orig
# tarball only with the first upload of a version (n = 1)
series=$what
sed -i "1s/^ads ([^)]*) [a-z]*;/ads ($version-1~$series$n) $series;/" \
    debian/changelog
if [ "$n" = 1 ]; then sa=-sa; else sa=-sd; fi
dpkg-buildpackage -S $sa -d
changes=$out/ads_$version-1~$series${n}_source.changes
echo "to upload: dput ppa:<team>/<ppa> $changes"

###############################################################################
#                          End of packaging/deb.sh                            #
###############################################################################
