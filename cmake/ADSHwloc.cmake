# --------------------------------------------------------------------------- #
#    ADSHwloc.cmake                                                           #
#                                                                             #
#    hwloc for the placement of the threads [see Affinity.h]: the target      #
#    ADS::Hwloc, with the headers and the library of hwloc found by           #
#    find_path() and find_library() (HWLOC_INCLUDE_DIR and HWLOC_LIBRARY in   #
#    the cache point to another hwloc, ADS_HWLOC_HINTS are directories to     #
#    look in first), and ADS_HWLOC_FOUND, true when they are found and,       #
#    where C++ is enabled, a program that loads the topology of hwloc 2       #
#    compiles and links with them. CMakeLists.txt and ADSConfig.cmake         #
#    include it.                                                              #
# --------------------------------------------------------------------------- #

find_path(HWLOC_INCLUDE_DIR hwloc.h HINTS ${ADS_HWLOC_HINTS})
find_library(HWLOC_LIBRARY hwloc HINTS ${ADS_HWLOC_HINTS})
set(ADS_HWLOC_FOUND FALSE)
if (HWLOC_INCLUDE_DIR AND HWLOC_LIBRARY)
    set(ADS_HWLOC_FOUND TRUE)
    if (CMAKE_CXX_COMPILER_LOADED)
        include(CheckCXXSourceCompiles)
        include(CMakePushCheckState)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_INCLUDES "${HWLOC_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_LIBRARIES "${HWLOC_LIBRARY}")
        set(CMAKE_REQUIRED_QUIET ON)
        check_cxx_source_compiles("
            #include <hwloc.h>
            static_assert( HWLOC_API_VERSION >= 0x00020000 );
            int main( void ) {
             hwloc_topology_t t;
             if( hwloc_topology_init( &t ) != 0 )
              return( 1 );
             const int r = hwloc_topology_load( t );
             hwloc_topology_destroy( t );
             return( r );
             }" ADS_HWLOC_LINKS)
        cmake_pop_check_state()
        set(ADS_HWLOC_FOUND ${ADS_HWLOC_LINKS})
    endif ()
endif ()
if (ADS_HWLOC_FOUND AND NOT TARGET ADS::Hwloc)
    add_library(ADS::Hwloc UNKNOWN IMPORTED)
    set_target_properties(ADS::Hwloc PROPERTIES
            IMPORTED_LOCATION "${HWLOC_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${HWLOC_INCLUDE_DIR}")
endif ()

# --------------------------------------------------------------------------- #
