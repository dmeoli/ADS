# --------------------------------------------------------------------------- #
#    ADSGMP.cmake                                                             #
#                                                                             #
#    GMP for the backends of BigInteger and Rational [see BigInteger.h and    #
#    Rational.h]: the target ADS::GMP, with the headers and the library of    #
#    GMP found by find_path() and find_library() (GMP_INCLUDE_DIR and         #
#    GMP_LIBRARY in the cache point to another GMP, ADS_GMP_HINTS are         #
#    directories to look in first), and ADS_GMP_FOUND, true when they are     #
#    found and, where C++ is enabled, a program that calls the functions of   #
#    GMP 6 that the backends use compiles and links with them. CMakeLists.txt #
#    and ADSConfig.cmake include it.                                          #
# --------------------------------------------------------------------------- #

find_path(GMP_INCLUDE_DIR gmp.h HINTS ${ADS_GMP_HINTS})
find_library(GMP_LIBRARY gmp HINTS ${ADS_GMP_HINTS})
set(ADS_GMP_FOUND FALSE)
if (GMP_INCLUDE_DIR AND GMP_LIBRARY)
    set(ADS_GMP_FOUND TRUE)
    if (CMAKE_CXX_COMPILER_LOADED)
        include(CheckCXXSourceCompiles)
        include(CMakePushCheckState)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_INCLUDES "${GMP_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_LIBRARIES "${GMP_LIBRARY}")
        set(CMAKE_REQUIRED_QUIET ON)
        check_cxx_source_compiles("
            #include <gmp.h>
            int main( void ) {
             mp_limb_t a[ 1 ] = { 4 } , s[ 1 ];
             mpz_t x;
             mpz_roinit_n( x , a , 1 );
             mpn_sqrtrem( s , nullptr , a , 1 );
             return( int( s[ 0 ] ) - 2 );
             }" ADS_GMP_LINKS)
        cmake_pop_check_state()
        set(ADS_GMP_FOUND ${ADS_GMP_LINKS})
    endif ()
endif ()
if (ADS_GMP_FOUND AND NOT TARGET ADS::GMP)
    add_library(ADS::GMP UNKNOWN IMPORTED)
    set_target_properties(ADS::GMP PROPERTIES
            IMPORTED_LOCATION "${GMP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}")
endif ()

# --------------------------------------------------------------------------- #
