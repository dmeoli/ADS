###############################################################################
#                              CMakeLists.txt                                 #
#                                                                             #
# ADS: the algorithms and the data structures of the book A. Montresor,       #
# Algoritmi e Strutture Dati, 2025 (https://algoritmicamente.it), as a        #
# header-only C++20 library, with the exercises of the book as tests.         #
#                                                                             #
# Build and test:                                                             #
#   cmake -S . -B build && cmake --build build && ctest --test-dir build      #
# Install, then use from another CMake project with                           #
#   find_package(ADS) and target_link_libraries(<target> PRIVATE ADS::ADS)    #
#                                                                             #
# The makefile does the same without CMake [see makefile].                    #
###############################################################################

cmake_minimum_required(VERSION 3.21)
project(ADS
        VERSION 0.1.0
        DESCRIPTION "The algorithms and data structures of the book"
        LANGUAGES CXX)

# ----- the library: headers only, the declarations in include/ADS/<area>/ - #
# ----- and the definitions of the templates in include/ADS/<area>/impl/ ---- #
add_library(ADS INTERFACE)
add_library(ADS::ADS ALIAS ADS)
target_include_directories(ADS INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_compile_features(ADS INTERFACE cxx_std_20)

# ----- the tests: the exercises of the book -------------------------------- #
option(ADS_BUILD_TESTS "build the tests (by default only as the main project)"
       ${PROJECT_IS_TOP_LEVEL})
if (ADS_BUILD_TESTS)
    include(CTest)
    if (BUILD_TESTING)
        add_subdirectory(test)
    endif ()
endif ()

# ----- the benchmarks, off by default -------------------------------------- #
option(ADS_BENCHMARKS "build the benchmarks in bench/" OFF)
if (ADS_BENCHMARKS)
    find_package(Boost QUIET)
    find_path(GMPXX_INCLUDE gmpxx.h)
    find_library(GMPXX_LIB gmpxx)
    find_library(GMP_LIB gmp)
    find_path(FFTW_INCLUDE fftw3.h)
    find_library(FFTW_LIB fftw3)
    foreach (b sort dictionaries priority sequences bignum fft hashing)
        add_executable(bench-${b} bench/${b}.cpp)
        target_link_libraries(bench-${b} PRIVATE ADS::ADS)
        target_compile_definitions(bench-${b} PRIVATE ADS_CONTRACTS=0)
        if (Boost_FOUND)
            target_include_directories(bench-${b}
                                       PRIVATE ${Boost_INCLUDE_DIRS})
        endif ()
    endforeach ()
    if (GMPXX_INCLUDE AND GMPXX_LIB AND GMP_LIB)
        target_compile_definitions(bench-bignum PRIVATE ADS_HAS_GMP)
        target_include_directories(bench-bignum PRIVATE ${GMPXX_INCLUDE})
        target_link_libraries(bench-bignum PRIVATE ${GMPXX_LIB} ${GMP_LIB})
    endif ()
    if (FFTW_INCLUDE AND FFTW_LIB)
        target_compile_definitions(bench-fft PRIVATE ADS_HAS_FFTW)
        target_include_directories(bench-fft PRIVATE ${FFTW_INCLUDE})
        target_link_libraries(bench-fft PRIVATE ${FFTW_LIB})
    endif ()
    # the competitors of bench-hashing: the header-only libraries from the
    # directory of their sources ADS_HASHING, Abseil if found
    set(ADS_HASHING "" CACHE PATH "the sources of the hashing libraries")
    if (ADS_HASHING)
        foreach (d unordered_dense/include robin-map/include libcuckoo xxHash
                   wyhash rapidhash fastfilter_cpp/src
                   xor_singleheader/include pthash/include
                   pthash/external/bits pthash/external/bits/include
                   pthash/external/bits/external/essentials/include
                   pthash/external/mm_file/include BBHash sux)
            target_include_directories(bench-hashing SYSTEM
                                       PRIVATE ${ADS_HASHING}/${d})
        endforeach ()
    endif ()
    find_package(absl QUIET)
    if (absl_FOUND)
        target_compile_definitions(bench-hashing PRIVATE ADS_HAS_ABSL)
        target_link_libraries(bench-hashing PRIVATE absl::flat_hash_map
                                                    absl::hash)
    endif ()
    find_package(Threads REQUIRED)
    target_link_libraries(bench-hashing PRIVATE Threads::Threads)
endif ()

# ----- install and export, for find_package(ADS) and pkg-config ---------- #
# ----- the CMake and pkg-config files go under share, not lib, since ----- #
# ----- a library of headers is the same on every architecture ------------ #
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(ADS_INSTALL_CMAKEDIR ${CMAKE_INSTALL_DATADIR}/cmake/ADS
    CACHE STRING "where to install the files of find_package(ADS)")
set(ADS_INSTALL_PKGCONFIGDIR ${CMAKE_INSTALL_DATADIR}/pkgconfig
    CACHE STRING "where to install ads.pc")

install(TARGETS ADS EXPORT ADSTargets)
install(DIRECTORY include/ADS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT ADSTargets
        NAMESPACE ADS::
        DESTINATION ${ADS_INSTALL_CMAKEDIR})

configure_package_config_file(cmake/ADSConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfig.cmake
        INSTALL_DESTINATION ${ADS_INSTALL_CMAKEDIR})
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfigVersion.cmake
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfigVersion.cmake
        DESTINATION ${ADS_INSTALL_CMAKEDIR})

# the include directory of ads.pc, as ${prefix}/include when it is relative
if (IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(ADS_PC_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
else ()
    set(ADS_PC_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif ()
configure_file(cmake/ads.pc.in ${CMAKE_CURRENT_BINARY_DIR}/ads.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ads.pc
        DESTINATION ${ADS_INSTALL_PKGCONFIGDIR})

###############################################################################
#                          End of CMakeLists.txt                              #
###############################################################################
