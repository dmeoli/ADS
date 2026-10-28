# --------------------------------------------------------------------------- #
#    DeriveVersion.cmake                                                      #
#                                                                             #
#    Derives the version of ADS from git, so that a release is just the tag   #
#    vX.Y.Z: the version of project(), of ADSConfig.h, of ads.pc and of the   #
#    package of find_package(ADS) all follow the tag.                         #
#                                                                             #
#    The first of these that gives a numeric x[.y[.z]] wins:                  #
#      1. a clone of ADS: `git describe --tags --abbrev=0`, when the source   #
#         directory is the top of the clone (and not a copy of the sources    #
#         inside the clone of another project, whose tags are not those of    #
#         ADS);                                                               #
#      2. a source tarball (e.g., the one of a GitHub release, which is not   #
#         a clone): the file VERSION.txt, which `git archive` fills in with   #
#         the tag through the entry `VERSION.txt export-subst` of             #
#         .gitattributes (its content is "$Format:%(describe:tags,abbrev=0)$" #
#         and git writes the tag in its place);                               #
#      3. otherwise, e.g., a shallow clone without the tags or a tarball of   #
#         a commit after the last tag, the version of MODULE.bazel, which     #
#         the Bazel Central Registry needs written in that file and which is  #
#         the one of the next release [see packaging/README.md].              #
#                                                                             #
#    The file is VERSION.txt and not VERSION, which would shadow the header   #
#    <version> of C++20 on the file systems that ignore the case (macOS,      #
#    Windows) whenever the root of the sources is on an include path.         #
#                                                                             #
#    The leading x[.y[.z]] is taken from whatever git gives, after an         #
#    optional "v", so that a describe with a distance ("0.1.0-4-gdeadbee")    #
#    gives "0.1.0".                                                           #
#                                                                             #
#    Call it before project():                                                #
#        include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeriveVersion.cmake)       #
#        ads_derive_version(ADS_VERSION)                                      #
#        project(ADS VERSION ${ADS_VERSION} ...)                              #
# --------------------------------------------------------------------------- #

function(ads_derive_version out_var)
    set(_raw "")
    set(_src "${CMAKE_CURRENT_SOURCE_DIR}")

    # 1) a clone of ADS, whose top is the source directory
    find_package(Git QUIET)
    if (GIT_FOUND)
        # the path from the source directory up to the top of the clone,
        # empty at the top
        execute_process(
                COMMAND "${GIT_EXECUTABLE}" rev-parse --show-cdup
                WORKING_DIRECTORY "${_src}"
                OUTPUT_VARIABLE _up
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
                RESULT_VARIABLE _result)
        if (_result EQUAL 0 AND _up STREQUAL "")
            execute_process(
                    COMMAND "${GIT_EXECUTABLE}" describe --tags --abbrev=0
                    WORKING_DIRECTORY "${_src}"
                    OUTPUT_VARIABLE _raw
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
                    RESULT_VARIABLE _result)
            if (NOT _result EQUAL 0)
                set(_raw "")
            endif ()
        endif ()
    endif ()

    # 2) a tarball: VERSION.txt as `git archive` filled it in; in a copy
    #    that is not an archive it still holds the literal "$Format:...$",
    #    which fails the numeric match below
    if (NOT _raw MATCHES "^v?[0-9]" AND EXISTS "${_src}/VERSION.txt")
        file(READ "${_src}/VERSION.txt" _raw)
        string(STRIP "${_raw}" _raw)
    endif ()

    # 3) the version of the next release, in MODULE.bazel
    if (NOT _raw MATCHES "^v?[0-9]" AND EXISTS "${_src}/MODULE.bazel")
        file(STRINGS "${_src}/MODULE.bazel" _raw
             REGEX "^[ \t]*version[ \t]*=[ \t]*\"[0-9]" LIMIT_COUNT 1)
        string(REGEX REPLACE "^[^\"]*\"([^\"]*)\".*" "\\1" _raw "${_raw}")
    endif ()

    if (_raw MATCHES "^v?([0-9]+(\\.[0-9]+)*)")
        set(_version "${CMAKE_MATCH_1}")
        # x and x.y as x.0.0 and x.y.0, so that ADSConfig.h has all three
        string(REGEX MATCHALL "[0-9]+" _parts "${_version}")
        list(LENGTH _parts _n)
        while (_n LESS 3)
            string(APPEND _version ".0")
            math(EXPR _n "${_n} + 1")
        endwhile ()
    else ()
        set(_version "0.0.0")
    endif ()

    set(${out_var} "${_version}" PARENT_SCOPE)
endfunction()

# --------------------------------------------------------------------------- #
