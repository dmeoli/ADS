# --------------------------------------------------------------------------- #
#    python/CMakeLists.txt                                                    #
#                                                                             #
#    The package of Python ads-python: the extension module ads._core on      #
#    nanobind, the package ads around it and the backend nx_ads of NetworkX.  #
#    It is built by the main CMakeLists.txt when ADS_WITH_PYTHON is on, and   #
#    by scikit-build-core from pyproject.toml (pip install python/), which    #
#    takes ADS from the directory above, or installed.                        #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.21)
if (PROJECT_IS_TOP_LEVEL OR NOT DEFINED PROJECT_NAME)
    project(ads_python LANGUAGES CXX)
endif ()

# ADS: the target of the build that includes this directory, or the one of
# the directory above, or the installed one
if (NOT TARGET ADS::ADS)
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../include/ADS/ADS.h)
        set(ADS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(ADS_WITH_PYTHON OFF CACHE STRING "" FORCE)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ads EXCLUDE_FROM_ALL)
    else ()
        find_package(ADS REQUIRED)
    endif ()
endif ()

# nanobind: the one of the Python that builds, found by its module
if (NOT Python_FOUND)
    find_package(Python 3.9 REQUIRED
                 COMPONENTS Interpreter Development.Module
                 OPTIONAL_COMPONENTS Development.SABIModule)
endif ()
if (NOT nanobind_FOUND)
    execute_process(COMMAND ${Python_EXECUTABLE} -m nanobind --cmake_dir
                    OUTPUT_VARIABLE nanobind_ROOT
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    find_package(nanobind CONFIG REQUIRED)
endif ()

# the module, on the stable ABI of Python 3.12 and later (one wheel for
# all of them), with the preconditions of ADS checked, so that a wrong
# argument raises ContractViolation instead of breaking a structure
nanobind_add_module(_core STABLE_ABI NB_STATIC
        src/module.cpp src/graphs.cpp src/dynamic.cpp src/succinct.cpp
        src/filters.cpp src/heaps.cpp)
target_include_directories(_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(_core PRIVATE ADS::ADS)
target_compile_features(_core PRIVATE cxx_std_20)
target_compile_definitions(_core PRIVATE ADS_CONTRACTS=1)
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-Wall -Wextra" ADS_PY_HAS_WALL)
if (ADS_PY_HAS_WALL)
    target_compile_options(_core PRIVATE -Wall -Wextra)
endif ()

# the package in the build directory, as it is installed: the module, the
# files of Python, the stubs, and the metadata by which NetworkX finds the
# backend, so that the tests run on it with no installation
set(ADS_PY_ROOT ${CMAKE_CURRENT_BINARY_DIR}/site)
set_target_properties(_core PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${ADS_PY_ROOT}/ads)
set(ADS_PY_SOURCES ads/__init__.py ads/py.typed nx_ads/__init__.py
        nx_ads/backend.py)
set(ADS_PY_COPIES "")
foreach (f IN LISTS ADS_PY_SOURCES)
    add_custom_command(OUTPUT ${ADS_PY_ROOT}/${f}
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_SOURCE_DIR}/${f} ${ADS_PY_ROOT}/${f}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${f})
    list(APPEND ADS_PY_COPIES ${ADS_PY_ROOT}/${f})
endforeach ()
add_custom_target(ads_python_files ALL DEPENDS ${ADS_PY_COPIES})
file(WRITE ${ADS_PY_ROOT}/ads_python-0.dist-info/METADATA
     "Metadata-Version: 2.1\nName: ads-python\nVersion: 0\n")
file(WRITE ${ADS_PY_ROOT}/ads_python-0.dist-info/entry_points.txt
     "[networkx.backends]\nads = nx_ads.backend:BackendInterface\n\n"
     "[networkx.backend_info]\nads = nx_ads:get_info\n")

# the stubs of the module, written by the stubgen of nanobind
nanobind_add_stub(_core_stub MODULE ads._core
        OUTPUT ${ADS_PY_ROOT}/ads/_core.pyi
        PYTHON_PATH ${ADS_PY_ROOT}
        DEPENDS _core ads_python_files)

# the installation of scikit-build-core
if (DEFINED SKBUILD)
    install(TARGETS _core LIBRARY DESTINATION ads)
    install(FILES ${ADS_PY_ROOT}/ads/_core.pyi DESTINATION ads)
endif ()

# the tests of pytest, one ctest, where pytest is found (those that need
# NetworkX, SciPy or pyroaring skip without them); and the examples of the
# manual, the second where NetworkX is found
if (BUILD_TESTING AND NOT DEFINED SKBUILD)
    execute_process(COMMAND ${Python_EXECUTABLE} -c "import pytest"
                    RESULT_VARIABLE ADS_PY_NO_PYTEST OUTPUT_QUIET ERROR_QUIET)
    execute_process(COMMAND ${Python_EXECUTABLE} -c "import networkx"
                    RESULT_VARIABLE ADS_PY_NO_NETWORKX OUTPUT_QUIET
                    ERROR_QUIET)
    set(ADS_PY_TESTS python.example.structures)
    if (ADS_PY_NO_PYTEST EQUAL 0)
        add_test(NAME python.pytest
                 COMMAND ${Python_EXECUTABLE} -m pytest -q -p no:cacheprovider
                         ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        list(APPEND ADS_PY_TESTS python.pytest)
    endif ()
    set(ADS_PY_EXAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    add_test(NAME python.example.structures
             COMMAND ${Python_EXECUTABLE} ${ADS_PY_EXAMPLES}/structures.py)
    if (ADS_PY_NO_NETWORKX EQUAL 0)
        add_test(NAME python.example.networkx_backend
                 COMMAND ${Python_EXECUTABLE}
                         ${ADS_PY_EXAMPLES}/networkx_backend.py)
        list(APPEND ADS_PY_TESTS python.example.networkx_backend)
    endif ()
    set_tests_properties(${ADS_PY_TESTS} PROPERTIES
            ENVIRONMENT "PYTHONPATH=${ADS_PY_ROOT}")
endif ()
