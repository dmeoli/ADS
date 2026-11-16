# --------------------------------------------------------------------------- #
#    ADSHighway.cmake                                                         #
#                                                                             #
#    Google Highway for the vector kernels [see Simd.h] and the prefetch      #
#    [see Prefetch.h]: the target ADS::Highway, with the headers and the      #
#    libraries hwy and hwy_contrib found by find_path() and find_library()    #
#    (HWY_INCLUDE_DIR, HWY_LIBRARY and HWY_CONTRIB_LIBRARY in the cache       #
#    point to another Highway, ADS_HWY_HINTS are directories to look in       #
#    first), and ADS_HWY_FOUND, true when they are found and, where C++ is    #
#    enabled, a program that dispatches a kernel at run time, compares and    #
#    counts lanes, and calls the sort of Highway compiles and links with      #
#    them. CMakeLists.txt and ADSConfig.cmake include it.                     #
# --------------------------------------------------------------------------- #

find_path(HWY_INCLUDE_DIR hwy/highway.h HINTS ${ADS_HWY_HINTS})
find_library(HWY_LIBRARY hwy HINTS ${ADS_HWY_HINTS})
find_library(HWY_CONTRIB_LIBRARY hwy_contrib HINTS ${ADS_HWY_HINTS})
set(ADS_HWY_FOUND FALSE)
if (HWY_INCLUDE_DIR AND HWY_LIBRARY AND HWY_CONTRIB_LIBRARY)
    set(ADS_HWY_FOUND TRUE)
    if (CMAKE_CXX_COMPILER_LOADED)
        include(CheckCXXSourceCompiles)
        include(CMakePushCheckState)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_INCLUDES "${HWY_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_LIBRARIES
            "${HWY_CONTRIB_LIBRARY};${HWY_LIBRARY}")
        set(CMAKE_REQUIRED_QUIET ON)
        check_cxx_source_compiles("
            #include <hwy/highway.h>
            #include <hwy/cache_control.h>
            #include <hwy/contrib/sort/vqsort.h>
            #include <cstdint>
            namespace hn = hwy::HWY_NAMESPACE;
            int main( void ) {
             const hn::CappedTag< std::uint8_t , 16 > d;
             std::uint8_t c[ 16 ] = { 3 };
             std::uint8_t b[ 8 ] = {};
             hwy::Prefetch( c );
             hn::StoreMaskBits( d , hn::Eq( hn::LoadU( d , c ) ,
                                            hn::Set( d , 3 ) ) , b );
             std::int64_t k[ 2 ] = { 2 , 1 };
             hwy::VQSort( k , 2 , hwy::SortAscending() );
             return( int( k[ 0 ] ) - 1 + ( b[ 0 ] & 0 ) +
                     int( hwy::SupportedTargets() == 0 ) );
             }" ADS_HWY_LINKS)
        cmake_pop_check_state()
        set(ADS_HWY_FOUND ${ADS_HWY_LINKS})
    endif ()
endif ()
if (ADS_HWY_FOUND AND NOT TARGET ADS::Highway)
    add_library(ADS::Highway INTERFACE IMPORTED)
    set_target_properties(ADS::Highway PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${HWY_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES
            "${HWY_CONTRIB_LIBRARY};${HWY_LIBRARY}")
endif ()

# --------------------------------------------------------------------------- #
