# --------------------------------------------------------------------------- #
#    ADSBoost.cmake                                                           #
#                                                                             #
#    The headers of Boost for the structures that delegate to it [see         #
#    PointerTable.h, FlatHashTable.h, SmallVector.h and RandomGenerators.h]:  #
#    the target ADS::Boost, with the directory of the headers of Boost found  #
#    by find_path() (Boost_INCLUDE_DIR in the cache points to another Boost,  #
#    ADS_BOOST_HINTS are directories to look in first), and ADS_BOOST_FOUND,  #
#    true when they are found and, where C++ is enabled, a program that uses  #
#    boost::unordered_flat_set, boost::unordered_flat_map and                 #
#    boost::container::small_vector of Boost 1.83 or later compiles with      #
#    them; ADS_BOOST_PC_CFLAGS, the flags of ads.pc. CMakeLists.txt and       #
#    ADSConfig.cmake include it.                                              #
# --------------------------------------------------------------------------- #

find_path(Boost_INCLUDE_DIR boost/unordered/unordered_flat_set.hpp
          HINTS ${ADS_BOOST_HINTS})
set(ADS_BOOST_FOUND FALSE)
if (Boost_INCLUDE_DIR)
    set(ADS_BOOST_FOUND TRUE)
    if (CMAKE_CXX_COMPILER_LOADED)
        include(CheckCXXSourceCompiles)
        include(CMakePushCheckState)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_INCLUDES "${Boost_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_QUIET ON)
        check_cxx_source_compiles("
            #include <boost/version.hpp>
            #include <boost/container/small_vector.hpp>
            #include <boost/unordered/unordered_flat_map.hpp>
            #include <boost/unordered/unordered_flat_set.hpp>
            #if BOOST_VERSION < 108300
             #error Boost 1.83 or later
            #endif
            int main( void ) {
             boost::unordered_flat_set< int > s;
             boost::unordered_flat_map< int , int > m;
             boost::container::small_vector< int , 2 > v;
             s.insert( 1 );
             m[ 2 ] = 3;
             v.push_back( 4 );
             return( int( s.size() + m.size() + v.size() ) - 3 );
             }" ADS_BOOST_COMPILES)
        cmake_pop_check_state()
        set(ADS_BOOST_FOUND ${ADS_BOOST_COMPILES})
    endif ()
endif ()
if (ADS_BOOST_FOUND)
    if (NOT TARGET ADS::Boost)
        add_library(ADS::Boost INTERFACE IMPORTED)
        set_target_properties(ADS::Boost PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIR}")
    endif ()
    # the flags of ads.pc: the directory of the headers, unless it is one
    # where the compiler looks by itself
    set(ADS_BOOST_PC_CFLAGS "")
    if (NOT Boost_INCLUDE_DIR IN_LIST
            CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
        set(ADS_BOOST_PC_CFLAGS " -I${Boost_INCLUDE_DIR}")
    endif ()
endif ()

# --------------------------------------------------------------------------- #
