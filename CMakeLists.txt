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
# the executors of the parallel algorithms beyond the pool of ADS [see
# include/ADS/parallel]: OpenMP, oneTBB and FastFlow, OFF by default
find_package(Threads REQUIRED)
target_link_libraries(ADS INTERFACE Threads::Threads)
option(ADS_WITH_OPENMP "the executor of OpenMP [see OpenMPExecutor.h]" OFF)
if (ADS_WITH_OPENMP)
    find_package(OpenMP REQUIRED)
    target_link_libraries(ADS INTERFACE OpenMP::OpenMP_CXX)
endif ()
option(ADS_WITH_TBB "the executor of oneTBB [see TBBExecutor.h]" OFF)
if (ADS_WITH_TBB)
    find_package(TBB REQUIRED)
    target_compile_definitions(ADS INTERFACE ADS_WITH_TBB)
    target_link_libraries(ADS INTERFACE TBB::tbb)
endif ()
# FastFlow is made of headers: from FASTFLOW_DIR if given, from its
# repository otherwise
option(ADS_WITH_FASTFLOW "the executor of FastFlow [see FastFlowExecutor.h]"
       OFF)
if (ADS_WITH_FASTFLOW)
    set(FASTFLOW_DIR "" CACHE PATH "the sources of FastFlow")
    if (NOT FASTFLOW_DIR)
        include(FetchContent)
        FetchContent_Declare(fastflow
                GIT_REPOSITORY https://github.com/fastflow/fastflow.git
                GIT_TAG master)
        FetchContent_Populate(fastflow)
        set(FASTFLOW_DIR ${fastflow_SOURCE_DIR})
    endif ()
    target_compile_definitions(ADS INTERFACE ADS_WITH_FASTFLOW)
    target_include_directories(ADS SYSTEM INTERFACE
                               $<BUILD_INTERFACE:${FASTFLOW_DIR}>)
endif ()

# the sort over the processes of MPI [see include/ADS/distributed], OFF by
# default
option(ADS_WITH_MPI "the distributed sort on MPI [see DistributedSort.h]"
       OFF)
if (ADS_WITH_MPI)
    find_package(MPI REQUIRED COMPONENTS CXX)
    target_compile_definitions(ADS INTERFACE ADS_WITH_MPI)
    target_link_libraries(ADS INTERFACE MPI::MPI_CXX)
endif ()

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
    find_path(SDSL_INCLUDE sdsl/rmq_succinct_sct.hpp)
    find_path(HYPERSCAN_INCLUDE hs/hs.h)
    find_library(HYPERSCAN_LIB hs)
    foreach (b sort dictionaries priority sequences memory bignum fft hashing
               search suffix edit numeric queues concurrent geometry matrix
               sparse external graphs combinatorial backtrack ordered arrays
               sets tries allocators positions assembly fingerprints multi lce
               parallel joins dynamic greedy learned stringdicts flows)
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
    target_link_libraries(bench-joins PRIVATE Threads::Threads)

    target_link_libraries(bench-greedy PRIVATE Threads::Threads)
    # the competitors of bench-parallel: OpenMP and the parallel mode of
    # libstdc++, oneTBB with the policies of std::execution, ParlayLib from
    # the directory of its sources ADS_PARLAY, the parallel IPS4o and IPS2Ra
    # from ADS_SORTING
    find_package(OpenMP QUIET)
    if (OpenMP_CXX_FOUND)
        target_link_libraries(bench-parallel PRIVATE OpenMP::OpenMP_CXX)
    endif ()
    find_package(TBB QUIET)
    if (TBB_FOUND)
        target_compile_definitions(bench-parallel PRIVATE ADS_HAS_TBB
                                                          ADS_WITH_TBB)
        target_link_libraries(bench-parallel PRIVATE TBB::tbb)
    endif ()
    set(ADS_PARLAY "" CACHE PATH "the sources of ParlayLib")
    if (ADS_PARLAY)
        target_include_directories(bench-parallel SYSTEM
                                   PRIVATE ${ADS_PARLAY}/include)
        target_include_directories(bench-greedy SYSTEM
                                   PRIVATE ${ADS_PARLAY}/include
                                           ${ADS_PARLAY})
    endif ()
    target_link_libraries(bench-parallel PRIVATE Threads::Threads)
    # the competitor of bench-joins: PAM from the directory of its sources
    # ADS_PAM, on ParlayLib from ADS_PARLAY
    set(ADS_PAM "" CACHE PATH "the sources of PAM")
    if (ADS_PAM AND ADS_PARLAY)
        target_compile_definitions(bench-joins PRIVATE ADS_HAS_PAM)
        target_include_directories(bench-joins SYSTEM
                                   PRIVATE ${ADS_PAM}/include
                                           ${ADS_PARLAY}/include)
    endif ()
    if (TARGET Eigen3::Eigen)
        target_link_libraries(bench-matrix PRIVATE Eigen3::Eigen)
        target_link_libraries(bench-sparse PRIVATE Eigen3::Eigen)
        target_link_libraries(bench-assembly PRIVATE Eigen3::Eigen)
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
        find_library(FFTWF_LIB fftw3f)
        if (FFTWF_LIB)
            target_compile_definitions(bench-fft PRIVATE ADS_HAS_FFTWF)
            target_link_libraries(bench-fft PRIVATE ${FFTWF_LIB})
        endif ()
    endif ()
    # the header-only competitors of bench-fft: pocketfft from the directory
    # of pocketfft_hdronly.h ADS_POCKETFFT, ducc0 from the directory src of
    # its sources ADS_DUCC0
    set(ADS_POCKETFFT "" CACHE PATH "the directory of pocketfft_hdronly.h")
    if (ADS_POCKETFFT)
        target_compile_definitions(bench-fft PRIVATE ADS_HAS_POCKETFFT)
        target_include_directories(bench-fft SYSTEM PRIVATE ${ADS_POCKETFFT})
    endif ()
    set(ADS_DUCC0 "" CACHE PATH "the directory src of the sources of ducc0")
    if (ADS_DUCC0)
        target_compile_definitions(bench-fft PRIVATE ADS_HAS_DUCC0)
        target_include_directories(bench-fft SYSTEM PRIVATE ${ADS_DUCC0})
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
    target_link_libraries(bench-fft PRIVATE Threads::Threads)
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
    if (HYPERSCAN_INCLUDE AND HYPERSCAN_LIB)
        target_compile_definitions(bench-multi PRIVATE ADS_HAS_HYPERSCAN)
        target_include_directories(bench-multi PRIVATE ${HYPERSCAN_INCLUDE})
        target_link_libraries(bench-multi PRIVATE ${HYPERSCAN_LIB})
    endif ()
    if (SDSL_INCLUDE)
        target_compile_definitions(bench-lce PRIVATE ADS_HAS_SDSL)
        target_include_directories(bench-lce SYSTEM PRIVATE ${SDSL_INCLUDE})
        # the sequential builds of sdsl-lite, against the parallel ones
        target_compile_definitions(bench-parallel PRIVATE ADS_HAS_SDSL)
        target_include_directories(bench-parallel SYSTEM
                                   PRIVATE ${SDSL_INCLUDE})
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
    # malloc, also for bench-allocators) and jemalloc (with the prefix
    # je_) from their prefixes
    set(ADS_MIMALLOC "" CACHE PATH "the prefix of mimalloc")
    if (ADS_MIMALLOC)
        foreach (b memory allocators)
            target_compile_definitions(bench-${b} PRIVATE ADS_HAS_MIMALLOC)
            target_include_directories(bench-${b} PRIVATE
                                       ${ADS_MIMALLOC}/include/mimalloc-2.1)
            target_link_libraries(bench-${b} PRIVATE
                         ${ADS_MIMALLOC}/lib/mimalloc-2.1/libmimalloc.a)
        endforeach ()
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
        foreach (d ips4o/include ips2ra/include)
            target_include_directories(bench-parallel SYSTEM
                                       PRIVATE ${ADS_SORTING}/${d})
        endforeach ()
        target_compile_options(bench-parallel PRIVATE -mcx16)
        target_link_libraries(bench-parallel PRIVATE atomic)
    endif ()
    find_package(hwy QUIET)
    if (hwy_FOUND)
        target_compile_definitions(bench-sort PRIVATE ADS_HAS_VQSORT)
        target_link_libraries(bench-sort PRIVATE hwy::hwy_contrib hwy::hwy)
    endif ()
    # the competitors of the combinatorial benchmarks: the header-only
    # libraries and the programs of Knuth from the directory ADS_COMBI
    set(ADS_COMBI "" CACHE PATH "the sources of the combinatorial libraries")
    if (ADS_COMBI)
        foreach (d combinations libpopcnt libmorton/include)
            target_include_directories(bench-combinatorial SYSTEM
                                       PRIVATE ${ADS_COMBI}/${d})
        endforeach ()
        target_compile_definitions(bench-backtrack
                                   PRIVATE ADS_KNUTH="${ADS_COMBI}/knuth")
    endif ()
    target_link_libraries(bench-allocators PRIVATE Threads::Threads)
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
