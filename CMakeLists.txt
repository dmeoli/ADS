# --------------------------------------------------------------------------- #
#    Main CMake file for ADS                                                  #
#                                                                             #
#    ADS: the algorithms and the data structures of the book A. Montresor,    #
#    Algoritmi e Strutture Dati, 2025 (https://algoritmicamente.it), as a     #
#    header-only C++20 library, with the exercises of the book as tests.      #
#                                                                             #
#    This file allows one to build and run the tests using CMake.             #
#    To do so, you can use the following commands:                            #
#                                                                             #
#        $ cmake -S <source-path> -B <build-path>                             #
#        $ cmake --build <build-path>                                         #
#        $ ctest --test-dir <build-path>                                      #
#                                                                             #
#    The following command also installs the library in the system:           #
#                                                                             #
#        $ cmake --install <build-path> [--prefix <dir>]                      #
#                                                                             #
#    and another CMake project then uses it with                              #
#                                                                             #
#        find_package(ADS)                                                    #
#        target_link_libraries(<target> PRIVATE ADS::ADS)                     #
#                                                                             #
#    The makefile does the same without CMake [see makefile and lib/].        #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.21)
# Derive the version from the most recent git tag (with the fallbacks for a
# tarball and for a clone without the tags), so that a release is just the
# tag vX.Y.Z. Must be done before project().
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeriveVersion.cmake)
ads_derive_version(ADS_VERSION)

project(ADS
        VERSION ${ADS_VERSION}
        DESCRIPTION "The algorithms and data structures of the book"
        HOMEPAGE_URL https://github.com/dmeoli/ADS
        LANGUAGES CXX)

# ----- Settings ------------------------------------------------------------ #
# Sets the default build type (if none was specified); the files of cmake/
# are included by their path, so that a project that takes ADS as a
# subproject, and has files of the same name, does not give its own.
# See: https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildType.cmake)

# ----- Library ------------------------------------------------------------- #
# Headers only: the declarations in include/ADS/<area>/ and the definitions
# of the templates in include/ADS/<area>/impl/, hence an INTERFACE library.
# Different INSTALL_INTERFACE and BUILD_INTERFACE paths are used when
# generating the target import file (ADSTargets.cmake): a target that finds
# ADS in its build directory looks into the BUILD_INTERFACE path for the
# headers, one that finds it installed into the INSTALL_INTERFACE path.
add_library(ADS INTERFACE)
target_include_directories(ADS INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_compile_features(ADS INTERFACE cxx_std_20)

# ----- Sanitizers ---------------------------------------------------------- #
# The sanitizers of this build, read from the options of the compiler
# (-fsanitize= of GCC and Clang, /fsanitize= of MSVC): ADS_SANITIZE_ADDRESS
# and ADS_SANITIZE_THREAD, ON when the options name them, are defined on
# the targets of this build tree, so that the headers and the tests know
# it from the build rather than from the macros of each compiler [see
# EventCount.h and test/CMakeLists.txt]; a program built otherwise defines
# them itself, as the makefile does.
string(TOUPPER "${CMAKE_BUILD_TYPE}" _ads_bt)
string(REGEX MATCHALL "[-/]fsanitize=[^ ]+" _ads_san
       "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_ads_bt}}")
set(ADS_SANITIZE_ADDRESS OFF)
set(ADS_SANITIZE_THREAD OFF)
foreach (s IN LISTS _ads_san)
    string(REGEX REPLACE "^[-/]fsanitize=" "" s "${s}")
    string(REPLACE "," ";" s "${s}")
    if ("address" IN_LIST s)
        set(ADS_SANITIZE_ADDRESS ON)
    endif ()
    if ("thread" IN_LIST s)
        set(ADS_SANITIZE_THREAD ON)
    endif ()
endforeach ()
foreach (s ADDRESS THREAD)
    if (ADS_SANITIZE_${s})
        target_compile_definitions(ADS INTERFACE
                                   $<BUILD_INTERFACE:ADS_SANITIZE_${s}>)
    endif ()
endforeach ()
message(STATUS "ADS: sanitizers: address ${ADS_SANITIZE_ADDRESS}, "
        "thread ${ADS_SANITIZE_THREAD}")

# This alias is defined so that executables in this same project, and the
# projects that take ADS as a subproject, can use the library with the same
# notation as the installed one.
add_library(ADS::ADS ALIAS ADS)

# ----- Configuration header ------------------------------------------------ #
# This generates ADS/ADSConfig.h, with the version, in the build directory;
# it is installed with the headers, none of which includes it.
configure_file(cmake/ADSConfig.h.in include/ADS/ADSConfig.h @ONLY)

# ----- Sets of the headers ------------------------------------------------- #
# The entry points ADS/core.h, ADS/base.h, ADS/advanced.h and ADS/ADS.h are
# written from cmake/ads_sets.txt by cmake/ads_sets.py, which stops on a
# header with no set or on an include across the sets that the file does
# not allow; it is run here when ADS is the main project and Python is
# found, and the target ads_sets runs it again.
find_package(Python3 QUIET COMPONENTS Interpreter)
if (PROJECT_IS_TOP_LEVEL AND Python3_FOUND)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                 ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ads_sets.txt)
    execute_process(COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ads_sets.py
                    RESULT_VARIABLE _sets ERROR_VARIABLE _sets_err)
    if (NOT _sets EQUAL 0)
        message(FATAL_ERROR "${_sets_err}")
    endif ()
    add_custom_target(ads_sets
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ads_sets.py
            COMMENT "the entry points of the sets [see cmake/ads_sets.txt]")
endif ()

# ----- Requirements -------------------------------------------------------- #
# The pool of threads of ADS [see include/ADS/parallel].
find_package(Threads REQUIRED)
target_link_libraries(ADS INTERFACE Threads::Threads)

# BigInteger and Rational on GMP [see BigInteger.h and Rational.h], gemm()
# on a CBLAS [see DenseMatrix.h]: AUTO, the default, turns each on when a
# program that calls the library compiles and links, ON requires it (an
# error at configure time if it is not found), OFF leaves the code of ADS.
set(ADS_WITH_GMP AUTO CACHE STRING
    "BigInteger and Rational on GMP: AUTO (if found), ON or OFF")
set_property(CACHE ADS_WITH_GMP PROPERTY STRINGS AUTO ON OFF)
set(ADS_WITH_BLAS AUTO CACHE STRING
    "gemm() on a CBLAS: AUTO (if found), ON or OFF")
set_property(CACHE ADS_WITH_BLAS PROPERTY STRINGS AUTO ON OFF)
string(TOUPPER "${ADS_WITH_GMP}" ADS_GMP_MODE)
string(TOUPPER "${ADS_WITH_BLAS}" ADS_BLAS_MODE)
set(ADS_USE_GMP OFF)
if (ADS_GMP_MODE STREQUAL "AUTO" OR ADS_WITH_GMP)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ADSGMP.cmake)
    if (ADS_GMP_FOUND)
        set(ADS_USE_GMP ON)
    elseif (NOT ADS_GMP_MODE STREQUAL "AUTO")
        message(FATAL_ERROR "ADS_WITH_GMP is ON, but GMP was not found: "
                "install it (libgmp-dev, gmp-devel, gmp) or set "
                "GMP_INCLUDE_DIR and GMP_LIBRARY")
    endif ()
endif ()
# The directories where the libraries were found, where ADSConfig.cmake
# looks first.
if (ADS_USE_GMP)
    target_compile_definitions(ADS INTERFACE ADS_WITH_GMP)
    target_link_libraries(ADS INTERFACE ADS::GMP)
    get_filename_component(ADS_GMP_LIBDIR "${GMP_LIBRARY}" DIRECTORY)
    set(ADS_GMP_HINTS "${GMP_INCLUDE_DIR};${ADS_GMP_LIBDIR}")
endif ()
set(ADS_USE_BLAS OFF)
if (ADS_BLAS_MODE STREQUAL "AUTO" OR ADS_WITH_BLAS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ADSBLAS.cmake)
    if (ADS_BLAS_FOUND)
        set(ADS_USE_BLAS ON)
    elseif (NOT ADS_BLAS_MODE STREQUAL "AUTO")
        message(FATAL_ERROR "ADS_WITH_BLAS is ON, but no CBLAS was found: "
                "install one (libopenblas-dev, openblas-devel, openblas) "
                "or set CBLAS_INCLUDE_DIR and CBLAS_LIBRARY")
    endif ()
endif ()
if (ADS_USE_BLAS)
    target_compile_definitions(ADS INTERFACE ADS_WITH_BLAS)
    target_link_libraries(ADS INTERFACE ADS::BLAS)
    get_filename_component(ADS_BLAS_LIBDIR "${CBLAS_LIBRARY}" DIRECTORY)
    set(ADS_BLAS_HINTS "${CBLAS_INCLUDE_DIR};${ADS_BLAS_LIBDIR}")
endif ()
message(STATUS "ADS: GMP ${ADS_USE_GMP}, CBLAS ${ADS_USE_BLAS}")

# PointerTable and FlatHashTable on the tables of Boost, SmallVector on
# boost::container::small_vector and Xoshiro256 on Boost.Random [see
# PointerTable.h, FlatHashTable.h, SmallVector.h and RandomGenerators.h]:
# AUTO, the default, turns them on when the headers of Boost 1.83 or later
# are found, ON requires them, OFF leaves the code of ADS. Boost is used
# by its headers alone, so that nothing is linked.
set(ADS_WITH_BOOST AUTO CACHE STRING
    "the delegations to the headers of Boost: AUTO (if found), ON or OFF")
set_property(CACHE ADS_WITH_BOOST PROPERTY STRINGS AUTO ON OFF)
string(TOUPPER "${ADS_WITH_BOOST}" ADS_BOOST_MODE)
set(ADS_USE_BOOST OFF)
if (ADS_BOOST_MODE STREQUAL "AUTO" OR ADS_WITH_BOOST)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ADSBoost.cmake)
    if (ADS_BOOST_FOUND)
        set(ADS_USE_BOOST ON)
    elseif (NOT ADS_BOOST_MODE STREQUAL "AUTO")
        message(FATAL_ERROR "ADS_WITH_BOOST is ON, but the headers of Boost "
                "1.83 or later were not found: install them (libboost-dev, "
                "boost-devel, boost) or set Boost_INCLUDE_DIR")
    endif ()
endif ()
if (ADS_USE_BOOST)
    target_compile_definitions(ADS INTERFACE ADS_WITH_BOOST)
    target_link_libraries(ADS INTERFACE ADS::Boost)
    set(ADS_BOOST_HINTS "${Boost_INCLUDE_DIR}")
endif ()
message(STATUS "ADS: Boost ${ADS_USE_BOOST}")

# The placement of the threads on hwloc [see Affinity.h]: AUTO, the default,
# turns it on when a program that loads the topology of hwloc 2 compiles and
# links, ON requires it, OFF leaves the threads where the system puts them.
set(ADS_WITH_HWLOC AUTO CACHE STRING
    "the placement of the threads on hwloc: AUTO (if found), ON or OFF")
set_property(CACHE ADS_WITH_HWLOC PROPERTY STRINGS AUTO ON OFF)
string(TOUPPER "${ADS_WITH_HWLOC}" ADS_HWLOC_MODE)
set(ADS_USE_HWLOC OFF)
if (ADS_HWLOC_MODE STREQUAL "AUTO" OR ADS_WITH_HWLOC)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ADSHwloc.cmake)
    if (ADS_HWLOC_FOUND)
        set(ADS_USE_HWLOC ON)
    elseif (NOT ADS_HWLOC_MODE STREQUAL "AUTO")
        message(FATAL_ERROR "ADS_WITH_HWLOC is ON, but hwloc 2 was not "
                "found: install it (libhwloc-dev, hwloc-devel, hwloc) or set "
                "HWLOC_INCLUDE_DIR and HWLOC_LIBRARY")
    endif ()
endif ()
if (ADS_USE_HWLOC)
    target_compile_definitions(ADS INTERFACE ADS_WITH_HWLOC)
    target_link_libraries(ADS INTERFACE ADS::Hwloc)
    get_filename_component(ADS_HWLOC_LIBDIR "${HWLOC_LIBRARY}" DIRECTORY)
    set(ADS_HWLOC_HINTS "${HWLOC_INCLUDE_DIR};${ADS_HWLOC_LIBDIR}")
endif ()
message(STATUS "ADS: hwloc ${ADS_USE_HWLOC}")

# The vector kernels on Google Highway [see Simd.h] and the prefetch [see
# Prefetch.h]: AUTO, the default, turns them on when a program that
# dispatches a kernel at run time and calls the sort of Highway compiles
# and links, ON requires it, OFF leaves the scalar code of ADS and a
# prefetch that does nothing. Highway chooses the instructions of the
# processor when the program runs, so that no flag of the compiler is
# needed.
set(ADS_WITH_HIGHWAY AUTO CACHE STRING
    "the vector kernels on Google Highway: AUTO (if found), ON or OFF")
set_property(CACHE ADS_WITH_HIGHWAY PROPERTY STRINGS AUTO ON OFF)
string(TOUPPER "${ADS_WITH_HIGHWAY}" ADS_HWY_MODE)
set(ADS_USE_HIGHWAY OFF)
if (ADS_HWY_MODE STREQUAL "AUTO" OR ADS_WITH_HIGHWAY)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ADSHighway.cmake)
    if (ADS_HWY_FOUND)
        set(ADS_USE_HIGHWAY ON)
    elseif (NOT ADS_HWY_MODE STREQUAL "AUTO")
        message(FATAL_ERROR "ADS_WITH_HIGHWAY is ON, but Highway was not "
                "found: install it (libhwy-dev, highway-devel, highway) or "
                "set HWY_INCLUDE_DIR, HWY_LIBRARY and HWY_CONTRIB_LIBRARY")
    endif ()
endif ()
if (ADS_USE_HIGHWAY)
    target_compile_definitions(ADS INTERFACE ADS_WITH_HIGHWAY)
    target_link_libraries(ADS INTERFACE ADS::Highway)
    get_filename_component(ADS_HWY_LIBDIR "${HWY_LIBRARY}" DIRECTORY)
    set(ADS_HWY_HINTS "${HWY_INCLUDE_DIR};${ADS_HWY_LIBDIR}")
endif ()
message(STATUS "ADS: Highway ${ADS_USE_HIGHWAY}")

# The interoperation with LEMON [see include/ADS/interop/lemon] and the
# solves from scratch that ADS delegates to it [see Matching.h and
# Assignment.h], in the target ADS::LEMON, which defines ADS_WITH_LEMON and
# links LEMON (abLEMON, or a LEMON whose headers compile in C++20): AUTO,
# the default, builds it when LEMON is found, ON requires it, OFF leaves it
# out. ADS::ADS never includes LEMON, so that nothing of it is compiled for
# a program that does not link ADS::LEMON.
set(ADS_WITH_LEMON AUTO CACHE STRING
    "the target ADS::LEMON of LEMON: AUTO (if found), ON or OFF")
set_property(CACHE ADS_WITH_LEMON PROPERTY STRINGS AUTO ON OFF)
string(TOUPPER "${ADS_WITH_LEMON}" ADS_LEMON_MODE)
set(ADS_USE_LEMON OFF)
if (ADS_LEMON_MODE STREQUAL "AUTO" OR ADS_WITH_LEMON)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ADSLemon.cmake)
    if (ADS_LEMON_FOUND)
        set(ADS_USE_LEMON ON)
    elseif (NOT ADS_LEMON_MODE STREQUAL "AUTO")
        message(FATAL_ERROR "ADS_WITH_LEMON is ON, but no LEMON whose headers "
                "compile in C++20 was found: install abLEMON or set "
                "LEMON_INCLUDE_DIR (and LEMON_LIBRARY)")
    endif ()
endif ()
if (ADS_USE_LEMON)
    add_library(ADS_LEMON INTERFACE)
    add_library(ADS::LEMON ALIAS ADS_LEMON)
    set_target_properties(ADS_LEMON PROPERTIES EXPORT_NAME LEMON)
    target_link_libraries(ADS_LEMON INTERFACE ADS ADS::LEMON_library)
    target_compile_definitions(ADS_LEMON INTERFACE ADS_WITH_LEMON)
    set(ADS_LEMON_HINTS "${LEMON_INCLUDE_DIR}")
    # the warm start of the algorithms of LEMON [see
    # include/ADS/interop/lemon/Warm.h], private copies of some of its
    # headers rewritten by the rules of include/ADS/interop/lemon/shim, put
    # on the include path before LEMON; with a LEMON whose lines are not
    # those of the rules, ON stops and AUTO goes on without it. An installed
    # ADS writes them again from the LEMON that its user finds [see
    # ADSConfig.cmake].
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ADSLemonShim.cmake)
    set(ADS_LEMON_SHIM_DIR ${CMAKE_CURRENT_BINARY_DIR}/lemon-shim)
    ads_lemon_shim(${ADS_LEMON_SHIM_DIR} ${LEMON_INCLUDE_DIR}
                   ${CMAKE_CURRENT_SOURCE_DIR}/include/ADS/interop/lemon/shim
                   ADS_USE_LEMON_SHIM)
    if (ADS_USE_LEMON_SHIM)
        target_include_directories(ADS_LEMON BEFORE INTERFACE
                $<BUILD_INTERFACE:${ADS_LEMON_SHIM_DIR}>)
        target_compile_definitions(ADS_LEMON INTERFACE
                $<BUILD_INTERFACE:ADS_LEMON_SHIM>)
    elseif (ADS_LEMON_MODE STREQUAL "AUTO")
        message(STATUS "ADS: no warm start of LEMON: ${ADS_USE_LEMON_SHIM_WHY}")
    else ()
        message(FATAL_ERROR "ADS: ${ADS_USE_LEMON_SHIM_WHY}")
    endif ()
endif ()
if (ADS_USE_LEMON_SHIM)
    message(STATUS "ADS: LEMON ON, its warm start ON")
elseif (ADS_USE_LEMON)
    message(STATUS "ADS: LEMON ON, its warm start OFF")
else ()
    message(STATUS "ADS: LEMON OFF")
endif ()

# The executors of the parallel algorithms beyond the pool of ADS [see
# include/ADS/parallel]: OpenMP, oneTBB and FastFlow, OFF by default.
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

# The sort and the runtime over the processes of MPI [see
# include/ADS/distributed]: OFF, the default, as for the other runtimes
# (OpenMP, oneTBB, FastFlow), since ADS::ADS then carries MPI to every
# program that links it; AUTO turns them on when FindMPI finds an MPI for
# C++ (Open MPI, MPICH), ON requires it.
set(ADS_WITH_MPI OFF CACHE STRING
    "the distributed sort and runtime on MPI: OFF, AUTO (if found) or ON")
set_property(CACHE ADS_WITH_MPI PROPERTY STRINGS AUTO ON OFF)
string(TOUPPER "${ADS_WITH_MPI}" ADS_MPI_MODE)
set(ADS_USE_MPI OFF)
if (ADS_MPI_MODE STREQUAL "AUTO" OR ADS_WITH_MPI)
    find_package(MPI QUIET COMPONENTS CXX)
    if (MPI_CXX_FOUND)
        set(ADS_USE_MPI ON)
    elseif (NOT ADS_MPI_MODE STREQUAL "AUTO")
        message(FATAL_ERROR "ADS_WITH_MPI is ON, but no MPI was found: "
                "install Open MPI or MPICH, or set MPI_HOME")
    endif ()
endif ()
if (ADS_USE_MPI)
    target_compile_definitions(ADS INTERFACE ADS_WITH_MPI)
    target_link_libraries(ADS INTERFACE MPI::MPI_CXX)
endif ()
message(STATUS "ADS: MPI ${ADS_USE_MPI}")

# The executors on the runtimes of a program [see include/ADS/interop/
# parallel and include/ADS/distributed/BoostMPI.h]: the parallel algorithms
# of the standard library (ADS_WITH_STDPAR), the senders of stdexec
# (ADS_WITH_STDEXEC), Taskflow (ADS_WITH_TASKFLOW), Kokkos (ADS_WITH_KOKKOS)
# and Boost.MPI beside MPI (ADS_WITH_BOOST_MPI), the archives of cereal
# for the persistent structures (ADS_WITH_CEREAL) [see include/ADS/interop/
# cereal/Cereal.h], the libraries that run their algorithms on the graphs
# and the matrices of ADS, Eigen (ADS_WITH_EIGEN), METIS (ADS_WITH_METIS),
# KaHIP (ADS_WITH_KAHIP), SuiteSparse:GraphBLAS (ADS_WITH_GRAPHBLAS) and
# NetworKit (ADS_WITH_NETWORKIT), the bitmaps of CRoaring
# (ADS_WITH_CROARING) and the vectors of sdsl-lite 3 (ADS_WITH_SDSL) [see
# include/ADS/interop/]. AUTO, the default, makes
# the target ADS::<name> of each runtime found [see ADSInterop.cmake], ON
# requires it, OFF leaves it out; ADS::ADS never carries them, so that a
# program takes a runtime only by linking its target, and the tests of the
# executors run on each one found.
set(ADS_INTEROP_BUILT "")
set(ADS_INTEROP_WANT "")
foreach (r StdPar stdexec Taskflow Kokkos BoostMPI cereal Eigen METIS KaHIP
           GraphBLAS NetworKit CRoaring sdsl)
    string(TOUPPER "${r}" R)
    if (R STREQUAL "BOOSTMPI")
        set(R "BOOST_MPI")
    endif ()
    set(ADS_WITH_${R} AUTO CACHE STRING
        "the interoperation with ${r}: AUTO (if found), ON or OFF")
    set_property(CACHE ADS_WITH_${R} PROPERTY STRINGS AUTO ON OFF)
    string(TOUPPER "${ADS_WITH_${R}}" ADS_${R}_MODE)
    if (ADS_${R}_MODE STREQUAL "AUTO" OR ADS_WITH_${R})
        list(APPEND ADS_INTEROP_WANT ${r})
    endif ()
endforeach ()
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ADSInterop.cmake)
foreach (r IN LISTS ADS_INTEROP_WANT)
    string(TOUPPER "${r}" R)
    if (R STREQUAL "BOOSTMPI")
        set(R "BOOST_MPI")
    endif ()
    if (ADS_${r}_FOUND)
        list(APPEND ADS_INTEROP_BUILT ${r})
    elseif (NOT ADS_${R}_MODE STREQUAL "AUTO")
        message(FATAL_ERROR "ADS_WITH_${R} is ON, but ${r} was not found "
                "[see cmake/ADSInterop.cmake for the hints]")
    endif ()
endforeach ()
message(STATUS "ADS: runtimes of the program found: ${ADS_INTEROP_BUILT}")

# ----- C++20 module -------------------------------------------------------- #
# The module ads (src/ADS.cppm) exports the names of all the headers, so
# that a translation unit can write "import ads;" in place of including
# them, by exporting the modules of the three sets [see cmake/ads_sets.txt],
# ads.core, ads.base and ads.advanced, each a target of its own (e.g.,
# ADS::ADS_module_base, which compiles only the base headers and the core);
# they are built when ADS_BUILD_MODULE is ON, which needs CMake 3.28, a
# generator that scans the module dependencies and a compiler that builds
# modules. Where one is missing, the option is turned off with a warning
# [see ADSCheckModule.cmake].
option(ADS_BUILD_MODULE "build the C++20 module ads [see src/ADS.cppm]" OFF)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ADSCheckModule.cmake)
ads_check_module()
if (ADS_BUILD_MODULE)
    # the scan of the module dependencies, which cmake_minimum_required()
    # above leaves off
    cmake_policy(SET CMP0155 NEW)
    # the include directories and the macros of ADS also on each module
    # itself, as the importer of the installed one compiles it again with
    # these alone, CMake 3.28 dropping those of the libraries it links
    get_target_property(ADS_DEFINITIONS ADS INTERFACE_COMPILE_DEFINITIONS)
    # a module of ADS, the target <target> of src/<file>.cppm, which
    # imports the modules of the targets that follow
    function(ads_add_module target file)
        add_library(${target})
        target_sources(${target} PUBLIC
                FILE_SET CXX_MODULES
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
                FILES src/${file}.cppm)
        target_compile_features(${target} PUBLIC cxx_std_20)
        target_link_libraries(${target} PUBLIC ADS ${ARGN})
        target_include_directories(${target} PRIVATE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>)
        if (ADS_DEFINITIONS)
            target_compile_definitions(${target} PRIVATE ${ADS_DEFINITIONS})
        endif ()
        add_library(ADS::${target} ALIAS ${target})
    endfunction()
    ads_add_module(ADS_module_core ADSCore)
    # ads.base and ads.advanced export the names of the core without
    # importing ads.core [see cmake/ads_module_exports.py]
    ads_add_module(ADS_module_base ADSBase)
    ads_add_module(ADS_module_advanced ADSAdvanced)
    ads_add_module(ADS_module ADS ADS_module_core ADS_module_base
                   ADS_module_advanced)
endif ()

# ----- Subdirectories ------------------------------------------------------ #
# The tests, the exercises of the book, built by default only when ADS is
# the main project; include(CTest) creates the BUILD_TESTING option
# (default: ON) and enables the testing with the ctest executable.
# See: https://cmake.org/cmake/help/latest/manual/ctest.1.html
option(ADS_BUILD_TESTS "build the tests (by default only as the main project)"
       ${PROJECT_IS_TOP_LEVEL})
if (ADS_BUILD_TESTS)
    include(CTest)
    if (BUILD_TESTING)
        add_subdirectory(test)
    endif ()
endif ()

# ----- Python -------------------------------------------------------------- #
# The package of Python ads-python [see python/]: the extension module ads
# on nanobind and the backend nx-ads of NetworkX, with its tests of pytest
# in one ctest. AUTO, the default, builds it when a Python with its headers
# and nanobind are found, ON requires them, OFF leaves it out.
set(ADS_WITH_PYTHON AUTO CACHE STRING
    "the package of Python on nanobind: AUTO (if found), ON or OFF")
set_property(CACHE ADS_WITH_PYTHON PROPERTY STRINGS AUTO ON OFF)
string(TOUPPER "${ADS_WITH_PYTHON}" ADS_PYTHON_MODE)
set(ADS_USE_PYTHON OFF)
if (ADS_PYTHON_MODE STREQUAL "AUTO" OR ADS_WITH_PYTHON)
    find_package(Python 3.9 QUIET
                 COMPONENTS Interpreter Development.Module
                 OPTIONAL_COMPONENTS Development.SABIModule)
    if (Python_FOUND)
        execute_process(COMMAND ${Python_EXECUTABLE} -m nanobind --cmake_dir
                        OUTPUT_VARIABLE nanobind_ROOT
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        RESULT_VARIABLE _nanobind ERROR_QUIET)
        if (_nanobind EQUAL 0)
            find_package(nanobind CONFIG QUIET)
        endif ()
    endif ()
    if (nanobind_FOUND)
        set(ADS_USE_PYTHON ON)
    elseif (NOT ADS_PYTHON_MODE STREQUAL "AUTO")
        message(FATAL_ERROR "ADS_WITH_PYTHON is ON, but no Python with its "
                "headers and nanobind was found: install them (python3-dev "
                "and pip install nanobind) or set Python_EXECUTABLE")
    endif ()
endif ()
message(STATUS "ADS: Python ${ADS_USE_PYTHON}")
if (ADS_USE_PYTHON)
    add_subdirectory(python)
endif ()

# ----- Benchmarks ---------------------------------------------------------- #
# The benchmarks in bench/, against the libraries found, off by default.
option(ADS_BENCHMARKS "build the benchmarks in bench/" OFF)
if (ADS_BENCHMARKS)
    find_package(Boost QUIET)
    find_path(GMPXX_INCLUDE gmpxx.h)
    find_library(GMPXX_LIB gmpxx)
    find_library(GMP_LIB gmp)
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
    foreach (b sort dictionaries priority sequences memory bignum hashing
               search suffix edit numeric queues concurrent geometry matrix
               sparse external graphs combinatorial backtrack ordered arrays
               sets tries allocators positions assembly fingerprints multi lce
               parallel joins dynamic greedy learned stringdicts flows
               assignment runtime bestfirst hashattack ssets)
        add_executable(bench-${b} bench/${b}.cpp)
        target_link_libraries(bench-${b} PRIVATE ADS::ADS)
        # the code of ADS, which the benchmarks compare with Boost and with
        # LEMON, also where ADS::ADS and ADS::LEMON delegate to them
        target_compile_definitions(bench-${b} PRIVATE ADS_CONTRACTS=0
                ADS_POINTERTABLE_BOOST=0 ADS_FLATHASH_BOOST=0
                ADS_SMALLVECTOR_BOOST=0 ADS_RANDOM_BOOST=0
                ADS_MATCHING_LEMON=0 ADS_ASSIGNMENT_LEMON=0)
        if (Boost_FOUND)
            target_include_directories(bench-${b}
                                       PRIVATE ${Boost_INCLUDE_DIRS})
        endif ()
    endforeach ()
    # LEMON, the first competitor of the graphs, of the flows and of the
    # assignment, with ADS::LEMON
    if (ADS_USE_LEMON)
        foreach (b graphs flows assignment)
            target_link_libraries(bench-${b} PRIVATE ADS::LEMON)
        endforeach ()
    endif ()
    # the sets of Morin in ssets, from the C++ sources of Open Data
    # Structures in ODS_DIR
    set(ODS_DIR "" CACHE PATH "the C++ sources of Open Data Structures")
    if (ODS_DIR)
        target_compile_definitions(bench-ssets PRIVATE ADS_HAS_ODS)
        target_include_directories(bench-ssets PRIVATE ${ODS_DIR})
    endif ()
    # bignum and matrix on the code of ADS alone, when ADS::ADS runs on the
    # backends
    if (ADS_USE_GMP)
        add_executable(bench-bignum-own bench/bignum.cpp)
        target_link_libraries(bench-bignum-own PRIVATE ADS::ADS)
        target_compile_definitions(bench-bignum-own PRIVATE ADS_CONTRACTS=0
                ADS_BENCH_OWN ADS_BIGINT_GMP=0 ADS_RATIONAL_GMP=0)
    endif ()
    if (ADS_USE_BLAS)
        add_executable(bench-matrix-own bench/matrix.cpp)
        target_link_libraries(bench-matrix-own PRIVATE ADS::ADS)
        target_compile_definitions(bench-matrix-own PRIVATE ADS_CONTRACTS=0
                ADS_BENCH_OWN ADS_GEMM_BLAS=0)
    endif ()
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
    # libsais built with OpenMP, from the prefix ADS_LIBSAIS_OMP
    set(ADS_LIBSAIS_OMP "" CACHE PATH "libsais built with OpenMP")
    if (ADS_LIBSAIS_OMP AND OpenMP_CXX_FOUND)
        target_compile_definitions(bench-parallel PRIVATE ADS_HAS_LIBSAIS_OMP)
        target_include_directories(bench-parallel
                                   PRIVATE ${ADS_LIBSAIS_OMP}/include)
        target_link_libraries(bench-parallel
                              PRIVATE ${ADS_LIBSAIS_OMP}/lib/libsais.a)
    endif ()
    target_link_libraries(bench-parallel PRIVATE Threads::Threads)
    target_link_libraries(bench-runtime PRIVATE Threads::Threads)
    if (OpenMP_CXX_FOUND)
        target_link_libraries(bench-runtime PRIVATE OpenMP::OpenMP_CXX)
    endif ()
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
        # the blocked Bloom filter of fastfilter_cpp, where its header
        # compiles at the flags of the build (it needs AVX2)
        include(CheckCXXSourceCompiles)
        include(CMakePushCheckState)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_INCLUDES ${ADS_HASHING}/fastfilter_cpp/src)
        set(CMAKE_REQUIRED_QUIET ON)
        check_cxx_source_compiles("#include <bloom/simd-block.h>
            int main( void ) { SimdBlockFilter<> f( 10 ); f.Add( 1 );
                               return( f.Find( 1 ) ? 0 : 1 ); }"
                                  ADS_HAS_FASTFILTER)
        cmake_pop_check_state()
        if (ADS_HAS_FASTFILTER)
            target_compile_definitions(bench-hashing
                                       PRIVATE ADS_HAS_FASTFILTER)
        endif ()
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
        # x86-simd-sort, where its header compiles at the flags of the
        # build (it needs AVX2), and fluxsort and crumsort without the
        # strict aliasing [see bench/sort.cpp]
        include(CheckCXXSourceCompiles)
        include(CMakePushCheckState)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_INCLUDES ${ADS_SORTING}/x86-simd-sort/src)
        set(CMAKE_REQUIRED_QUIET ON)
        check_cxx_source_compiles("#include <x86simdsort-static-incl.h>
            int main( void ) { int a[ 2 ] = { 2 , 1 };
                               x86simdsortStatic::qsort( a , 2 );
                               return( a[ 0 ] - 1 ); }" ADS_HAS_XSS)
        cmake_pop_check_state()
        if (ADS_HAS_XSS)
            target_compile_definitions(bench-sort PRIVATE ADS_HAS_XSS)
        endif ()
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag(-fno-strict-aliasing ADS_HAS_NO_STRICT_ALIAS)
        if (ADS_HAS_NO_STRICT_ALIAS
            AND EXISTS ${ADS_SORTING}/fluxsort/src/fluxsort.h)
            target_compile_options(bench-sort PRIVATE -fno-strict-aliasing)
        endif ()
        check_cxx_compiler_flag(-mcx16 ADS_HAS_MCX16)
        if (ADS_HAS_MCX16)
            target_compile_options(bench-parallel PRIVATE -mcx16)
        endif ()
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

# ----- Install instructions ------------------------------------------------ #
# The following commands are used when installing the library and its CMake
# and pkg-config files on the system; they are not required for local
# builds. The CMake and pkg-config files go under share, not lib, since a
# library of headers is the same on every architecture.
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(ADS_INSTALL_CMAKEDIR ${CMAKE_INSTALL_DATADIR}/cmake/ADS
    CACHE STRING "where to install the files of find_package(ADS)")
set(ADS_INSTALL_PKGCONFIGDIR ${CMAKE_INSTALL_DATADIR}/pkgconfig
    CACHE STRING "where to install ads.pc")

# Install the library
install(TARGETS ADS EXPORT ADSTargets)

# Install the target of the interoperation with LEMON, in a file of its own
# that ADSConfig.cmake includes when LEMON is found again
if (ADS_USE_LEMON)
    install(TARGETS ADS_LEMON EXPORT ADSLemonTargets)
    install(EXPORT ADSLemonTargets
            NAMESPACE ADS::
            DESTINATION ${ADS_INSTALL_CMAKEDIR})
endif ()

# Install the modules, whose interfaces are compiled again by each importer
if (ADS_BUILD_MODULE)
    install(TARGETS ADS_module_core ADS_module_base ADS_module_advanced
            ADS_module EXPORT ADSTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            FILE_SET CXX_MODULES
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ADS/modules)
endif ()

# Install the headers, and the auto-generated configuration header (see
# above).
install(DIRECTORY include/ADS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ADS/ADSConfig.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ADS)

# (Generate and) install the target import file, that allows other
# CMake projects to import the target.
install(EXPORT ADSTargets
        NAMESPACE ADS::
        DESTINATION ${ADS_INSTALL_CMAKEDIR})

# Generate the package configuration file, that allows other CMake projects
# to find the library with find_package(), and the package version file,
# that allows them to know the version.
configure_package_config_file(cmake/ADSConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfig.cmake
        INSTALL_DESTINATION ${ADS_INSTALL_CMAKEDIR})
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfigVersion.cmake
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)

# Install the package version and configuration files, with the modules of
# GMP, of the CBLAS, of Boost, of hwloc, of Highway, of LEMON and of the
# runtimes of a program that the configuration file includes.
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfigVersion.cmake
        cmake/ADSGMP.cmake
        cmake/ADSBLAS.cmake
        cmake/ADSBoost.cmake
        cmake/ADSHwloc.cmake
        cmake/ADSHighway.cmake
        cmake/ADSLemon.cmake
        cmake/ADSLemonShim.cmake
        cmake/ADSInterop.cmake
        DESTINATION ${ADS_INSTALL_CMAKEDIR})

# Generate and install ads.pc: its prefix is found from where the file is
# (${pcfiledir} and one .. per directory of ADS_INSTALL_PKGCONFIGDIR) when
# that is relative, so that an install with --prefix moved elsewhere still
# points to its own headers; its include directory is ${prefix}/include
# when that is relative; with GMP it has the define and the library, by
# gmp.pc, with a CBLAS its define, its headers and its library, with Boost
# its define and its headers, with hwloc its define and hwloc.pc, and with
# Highway its define and its two libraries, by libhwy-contrib.pc and
# libhwy.pc.
if (IS_ABSOLUTE "${ADS_INSTALL_PKGCONFIGDIR}")
    set(ADS_PC_PREFIX "${CMAKE_INSTALL_PREFIX}")
else ()
    set(ADS_PC_PREFIX "\${pcfiledir}")
    string(REPLACE "/" ";" ADS_PC_DIRS "${ADS_INSTALL_PKGCONFIGDIR}")
    foreach (ADS_PC_DIR IN LISTS ADS_PC_DIRS)
        if (NOT ADS_PC_DIR STREQUAL "" AND NOT ADS_PC_DIR STREQUAL ".")
            string(APPEND ADS_PC_PREFIX "/..")
        endif ()
    endforeach ()
endif ()
if (IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(ADS_PC_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
else ()
    set(ADS_PC_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif ()
if (ADS_USE_GMP)
    set(ADS_PC_REQUIRES "gmp")
    string(APPEND ADS_PC_CFLAGS " -DADS_WITH_GMP")
endif ()
if (ADS_USE_BLAS)
    string(APPEND ADS_PC_CFLAGS " -DADS_WITH_BLAS ${ADS_BLAS_PC_CFLAGS}")
    set(ADS_PC_LIBS "${ADS_BLAS_PC_LIBS}")
endif ()
if (ADS_USE_BOOST)
    string(APPEND ADS_PC_CFLAGS " -DADS_WITH_BOOST${ADS_BOOST_PC_CFLAGS}")
endif ()
if (ADS_USE_HWLOC)
    string(APPEND ADS_PC_REQUIRES " hwloc")
    string(APPEND ADS_PC_CFLAGS " -DADS_WITH_HWLOC")
endif ()
if (ADS_USE_HIGHWAY)
    string(APPEND ADS_PC_REQUIRES " libhwy-contrib libhwy")
    string(APPEND ADS_PC_CFLAGS " -DADS_WITH_HIGHWAY")
endif ()
configure_file(cmake/ads.pc.in ${CMAKE_CURRENT_BINARY_DIR}/ads.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ads.pc
        DESTINATION ${ADS_INSTALL_PKGCONFIGDIR})

# Generate and install ads-lemon.pc, ads.pc with the define and the flags of
# LEMON, when the target ADS::LEMON is built.
if (ADS_USE_LEMON)
    configure_file(cmake/ads-lemon.pc.in
                   ${CMAKE_CURRENT_BINARY_DIR}/ads-lemon.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ads-lemon.pc
            DESTINATION ${ADS_INSTALL_PKGCONFIGDIR})
endif ()

# --------------------------------------------------------------------------- #
