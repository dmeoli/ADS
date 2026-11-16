# --------------------------------------------------------------------------- #
#    test/CMakeLists.txt                                                      #
#                                                                             #
#    One program per chapter of the book, chNN.cpp, and one ctest per test in #
#    it, read from tests.txt as <program>.<name>; <program>.registry checks   #
#    that tests.txt and the tests in the program are the same.                #
# --------------------------------------------------------------------------- #

find_package(Threads REQUIRED)

# the options of the warnings and of the objects, each set where the compiler
# takes it (the spelling of GCC and Clang, or the one of MSVC), and the stack
# of Linux, 8 MB, for the recursive solutions of the exercises on chains of
# 10^4 nodes (1 MB by default with MSVC, 2 MB with MinGW), where the linker
# sets it; the objects of more than 2^16 sections, which the format COFF
# takes only in its big variant, where the assembler has an option for it
# (Clang switches to it by itself, the assembler of GCC by -mbig-obj)
include(CheckCXXCompilerFlag)
include(CheckLinkerFlag)
set(ADS_TEST_WARNINGS "")
set(ADS_TEST_LINK "")
check_cxx_compiler_flag("-Wall -Wextra" ADS_HAS_WALL)
if (ADS_HAS_WALL)
    list(APPEND ADS_TEST_WARNINGS -Wall -Wextra)
endif ()
check_cxx_compiler_flag("/W3 /permissive- /utf-8" ADS_HAS_W3)
if (ADS_HAS_W3)
    list(APPEND ADS_TEST_WARNINGS /W3 /permissive- /utf-8)
endif ()
set(ADS_TEST_OPTIONS ${ADS_TEST_WARNINGS})
check_cxx_compiler_flag(/bigobj ADS_HAS_BIGOBJ)
if (ADS_HAS_BIGOBJ)
    list(APPEND ADS_TEST_OPTIONS /bigobj)
endif ()
check_cxx_compiler_flag(-Wa,-mbig-obj ADS_HAS_MBIGOBJ)
if (ADS_HAS_MBIGOBJ)
    list(APPEND ADS_TEST_OPTIONS -Wa,-mbig-obj)
endif ()
check_linker_flag(CXX "LINKER:/STACK:8388608" ADS_HAS_STACK)
check_linker_flag(CXX "LINKER:--stack,8388608" ADS_HAS_GNU_STACK)
if (ADS_HAS_STACK)
    list(APPEND ADS_TEST_LINK "LINKER:/STACK:8388608")
elseif (ADS_HAS_GNU_STACK)
    list(APPEND ADS_TEST_LINK "LINKER:--stack,8388608")
endif ()

file(STRINGS tests.txt lines REGEX "^[a-z]")
set(programs "")
foreach (line IN LISTS lines)
    string(REGEX MATCH "^([^ ]+) +([^ ]+)" _ "${line}")
    set(program ${CMAKE_MATCH_1})
    set(name ${CMAKE_MATCH_2})
    if (NOT program IN_LIST programs)
        list(APPEND programs ${program})
        add_executable(${program} ${program}.cpp)
        target_link_libraries(${program} PRIVATE ADS::ADS Threads::Threads)
        # with LEMON, the tests run on the solves that ADS delegates to it,
        # and engineered_lemon on the interoperation
        if (ADS_USE_LEMON)
            target_link_libraries(${program} PRIVATE ADS::LEMON)
        endif ()
        # the tests check all the contracts, invariants included
        target_compile_definitions(${program} PRIVATE ADS_CONTRACTS=3)
        # the options above
        target_compile_options(${program} PRIVATE ${ADS_TEST_OPTIONS})
        target_link_options(${program} PRIVATE ${ADS_TEST_LINK})
        add_test(NAME ${program}.registry
                 COMMAND ${CMAKE_COMMAND}
                         -DPROGRAM=$<TARGET_FILE:${program}>
                         -DNAME=${program}
                         -DLIST=${CMAKE_CURRENT_SOURCE_DIR}/tests.txt
                         -P ${CMAKE_CURRENT_SOURCE_DIR}/registry.cmake)
    endif ()
    add_test(NAME ${program}.${name} COMMAND ${program} ${name})
endforeach ()

# the sort and the runtime over the processes of MPI, not in tests.txt:
# on 1 to 4 processes
if (ADS_USE_MPI)
    add_executable(engineered_mpi engineered_mpi.cpp)
    target_link_libraries(engineered_mpi PRIVATE ADS::ADS Threads::Threads)
    target_compile_definitions(engineered_mpi PRIVATE ADS_CONTRACTS=3)
    foreach (t distributed-sort distributed-sort-counts mpi-executor
               mpi-mailbox mpi-termination mpi-min mpi-bytes mpi-codec
               mpi-stop)
        foreach (np 1 2 3 4)
            add_test(NAME engineered_mpi.${t}.np${np}
                     COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG}
                             ${np} ${MPIEXEC_PREFLAGS}
                             $<TARGET_FILE:engineered_mpi>
                             ${MPIEXEC_POSTFLAGS} ${t})
        endforeach ()
    endforeach ()
endif ()

# the executors on the runtimes of a program found [see ADSInterop.cmake]
# under the parallel algorithms, their own tests [see InteropExecutors.h and
# interop_parallel.cpp] and the examples of appendix C of the manual [see
# interop_examples.cpp], the last in C++23 where the compiler takes it (but
# MSVC, whose library has no std::flat_map), for std::flat_map
foreach (program engineered_parallel engineered_speculative interop_parallel
                 interop_examples)
    foreach (r IN LISTS ADS_INTEROP_BUILT)
        if (NOT r STREQUAL "BoostMPI" AND NOT r STREQUAL "cereal")
            target_link_libraries(${program} PRIVATE ADS::${r})
        endif ()
    endforeach ()
endforeach ()

if (cxx_std_23 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    target_compile_features(interop_examples PRIVATE cxx_std_23)
endif ()

# the examples of MPI and of Boost.MPI of appendix C of the manual, not in
# tests.txt: on 1 to 4 processes [see interop_examples_mpi.cpp]
if (ADS_USE_MPI)
    add_executable(interop_examples_mpi interop_examples_mpi.cpp)
    target_link_libraries(interop_examples_mpi PRIVATE ADS::ADS
                          Threads::Threads)
    if ("BoostMPI" IN_LIST ADS_INTEROP_BUILT)
        target_link_libraries(interop_examples_mpi PRIVATE ADS::BoostMPI)
    endif ()
    target_compile_definitions(interop_examples_mpi PRIVATE ADS_CONTRACTS=3)
    foreach (t mpi-sort boost-mpi-sort)
        foreach (np 1 2 3 4)
            add_test(NAME interop_examples_mpi.${t}.np${np}
                     COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG}
                             ${np} ${MPIEXEC_PREFLAGS}
                             $<TARGET_FILE:interop_examples_mpi>
                             ${MPIEXEC_POSTFLAGS} ${t})
        endforeach ()
    endforeach ()
endif ()

# the archives of cereal under the persistence, where they are found [see
# engineered_persistence.cpp]
if ("cereal" IN_LIST ADS_INTEROP_BUILT)
    target_link_libraries(engineered_persistence PRIVATE ADS::cereal)
endif ()

# the processes of MPI in a program of Boost.MPI, not in tests.txt: on 1 to
# 4 processes [see engineered_boost_mpi.cpp]
if ("BoostMPI" IN_LIST ADS_INTEROP_BUILT)
    add_executable(engineered_boost_mpi engineered_boost_mpi.cpp)
    target_link_libraries(engineered_boost_mpi PRIVATE ADS::BoostMPI
                          Threads::Threads)
    target_compile_definitions(engineered_boost_mpi PRIVATE ADS_CONTRACTS=3)
    foreach (t boost-mpi-sort boost-mpi-codec)
        foreach (np 1 2 3 4)
            add_test(NAME engineered_boost_mpi.${t}.np${np}
                     COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG}
                             ${np} ${MPIEXEC_PREFLAGS}
                             $<TARGET_FILE:engineered_boost_mpi>
                             ${MPIEXEC_POSTFLAGS} ${t})
        endforeach ()
    endforeach ()
endif ()

# the configurations of the layouts [see Abi.h and abi_units.cpp]: one
# program from two units of abi_unit.cpp, the first without the vector
# matching and the compare-and-swap of 16 bytes, the second with the
# defaults of the target and -mcx16 where the compiler takes it
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-mcx16 ADS_HAS_MCX16)
add_library(abi_unit_a OBJECT abi_unit.cpp)
target_compile_definitions(abi_unit_a PRIVATE ADS_ABI_UNIT=A ADS_FLAT_SIMD=0
                           ADS_FLAT_CAS16=0)
add_library(abi_unit_b OBJECT abi_unit.cpp)
target_compile_definitions(abi_unit_b PRIVATE ADS_ABI_UNIT=B)
if (ADS_HAS_MCX16)
    target_compile_options(abi_unit_b PRIVATE -mcx16)
endif ()
add_executable(abi_units abi_units.cpp $<TARGET_OBJECTS:abi_unit_a>
                         $<TARGET_OBJECTS:abi_unit_b>)
foreach (t abi_unit_a abi_unit_b abi_units)
    target_link_libraries(${t} PRIVATE ADS::ADS Threads::Threads)
    target_compile_options(${t} PRIVATE ${ADS_TEST_WARNINGS})
endforeach ()
add_test(NAME abi-units COMMAND abi_units)

# the order of the includes of FastFlowExecutor.h, a test of the compiler
# [see fastflow_order.cpp]: the build of a program that includes a header of
# FastFlow first must fail with the error of the header
add_executable(fastflow_order EXCLUDE_FROM_ALL fastflow_order.cpp)
target_link_libraries(fastflow_order PRIVATE ADS::ADS)
add_test(NAME fastflow-order
         COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                 --target fastflow_order --config $<CONFIG>)
set_tests_properties(fastflow-order PROPERTIES
        PASS_REGULAR_EXPRESSION "included after a header of FastFlow")

# the wrong attachments of an observer, a test of the compiler [see
# modification_attach.cpp]: each case must fail with the error of the
# subject
set(attach_errors
    "has an on\\(\\) for none of the events"
    "must be noexcept"
    "requires an event that the structure"
    "precedes one it depends on")
foreach (c 1 2 3 4)
    math(EXPR i "${c} - 1")
    list(GET attach_errors ${i} err)
    add_executable(modification_attach_${c} EXCLUDE_FROM_ALL
                   modification_attach.cpp)
    target_link_libraries(modification_attach_${c} PRIVATE ADS::ADS)
    target_compile_definitions(modification_attach_${c} PRIVATE
                               ADS_ATTACH_CASE=${c})
    add_test(NAME modification-attach.${c}
             COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                     --target modification_attach_${c} --config $<CONFIG>)
    set_tests_properties(modification-attach.${c} PROPERTIES
            PASS_REGULAR_EXPRESSION "${err}")
endforeach ()

# the module ads, imported in place of the headers [see module.cpp], and
# those of the sets, each linked alone with ads.core [see module_sets.cpp]
if (ADS_BUILD_MODULE)
    add_executable(module module.cpp)
    target_link_libraries(module PRIVATE ADS::ADS_module)
    add_test(NAME module COMMAND module)
    set(_n 0)
    foreach (s IN ITEMS core base advanced)
        math(EXPR _n "${_n} + 1")
        add_executable(module_${s} module_sets.cpp)
        target_link_libraries(module_${s} PRIVATE ADS::ADS_module_${s})
        target_compile_definitions(module_${s} PRIVATE ADS_MODULE_SET=${_n})
        add_test(NAME module.${s} COMMAND module_${s})
    endforeach ()
endif ()

# each entry point of the sets compiled alone, with the headers of ADS that
# it pulls checked against cmake/ads_sets.txt, and the files that the sets
# write up to date [see cmake/ads_sets.py]
# where the compiler takes the options of the script, -fsyntax-only and -H
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/sets_probe.cpp "int main() {}\n")
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -std=c++20 -fsyntax-only -H
                        ${CMAKE_CURRENT_BINARY_DIR}/sets_probe.cpp
                RESULT_VARIABLE _ads_sets_probe OUTPUT_QUIET ERROR_QUIET)
if (Python3_FOUND AND _ads_sets_probe EQUAL 0)
    foreach (s IN ITEMS core base advanced)
        add_test(NAME sets.${s}
                 COMMAND ${Python3_EXECUTABLE}
                         ${PROJECT_SOURCE_DIR}/cmake/ads_sets.py --test ${s}
                         --cxx ${CMAKE_CXX_COMPILER} --
                         -I${PROJECT_BINARY_DIR}/include)
    endforeach ()
    add_test(NAME sets.check
             COMMAND ${Python3_EXECUTABLE}
                     ${PROJECT_SOURCE_DIR}/cmake/ads_sets.py --check)
endif ()

# --------------------------------------------------------------------------- #
