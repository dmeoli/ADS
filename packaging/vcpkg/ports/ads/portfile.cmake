# ADS is header-only: only its headers, its CMake package and its ads.pc
# are installed
set(VCPKG_BUILD_TYPE release)

vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/dmeoli/ADS/releases/download/v${VERSION}/ADS-${VERSION}.tar.gz"
    FILENAME "ADS-${VERSION}.tar.gz"
    SHA512 0
)
vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DADS_BUILD_TESTS=OFF
        -DADS_BENCHMARKS=OFF
        -DADS_WITH_GMP=OFF
        -DADS_WITH_BLAS=OFF
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME ads CONFIG_PATH share/cmake/ADS)
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE"
                                  "${SOURCE_PATH}/COPYING")
