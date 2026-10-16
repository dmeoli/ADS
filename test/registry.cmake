###############################################################################
#                            registry.cmake                                   #
#                                                                             #
# Checks that the tests of PROGRAM (as PROGRAM -l lists them) are those that  #
# LIST gives for NAME, so that no test is run by one build and not by the     #
# other, nor written and never run.                                           #
###############################################################################

execute_process(COMMAND ${PROGRAM} -l OUTPUT_VARIABLE out
                RESULT_VARIABLE rc)
if (NOT rc EQUAL 0)
    message(FATAL_ERROR "${PROGRAM} -l failed")
endif ()
string(REPLACE "\n" ";" in_program "${out}")
list(FILTER in_program EXCLUDE REGEX "^$")
list(SORT in_program)

file(STRINGS ${LIST} lines REGEX "^${NAME} ")
set(in_list "")
foreach (line IN LISTS lines)
    string(REGEX MATCH "^[^ ]+ +([^ ]+)" _ "${line}")
    list(APPEND in_list ${CMAKE_MATCH_1})
endforeach ()
list(SORT in_list)

if (NOT in_program STREQUAL in_list)
    message(FATAL_ERROR "the tests of ${NAME} differ:\n"
            "  in the program: ${in_program}\n"
            "  in ${LIST}: ${in_list}")
endif ()
