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

# ----- BigInteger on GMP where it is faster, off by default ---------------- #
option(ADS_WITH_GMP "BigInteger on the backend of GMP [see BigInteger.h]" OFF)
if (ADS_WITH_GMP)
    include(cmake/ADSGMP.cmake)
    if (NOT ADS_GMP_FOUND)
        message(FATAL_ERROR "ADS_WITH_GMP is ON, but GMP was not found: "
                "install it (libgmp-dev, gmp-devel, gmp) or set "
                "GMP_INCLUDE_DIR and GMP_LIBRARY")
    endif ()
    target_compile_definitions(ADS INTERFACE ADS_WITH_GMP)
    target_link_libraries(ADS INTERFACE ADS::GMP)
endif ()

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
    find_path(LIBSAIS_INCLUDE libsais.h)
    find_library(LIBSAIS_LIB sais)
    find_path(DIVSUFSORT_INCLUDE divsufsort.h)
    find_library(DIVSUFSORT_LIB divsufsort)
    find_path(EDLIB_INCLUDE edlib.h)
    find_library(EDLIB_LIB edlib)
    find_path(RAPIDFUZZ_INCLUDE rapidfuzz/distance.hpp)
    find_package(Eigen3 QUIET NO_MODULE)
    find_package(Threads REQUIRED)
    foreach (b sort dictionaries priority sequences memory bignum fft hashing
               search suffix edit numeric queues concurrent geometry matrix
               sparse external graphs)
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
    target_link_libraries(bench-concurrent PRIVATE Threads::Threads)
    if (TARGET Eigen3::Eigen)
        target_link_libraries(bench-matrix PRIVATE Eigen3::Eigen)
        target_link_libraries(bench-sparse PRIVATE Eigen3::Eigen)
    endif ()
    if (GMPXX_INCLUDE AND GMP_LIB)
        target_compile_definitions(bench-numeric PRIVATE ADS_HAS_GMP)
        target_include_directories(bench-numeric PRIVATE ${GMPXX_INCLUDE})
        target_link_libraries(bench-numeric PRIVATE ${GMP_LIB})
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
    # the competitor of bench-external: tlx from the directory of its
    # sources ADS_TLX
    set(ADS_TLX "" CACHE PATH "the sources of tlx")
    if (ADS_TLX)
        target_compile_definitions(bench-external PRIVATE ADS_HAS_TLX)
        target_include_directories(bench-external SYSTEM PRIVATE ${ADS_TLX})
    endif ()
    find_package(Threads REQUIRED)
    target_link_libraries(bench-hashing PRIVATE Threads::Threads)
    if (LIBSAIS_INCLUDE AND LIBSAIS_LIB)
        target_compile_definitions(bench-suffix PRIVATE ADS_HAS_LIBSAIS)
        target_include_directories(bench-suffix PRIVATE ${LIBSAIS_INCLUDE})
        target_link_libraries(bench-suffix PRIVATE ${LIBSAIS_LIB})
    endif ()
    if (DIVSUFSORT_INCLUDE AND DIVSUFSORT_LIB)
        target_compile_definitions(bench-suffix PRIVATE ADS_HAS_DIVSUFSORT)
        target_include_directories(bench-suffix PRIVATE ${DIVSUFSORT_INCLUDE})
        target_link_libraries(bench-suffix PRIVATE ${DIVSUFSORT_LIB})
    endif ()
    if (EDLIB_INCLUDE AND EDLIB_LIB)
        target_compile_definitions(bench-edit PRIVATE ADS_HAS_EDLIB)
        target_include_directories(bench-edit PRIVATE ${EDLIB_INCLUDE})
        target_link_libraries(bench-edit PRIVATE ${EDLIB_LIB})
    endif ()
    if (RAPIDFUZZ_INCLUDE)
        target_compile_definitions(bench-edit PRIVATE ADS_HAS_RAPIDFUZZ)
        target_include_directories(bench-edit PRIVATE ${RAPIDFUZZ_INCLUDE})
    endif ()
    # the competitors of bench-priority: tlx and radix-heap from the
    # directory of their sources ADS_PRIORITY
    set(ADS_PRIORITY "" CACHE PATH "the sources of the priority queues")
    if (ADS_PRIORITY)
        target_include_directories(bench-priority SYSTEM PRIVATE
                                   ${ADS_PRIORITY}/tlx
                                   ${ADS_PRIORITY}/radix-heap)
        target_sources(bench-priority PRIVATE
                       ${ADS_PRIORITY}/tlx/tlx/die/core.cpp)
    endif ()
    # the competitors of bench-memory: mimalloc (without the override of
    # malloc) and jemalloc (with the prefix je_) from their prefixes
    set(ADS_MIMALLOC "" CACHE PATH "the prefix of mimalloc")
    if (ADS_MIMALLOC)
        target_compile_definitions(bench-memory PRIVATE ADS_HAS_MIMALLOC)
        target_include_directories(bench-memory PRIVATE
                                   ${ADS_MIMALLOC}/include/mimalloc-2.1)
        target_link_libraries(bench-memory PRIVATE
                     ${ADS_MIMALLOC}/lib/mimalloc-2.1/libmimalloc.a)
    endif ()
    set(ADS_JEMALLOC "" CACHE PATH "the prefix of jemalloc")
    if (ADS_JEMALLOC)
        target_compile_definitions(bench-memory PRIVATE ADS_HAS_JEMALLOC)
        target_include_directories(bench-memory PRIVATE
                                   ${ADS_JEMALLOC}/include)
        target_link_libraries(bench-memory PRIVATE
                              ${ADS_JEMALLOC}/lib/libjemalloc.a
                              ${CMAKE_DL_LIBS})
    endif ()
    target_link_libraries(bench-memory PRIVATE Threads::Threads)
    # the competitors of bench-sort: the header-only sorts from the directory
    # of their sources ADS_SORTING, vqsort of Highway if found
    set(ADS_SORTING "" CACHE PATH "the sources of the sorting libraries")
    if (ADS_SORTING)
        foreach (d ips4o/include ips2ra/include x86-simd-sort/src ska_sort
                   fluxsort/src crumsort/src)
            target_include_directories(bench-sort SYSTEM
                                       PRIVATE ${ADS_SORTING}/${d})
        endforeach ()
    endif ()
    find_package(hwy QUIET)
    if (hwy_FOUND)
        target_compile_definitions(bench-sort PRIVATE ADS_HAS_VQSORT)
        target_link_libraries(bench-sort PRIVATE hwy::hwy_contrib hwy::hwy)
    endif ()
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
        cmake/ADSGMP.cmake
        DESTINATION ${ADS_INSTALL_CMAKEDIR})

# the include directory of ads.pc, as ${prefix}/include when it is relative
if (IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(ADS_PC_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
else ()
    set(ADS_PC_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif ()
# with GMP, its define and the library, by gmp.pc
if (ADS_WITH_GMP)
    set(ADS_PC_REQUIRES "gmp")
    set(ADS_PC_CFLAGS " -DADS_WITH_GMP")
endif ()
configure_file(cmake/ads.pc.in ${CMAKE_CURRENT_BINARY_DIR}/ads.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ads.pc
        DESTINATION ${ADS_INSTALL_PKGCONFIGDIR})

###############################################################################
#                          End of CMakeLists.txt                              #
###############################################################################
