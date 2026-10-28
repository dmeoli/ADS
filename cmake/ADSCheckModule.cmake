# --------------------------------------------------------------------------- #
#    ADSCheckModule.cmake                                                     #
#                                                                             #
#    Checks whether the C++20 module ads can be built, and turns              #
#    ADS_BUILD_MODULE off, with a warning that says why, if it cannot: a      #
#    build that asked for the module then goes on with the headers alone, as  #
#    when the option is OFF, instead of failing halfway. The module needs     #
#      - CMake 3.28 or later;                                                 #
#      - the Ninja generator (Ninja 1.11 or later) or a Visual Studio one;    #
#      - Clang 16 or later with clang-scan-deps, or GCC 15 or later: GCC 14   #
#        stops with an internal error on the module, and MSVC discards the    #
#        operators of the standard library (e.g., those of std::complex) that #
#        the templates of the module use only in their instantiations.        #
#                                                                             #
#      include(cmake/ADSCheckModule.cmake)                                    #
#      ads_check_module()                                                     #
#      if (ADS_BUILD_MODULE)                                                  #
#          ...                                                                #
# --------------------------------------------------------------------------- #

function(ads_check_module)
    if (NOT ADS_BUILD_MODULE)
        return()
    endif ()

    set(_why "")
    if (CMAKE_VERSION VERSION_LESS 3.28)
        set(_why "CMake ${CMAKE_VERSION} is older than 3.28")
    elseif (CMAKE_GENERATOR MATCHES "^Ninja")
        execute_process(COMMAND "${CMAKE_MAKE_PROGRAM}" --version
                OUTPUT_VARIABLE _ninja OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE _res)
        if (NOT _res EQUAL 0 OR _ninja VERSION_LESS 1.11)
            set(_why "Ninja ${_ninja} is older than 1.11")
        endif ()
    elseif (NOT CMAKE_GENERATOR MATCHES "^Visual Studio")
        set(_why "the generator ${CMAKE_GENERATOR} does not build modules \
(Ninja or Visual Studio is needed)")
    endif ()

    if (NOT _why)
        set(_id "${CMAKE_CXX_COMPILER_ID}")
        set(_ver "${CMAKE_CXX_COMPILER_VERSION}")
        if (NOT ((_id STREQUAL "Clang" AND _ver VERSION_GREATER_EQUAL 16)
                 OR (_id STREQUAL "GNU" AND _ver VERSION_GREATER_EQUAL 15)))
            set(_why "the compiler ${_id} ${_ver} does not build the module \
(Clang 16 or GCC 15 or later is needed)")
        elseif (_id STREQUAL "Clang"
                AND NOT CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS)
            set(_why "clang-scan-deps, which scans the module dependencies \
for Clang, is not there (e.g., it is in clang-tools on Debian and Ubuntu)")
        endif ()
    endif ()

    if (_why)
        message(WARNING "ADS_BUILD_MODULE is turned OFF: ${_why}")
        set(ADS_BUILD_MODULE OFF PARENT_SCOPE)
    endif ()
endfunction()

# --------------------------------------------------------------------------- #
