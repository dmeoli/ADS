###############################################################################
#                           test/CMakeLists.txt                               #
#                                                                             #
# One program per chapter of the book, chNN.cpp, and one ctest per test in    #
# it, read from tests.txt as <program>.<name>; <program>.registry checks      #
# that tests.txt and the tests in the program are the same.                   #
###############################################################################

find_package(Threads REQUIRED)

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
        # the tests check all the contracts, invariants included
        target_compile_definitions(${program} PRIVATE ADS_CONTRACTS=3)
        target_compile_options(${program} PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
                $<$<CXX_COMPILER_ID:MSVC>:/W3 /bigobj /permissive- /utf-8>)
        # the stack of Linux, 8 MB, for the recursive solutions of the
        # exercises on chains of 10^4 nodes (1 MB by default on Windows)
        target_link_options(${program} PRIVATE
                $<$<CXX_COMPILER_ID:MSVC>:/STACK:8388608>)
        add_test(NAME ${program}.registry
                 COMMAND ${CMAKE_COMMAND}
                         -DPROGRAM=$<TARGET_FILE:${program}>
                         -DNAME=${program}
                         -DLIST=${CMAKE_CURRENT_SOURCE_DIR}/tests.txt
                         -P ${CMAKE_CURRENT_SOURCE_DIR}/registry.cmake)
    endif ()
    add_test(NAME ${program}.${name} COMMAND ${program} ${name})
endforeach ()

# the sort over the processes of MPI, not in tests.txt: on 1 to 4
# processes
if (ADS_WITH_MPI)
    add_executable(engineered_mpi engineered_mpi.cpp)
    target_link_libraries(engineered_mpi PRIVATE ADS::ADS Threads::Threads)
    target_compile_definitions(engineered_mpi PRIVATE ADS_CONTRACTS=3)
    foreach (np 1 2 3 4)
        add_test(NAME engineered_mpi.distributed-sort.np${np}
                 COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${np}
                         ${MPIEXEC_PREFLAGS} $<TARGET_FILE:engineered_mpi>
                         ${MPIEXEC_POSTFLAGS} distributed-sort)
    endforeach ()
endif ()

###############################################################################
#                       End of test/CMakeLists.txt                            #
###############################################################################
