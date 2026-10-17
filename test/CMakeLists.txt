###############################################################################
#                           test/CMakeLists.txt                               #
#                                                                             #
# One program per chapter of the book, chNN.cpp, and one ctest per test in    #
# it, read from tests.txt as <program>.<name>; <program>.registry checks      #
# that tests.txt and the tests in the program are the same.                   #
###############################################################################

file(STRINGS tests.txt lines REGEX "^[a-z]")
set(programs "")
foreach (line IN LISTS lines)
    string(REGEX MATCH "^([^ ]+) +([^ ]+)" _ "${line}")
    set(program ${CMAKE_MATCH_1})
    set(name ${CMAKE_MATCH_2})
    if (NOT program IN_LIST programs)
        list(APPEND programs ${program})
        add_executable(${program} ${program}.cpp)
        target_link_libraries(${program} PRIVATE ADS::ADS)
        # the tests check all the contracts, invariants included
        target_compile_definitions(${program} PRIVATE ADS_CONTRACTS=3)
        target_compile_options(${program} PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)
        add_test(NAME ${program}.registry
                 COMMAND ${CMAKE_COMMAND}
                         -DPROGRAM=$<TARGET_FILE:${program}>
                         -DNAME=${program}
                         -DLIST=${CMAKE_CURRENT_SOURCE_DIR}/tests.txt
                         -P ${CMAKE_CURRENT_SOURCE_DIR}/registry.cmake)
    endif ()
    add_test(NAME ${program}.${name} COMMAND ${program} ${name})
endforeach ()

###############################################################################
#                       End of test/CMakeLists.txt                            #
###############################################################################
