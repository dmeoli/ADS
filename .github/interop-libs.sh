#!/usr/bin/env bash
###############################################################################
#                             interop-libs.sh                                 #
#                                                                             #
# Builds from their sources, into the prefix <prefix>, the libraries of the   #
# adapters of include/ADS/interop that the package managers of the runners    #
# lack, or have in a version too old:                                         #
#                                                                             #
#     .github/interop-libs.sh <prefix> <library>...                           #
#                                                                             #
# where a library is sdsl (sdsl-lite 3, its headers), croaring (CRoaring,     #
# static), nanoarrow, graphblas (SuiteSparse:GraphBLAS 10, compact, without   #
# its JIT), kahip (KaHIP, its library without MPI) or networkit (NetworKit,   #
# with tlx); a library whose files are in <prefix> already is skipped, so     #
# that a prefix restored from the cache of the CI is built once. The          #
# versions are those of the tags below; the generator is that of CMake's      #
# environment (CMAKE_GENERATOR), the jobs 4.                                  #
###############################################################################

set -euo pipefail

SDSL_TAG=v3.0.4
CROARING_TAG=v5.5.5
NANOARROW_TAG=apache-arrow-nanoarrow-0.9.0
GRAPHBLAS_TAG=v10.5.3
KAHIP_TAG=v3.28
NETWORKIT_TAG=11.3

prefix=$1
shift
src=$(mktemp -d)
mkdir -p "$prefix/include" "$prefix/lib"

# the sources of the repository $1 at the tag $2, in $src/$3
clone() {
    git -c advice.detachedHead=false clone --quiet --depth 1 --branch "$2" \
        --recurse-submodules --shallow-submodules \
        "https://github.com/$1.git" "$src/$3"
}

# configure, build and install the sources $src/$1 with the options that
# follow
build() {
    local d=$1
    shift
    cmake -S "$src/$d" -B "$src/$d-build" -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX="$prefix" "$@"
    cmake --build "$src/$d-build" --parallel 4
    cmake --install "$src/$d-build"
}

for lib in "$@"; do
    case $lib in
    sdsl)
        test -f "$prefix/include/sdsl/cereal.hpp" && continue
        clone xxsds/sdsl-lite $SDSL_TAG sdsl
        cp -R "$src/sdsl/include/sdsl" "$prefix/include/" ;;
    croaring)
        test -f "$prefix/include/roaring/roaring.hh" && continue
        clone RoaringBitmap/CRoaring $CROARING_TAG croaring
        build croaring -DBUILD_SHARED_LIBS=OFF -DENABLE_ROARING_TESTS=OFF \
              -DROARING_DISABLE_NATIVE=ON ;;
    nanoarrow)
        test -f "$prefix/include/nanoarrow/nanoarrow.h" && continue
        clone apache/arrow-nanoarrow $NANOARROW_TAG nanoarrow
        build nanoarrow ;;
    graphblas)
        test -f "$prefix/include/suitesparse/GraphBLAS.h" && continue
        clone DrTimothyAldenDavis/GraphBLAS $GRAPHBLAS_TAG graphblas
        build graphblas -DGRAPHBLAS_COMPACT=ON -DGRAPHBLAS_USE_JIT=OFF \
              -DBUILD_STATIC_LIBS=OFF -DSUITESPARSE_DEMOS=OFF \
              -DBUILD_TESTING=OFF ;;
    kahip)
        # the library alone: the install of KaHIP needs all its programs
        test -f "$prefix/include/kaHIP_interface.h" && continue
        clone KaHIP/KaHIP $KAHIP_TAG kahip
        cmake -S "$src/kahip" -B "$src/kahip-build" \
              -DCMAKE_BUILD_TYPE=Release -DNOMPI=ON -DPARHIP=OFF \
              -DNONATIVEOPTIMIZATIONS=ON
        cmake --build "$src/kahip-build" --parallel 4 --target kahip
        cp "$src/kahip/interface/kaHIP_interface.h" "$prefix/include/"
        find "$src/kahip-build" -maxdepth 1 -name 'libkahip.*' \
             -exec cp -P {} "$prefix/lib/" \; ;;
    networkit)
        test -f "$prefix/include/networkit/graph/Graph.hpp" && continue
        clone networkit/networkit $NETWORKIT_TAG networkit
        build networkit -DNETWORKIT_BUILD_TESTS=OFF -DNETWORKIT_NATIVE=OFF ;;
    *)
        echo "interop-libs.sh: unknown library $lib" >&2
        exit 1 ;;
    esac
done
rm -rf "$src"
