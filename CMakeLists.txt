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

# ----- the library: headers only ------------------------------------------- #
add_library(ADS INTERFACE)
add_library(ADS::ADS ALIAS ADS)
target_include_directories(ADS INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_compile_features(ADS INTERFACE cxx_std_20)

# ----- the tests: the exercises of the book -------------------------------- #
include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
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
