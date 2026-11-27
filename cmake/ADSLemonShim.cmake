# --------------------------------------------------------------------------- #
#    ADSLemonShim.cmake                                                       #
#                                                                             #
#    The warm start of the algorithms of abLEMON [see                         #
#    include/ADS/interop/lemon/Warm.h]: ads_lemon_shim(<out> <inc> <src>      #
#    <ok>) writes into <out>/lemon the headers of abLEMON in <inc>/lemon that #
#    the rules of <src>/rewrites.txt name, rewritten by them, the same as     #
#    <src>/rewrite.awk of the makefiles: each rule is header|anchor|what, the #
#    anchor a whole line that has to be there exactly once, and what is <file #
#    (the file of <src> inserted before the line), +text (appended to the     #
#    line) or =text (in place of the line). <ok> is TRUE if every header was  #
#    written, FALSE, with the line missing in <ok>_WHY, otherwise, and        #
#    nothing is then written. <out> goes on the include path before <inc>.    #
#    CMakeLists.txt and ADSConfig.cmake include it.                           #
# --------------------------------------------------------------------------- #

function(ads_lemon_shim out inc src ok)
    set(${ok} FALSE PARENT_SCOPE)
    # the ';' of the rules kept out of the way of the CMake lists
    file(READ ${src}/rewrites.txt _rules)
    string(REPLACE ";" "<semicolon>" _rules "${_rules}")
    string(REGEX REPLACE "#[^\n]*\n" "" _rules "${_rules}")
    string(REPLACE "\n" ";" _rules "${_rules}")
    list(REMOVE_ITEM _rules "")
    set(_hdrs "")
    foreach (_rule IN LISTS _rules)
        string(FIND "${_rule}" "|" _p)
        string(SUBSTRING "${_rule}" 0 ${_p} _hdr)
        list(APPEND _hdrs ${_hdr})
    endforeach ()
    list(REMOVE_DUPLICATES _hdrs)
    set(_texts "")
    foreach (_hdr IN LISTS _hdrs)
        if (NOT EXISTS ${inc}/lemon/${_hdr})
            set(${ok}_WHY "no lemon/${_hdr} in ${inc}" PARENT_SCOPE)
            return()
        endif ()
        file(READ ${inc}/lemon/${_hdr} _text)
        set(_text "\n${_text}")
        foreach (_rule IN LISTS _rules)
            string(FIND "${_rule}" "|" _p)
            string(SUBSTRING "${_rule}" 0 ${_p} _rule_hdr)
            if (NOT _rule_hdr STREQUAL _hdr)
                continue()
            endif ()
            math(EXPR _p "${_p} + 1")
            string(SUBSTRING "${_rule}" ${_p} -1 _rest)
            string(FIND "${_rest}" "|" _p)
            string(SUBSTRING "${_rest}" 0 ${_p} _anchor)
            math(EXPR _p "${_p} + 1")
            string(SUBSTRING "${_rest}" ${_p} 1 _op)
            math(EXPR _p "${_p} + 1")
            string(SUBSTRING "${_rest}" ${_p} -1 _arg)
            string(REPLACE "<semicolon>" ";" _anchor "${_anchor}")
            string(REPLACE "<semicolon>" ";" _arg "${_arg}")
            string(FIND "${_text}" "\n${_anchor}\n" _at)
            string(FIND "${_text}" "\n${_anchor}\n" _last REVERSE)
            if (_at EQUAL -1 OR NOT _at EQUAL _last)
                set(${ok}_WHY "lemon/${_hdr} has not one line \"${_anchor}\" "
                    "[see ADS/interop/lemon/shim/rewrites.txt]" PARENT_SCOPE)
                return()
            endif ()
            if (_op STREQUAL "<")
                file(READ ${src}/${_arg} _block)
                set(_new "\n${_block}${_anchor}\n")
            elseif (_op STREQUAL "+")
                set(_new "\n${_anchor}${_arg}\n")
            else ()
                set(_new "\n${_arg}\n")
            endif ()
            string(REPLACE "\n${_anchor}\n" "${_new}" _text "${_text}")
        endforeach ()
        string(SUBSTRING "${_text}" 1 -1 _text)
        list(APPEND _texts "${_hdr}")
        set(_text_${_hdr} "${_text}")
    endforeach ()
    # every header rewritten in full, then written
    foreach (_hdr IN LISTS _texts)
        file(WRITE ${out}/lemon/${_hdr} "${_text_${_hdr}}")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                     ${inc}/lemon/${_hdr})
    endforeach ()
    # a change of the rules or of the blocks writes the headers again
    file(GLOB _files ${src}/*)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_files})
    set(${ok} TRUE PARENT_SCOPE)
endfunction()

# --------------------------------------------------------------------------- #
