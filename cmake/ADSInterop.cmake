# --------------------------------------------------------------------------- #
#    ADSInterop.cmake                                                         #
#                                                                             #
#    The runtimes of a program for the executors on them [see                 #
#    include/ADS/interop/parallel and include/ADS/distributed/BoostMPI.h]:    #
#    for each name in ADS_INTEROP_WANT, among StdPar (the parallel            #
#    algorithms of the standard library, on oneTBB with libstdc++), stdexec,  #
#    Taskflow, Kokkos and BoostMPI, the target ADS::<name>, with the macro    #
#    ADS_WITH_<NAME> of its header, the headers and the libraries of the      #
#    runtime and ADS::ADS, and ADS_<name>_FOUND, true when the runtime is     #
#    found and, for the headers alone, a program that uses it compiles and    #
#    links. ADS::ADS itself never carries them, so that a program links a     #
#    runtime only by asking for its target. The hints are                     #
#    STDEXEC_INCLUDE_DIR, Taskflow_INCLUDE_DIR, Kokkos_DIR and Boost_DIR (or  #
#    CMAKE_PREFIX_PATH). CMakeLists.txt and ADSConfig.cmake include it.       #
# --------------------------------------------------------------------------- #

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# whether the program src compiles and links in C++20 with the include
# directories incs and the libraries libs, in var
function(ads_interop_compiles var src incs libs)
    if (NOT CMAKE_CXX_COMPILER_LOADED)
        set(${var} TRUE PARENT_SCOPE)
        return()
    endif ()
    cmake_push_check_state(RESET)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_REQUIRED_INCLUDES "${incs}")
    set(CMAKE_REQUIRED_LIBRARIES "${libs}")
    set(CMAKE_REQUIRED_QUIET ON)
    check_cxx_source_compiles("${src}" ${var})
    cmake_pop_check_state()
    set(${var} ${${var}} PARENT_SCOPE)
endfunction()

# the target ADS::<name> with the macro def, the include directories incs
# and the libraries libs
function(ads_interop_target name def incs libs)
    if (NOT TARGET ADS::${name})
        add_library(ADS::${name} INTERFACE IMPORTED)
        set_target_properties(ADS::${name} PROPERTIES
                INTERFACE_COMPILE_DEFINITIONS "${def}"
                INTERFACE_INCLUDE_DIRECTORIES "${incs}"
                INTERFACE_LINK_LIBRARIES "ADS::ADS;${libs}")
    endif ()
endfunction()

# the target ADS::<name> with the macro def, the include directories incs
# and the libraries libs, and ADS_<name>_FOUND, if the program src compiles
# and links with them, or else with oneTBB too (the backend of the parallel
# algorithms of libstdc++, which the headers of the runtime may include)
function(ads_interop_try name def src incs libs)
    ads_interop_compiles(ADS_${name}_ALONE "${src}" "${incs}" "${libs}")
    if (ADS_${name}_ALONE)
        ads_interop_target(${name} ${def} "${incs}" "${libs}")
        set(ADS_${name}_FOUND TRUE PARENT_SCOPE)
        return()
    endif ()
    find_package(TBB QUIET)
    if (TARGET TBB::tbb)
        ads_interop_compiles(ADS_${name}_TBB "${src}" "${incs}"
                             "${libs};TBB::tbb")
        if (ADS_${name}_TBB)
            ads_interop_target(${name} ${def} "${incs}" "${libs};TBB::tbb")
            set(ADS_${name}_FOUND TRUE PARENT_SCOPE)
            return()
        endif ()
    endif ()
    set(ADS_${name}_FOUND FALSE PARENT_SCOPE)
endfunction()

find_package(Threads QUIET)

# the parallel algorithms of the standard library, by themselves or on
# oneTBB (the backend of libstdc++, linked when the headers of oneTBB are
# there)
if ("StdPar" IN_LIST ADS_INTEROP_WANT)
    set(ADS_StdPar_FOUND FALSE)
    set(ADS_STDPAR_SRC "
        #include <algorithm>
        #include <atomic>
        #include <execution>
        #include <vector>
        int main( void ) {
         std::vector< int > v( 1000 , 1 );
         std::atomic< int > s{ 0 };
         std::for_each( std::execution::par , v.begin() , v.end() ,
                        [ & ]( int x ) { s += x; } );
         return( s.load() - 1000 );
         }")
    ads_interop_try(StdPar ADS_WITH_STDPAR "${ADS_STDPAR_SRC}" "" "")
endif ()

# the senders of stdexec, by their headers
if ("stdexec" IN_LIST ADS_INTEROP_WANT)
    set(ADS_stdexec_FOUND FALSE)
    find_path(STDEXEC_INCLUDE_DIR stdexec/execution.hpp
              PATH_SUFFIXES include)
    if (STDEXEC_INCLUDE_DIR AND TARGET Threads::Threads)
        ads_interop_try(stdexec ADS_WITH_STDEXEC "
            #include <stdexec/execution.hpp>
            #include <exec/static_thread_pool.hpp>
            #include <atomic>
            int main( void ) {
             exec::static_thread_pool pool( 2 );
             std::atomic< int > s{ 0 };
             stdexec::sync_wait( stdexec::schedule( pool.get_scheduler() ) |
                                 stdexec::bulk( stdexec::par , 10 ,
                                   [ & ]( int i ) noexcept { s += i; } ) );
             return( s.load() - 45 );
             }" "${STDEXEC_INCLUDE_DIR}" Threads::Threads)
    endif ()
endif ()

# Taskflow, by its headers
if ("Taskflow" IN_LIST ADS_INTEROP_WANT)
    set(ADS_Taskflow_FOUND FALSE)
    find_path(Taskflow_INCLUDE_DIR taskflow/taskflow.hpp
              PATH_SUFFIXES include)
    if (Taskflow_INCLUDE_DIR AND TARGET Threads::Threads)
        ads_interop_try(Taskflow ADS_WITH_TASKFLOW "
            #include <taskflow/taskflow.hpp>
            #include <taskflow/algorithm/for_each.hpp>
            #include <atomic>
            int main( void ) {
             tf::Executor e( 2 );
             tf::Taskflow f;
             std::atomic< int > s{ 0 };
             f.for_each_index( 0 , 10 , 1 , [ & ]( int i ) { s += i; } );
             e.run( f ).wait();
             return( s.load() - 45 );
             }" "${Taskflow_INCLUDE_DIR}" Threads::Threads)
    endif ()
endif ()

# Kokkos, by its package, which carries its flags
if ("Kokkos" IN_LIST ADS_INTEROP_WANT)
    set(ADS_Kokkos_FOUND FALSE)
    find_package(Kokkos CONFIG QUIET)
    if (TARGET Kokkos::kokkos)
        set(ADS_Kokkos_FOUND TRUE)
        ads_interop_target(Kokkos ADS_WITH_KOKKOS "" Kokkos::kokkos)
    endif ()
endif ()

# Boost.MPI and Boost.Serialization, beside MPI
if ("BoostMPI" IN_LIST ADS_INTEROP_WANT)
    set(ADS_BoostMPI_FOUND FALSE)
    if (TARGET MPI::MPI_CXX)
        find_package(Boost CONFIG QUIET COMPONENTS mpi serialization)
        if (TARGET Boost::mpi AND TARGET Boost::serialization)
            set(ADS_BoostMPI_FOUND TRUE)
            ads_interop_target(BoostMPI ADS_WITH_BOOST_MPI ""
                    "Boost::mpi;Boost::serialization;MPI::MPI_CXX")
        endif ()
    endif ()
endif ()

# --------------------------------------------------------------------------- #
