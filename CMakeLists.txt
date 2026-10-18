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
    foreach (b sort dictionaries priority sequences bignum fft)
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
endif ()

# ----- install and export, for find_package(ADS) --------------------------- #
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ADS EXPORT ADSTargets)
install(DIRECTORY include/ADS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT ADSTargets
        NAMESPACE ADS::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ADS)

configure_package_config_file(cmake/ADSConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ADS)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfigVersion.cmake
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ADSConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ADS)

###############################################################################
#                          End of CMakeLists.txt                              #
###############################################################################
