# --------------------------------------------------------------------------- #
#    ADSLemon.cmake                                                           #
#                                                                             #
#    LEMON for the interoperation of include/ADS/interop/lemon and for the    #
#    solves that ADS delegates to it [see Matching.h, Assignment.h]: the      #
#    target ADS::LEMON_library, with the headers of LEMON found by            #
#    find_path() (LEMON_INCLUDE_DIR in the cache points to another LEMON,     #
#    ADS_LEMON_HINTS and LEMON_ROOT are directories to look in first) and its #
#    library, or LEMON_ONLY_TEMPLATES where there is none, and                #
#    ADS_LEMON_FOUND, true when they are found and, where C++ is enabled, a   #
#    program with a map of arcs of a ListDigraph and Dijkstra's algorithm     #
#    compiles in C++20 with them (abLEMON does, LEMON 1.3 does not, since its #
#    maps call members of std::allocator that C++20 removed);                 #
#    ADS_LEMON_PC_CFLAGS and ADS_LEMON_PC_LIBS, the flags of ads-lemon.pc.    #
#    CMakeLists.txt and ADSConfig.cmake include it.                           #
# --------------------------------------------------------------------------- #

find_path(LEMON_INCLUDE_DIR lemon/config.h
          HINTS ${ADS_LEMON_HINTS} ${LEMON_ROOT} ENV LEMON_ROOT
          PATH_SUFFIXES include)
find_library(LEMON_LIBRARY NAMES lemon emon
             HINTS ${ADS_LEMON_HINTS} ${LEMON_ROOT} ENV LEMON_ROOT
             PATH_SUFFIXES lib lib64)
set(ADS_LEMON_FOUND FALSE)
if (LEMON_INCLUDE_DIR)
    set(ADS_LEMON_FOUND TRUE)
    if (CMAKE_CXX_COMPILER_LOADED)
        include(CheckCXXSourceCompiles)
        include(CMakePushCheckState)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_INCLUDES "${LEMON_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_DEFINITIONS -DLEMON_ONLY_TEMPLATES)
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX20_STANDARD_COMPILE_OPTION}")
        set(CMAKE_REQUIRED_QUIET ON)
        check_cxx_source_compiles("
            #include <lemon/dijkstra.h>
            #include <lemon/list_graph.h>
            int main( void ) {
             lemon::ListDigraph g;
             lemon::ListDigraph::ArcMap< int > l( g );
             lemon::ListDigraph::NodeMap< lemon::ListDigraph::Arc > p( g );
             const lemon::ListDigraph::Node s = g.addNode();
             l[ g.addArc( s , g.addNode() ) ] = 1;
             lemon::Dijkstra< lemon::ListDigraph > d( g , l );
             d.run( s );
             return( d.reached( s ) ? 0 : 1 );
             }" ADS_LEMON_COMPILES)
        cmake_pop_check_state()
        set(ADS_LEMON_FOUND ${ADS_LEMON_COMPILES})
    endif ()
endif ()
if (ADS_LEMON_FOUND)
    if (NOT TARGET ADS::LEMON_library)
        add_library(ADS::LEMON_library INTERFACE IMPORTED)
        set_target_properties(ADS::LEMON_library PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LEMON_INCLUDE_DIR}")
        # the library defines lemon::INVALID, and the epsilons of the
        # tolerances of the floating types that some algorithms read (e.g.,
        # CycleCanceling); without it, the headers define the first, and a
        # program that needs the others links the library itself
        if (LEMON_LIBRARY)
            set_target_properties(ADS::LEMON_library PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${LEMON_LIBRARY}")
        else ()
            set_target_properties(ADS::LEMON_library PROPERTIES
                    INTERFACE_COMPILE_DEFINITIONS LEMON_ONLY_TEMPLATES)
        endif ()
    endif ()
    # the flags of ads-lemon.pc: the directory of the headers, unless it is
    # one where the compiler looks by itself, and the library
    set(ADS_LEMON_PC_CFLAGS "")
    if (NOT LEMON_INCLUDE_DIR IN_LIST CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
        set(ADS_LEMON_PC_CFLAGS " -I${LEMON_INCLUDE_DIR}")
    endif ()
    set(ADS_LEMON_PC_LIBS "")
    if (LEMON_LIBRARY)
        set(ADS_LEMON_PC_LIBS " ${LEMON_LIBRARY}")
    else ()
        string(APPEND ADS_LEMON_PC_CFLAGS " -DLEMON_ONLY_TEMPLATES")
    endif ()
endif ()

# --------------------------------------------------------------------------- #
