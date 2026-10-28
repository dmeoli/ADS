# --------------------------------------------------------------------------- #
#    packaging/test/CMakeLists.txt                                            #
#                                                                             #
#    A project that uses an installed ADS through find_package(ADS), and,     #
#    when pkg-config is there, also through ads.pc, and through the C++20     #
#    module ads when the package has it: the recipes of packaging/ build it   #
#    against the package they made and run it.                                #
#                                                                             #
#      cmake -S packaging/test -B build-use -DCMAKE_PREFIX_PATH=<prefix>      #
#      cmake --build build-use && ctest --test-dir build-use                  #
# --------------------------------------------------------------------------- #

cmake_minimum_required(VERSION 3.21)
project(ADSUse LANGUAGES CXX)

find_package(ADS 0.1 REQUIRED)
add_executable(use-cmake main.cpp)
target_link_libraries(use-cmake PRIVATE ADS::ADS)

enable_testing()
add_test(NAME use-cmake COMMAND use-cmake)

# the module ads, when the package has it [see module.cpp]
if (TARGET ADS::ADS_module)
    cmake_minimum_required(VERSION 3.28)
    add_executable(use-module module.cpp)
    target_link_libraries(use-module PRIVATE ADS::ADS_module)
    add_test(NAME use-module COMMAND use-module)
endif ()

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(ADSPC REQUIRED IMPORTED_TARGET ads)
    add_executable(use-pkgconfig main.cpp)
    target_link_libraries(use-pkgconfig PRIVATE PkgConfig::ADSPC)
    target_compile_features(use-pkgconfig PRIVATE cxx_std_20)
    add_test(NAME use-pkgconfig COMMAND use-pkgconfig)
endif ()

# --------------------------------------------------------------------------- #
