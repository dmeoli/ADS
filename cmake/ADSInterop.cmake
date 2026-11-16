# --------------------------------------------------------------------------- #
#    ADSInterop.cmake                                                         #
#                                                                             #
#    The runtimes of a program for the executors on them [see                 #
#    include/ADS/interop/parallel and include/ADS/distributed/BoostMPI.h]:    #
#    for each name in ADS_INTEROP_WANT, among StdPar (the parallel            #
#    algorithms of the standard library, on oneTBB with libstdc++), stdexec,  #
#    Taskflow, Kokkos, BoostMPI, cereal, and the libraries of graphs and of   #
#    matrices Eigen, METIS, KaHIP, GraphBLAS and NetworKit, the target        #
#    ADS::<name>, with the macro                                              #
#    ADS_WITH_<NAME> of its header, the headers and the libraries of the      #
#    runtime and ADS::ADS, and ADS_<name>_FOUND, true when the runtime is     #
#    found and, for the headers alone, a program that uses it compiles and    #
#    links. ADS::ADS itself never carries them, so that a program links a     #
#    runtime only by asking for its target. The hints are                     #
#    STDEXEC_INCLUDE_DIR, Taskflow_INCLUDE_DIR, Kokkos_DIR, Boost_DIR,        #
#    cereal_INCLUDE_DIR, EIGEN3_INCLUDE_DIR, METIS_INCLUDE_DIR and            #
#    METIS_LIBRARY, KaHIP_INCLUDE_DIR and KaHIP_LIBRARY,                      #
#    GraphBLAS_INCLUDE_DIR and GraphBLAS_LIBRARY, NetworKit_INCLUDE_DIR,      #
#    NetworKit_LIBRARY and tlx_INCLUDE_DIR (or CMAKE_PREFIX_PATH).            #
#    CMakeLists.txt and ADSConfig.cmake include it.                           #
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

# the archives of cereal, by its headers [see include/ADS/interop/cereal]
if ("cereal" IN_LIST ADS_INTEROP_WANT)
    set(ADS_cereal_FOUND FALSE)
    find_path(cereal_INCLUDE_DIR cereal/cereal.hpp PATH_SUFFIXES include)
    if (cereal_INCLUDE_DIR)
        ads_interop_try(cereal ADS_WITH_CEREAL "
            #include <cereal/archives/binary.hpp>
            #include <cereal/types/vector.hpp>
            #include <sstream>
            #include <vector>
            int main( void ) {
             std::stringstream s;
             std::vector< int > v{ 1 , 2 };
             { cereal::BinaryOutputArchive a( s ); a( v ); }
             cereal::BinaryInputArchive a( s );
             a( v );
             return( v[ 1 ] - 2 );
             }" "${cereal_INCLUDE_DIR}" "")
    endif ()
endif ()

# --------------------------------------------------------------------------- #

# Eigen, by its headers [see include/ADS/interop/eigen]
if ("Eigen" IN_LIST ADS_INTEROP_WANT)
    set(ADS_Eigen_FOUND FALSE)
    find_path(EIGEN3_INCLUDE_DIR Eigen/SparseCore PATH_SUFFIXES eigen3)
    if (EIGEN3_INCLUDE_DIR)
        ads_interop_try(Eigen ADS_WITH_EIGEN "
            #include <Eigen/SparseCore>
            static_assert( EIGEN_VERSION_AT_LEAST( 3 , 4 , 0 ) );
            int main( void ) {
             Eigen::SparseMatrix< double , Eigen::RowMajor , int > A( 2 , 2 );
             A.insert( 1 , 0 ) = 2.;
             return( int( A.sum() ) - 2 );
             }" "${EIGEN3_INCLUDE_DIR}" "")
    endif ()
endif ()

# METIS 5, by its header and its library [see include/ADS/interop/metis]
if ("METIS" IN_LIST ADS_INTEROP_WANT)
    set(ADS_METIS_FOUND FALSE)
    find_path(METIS_INCLUDE_DIR metis.h PATH_SUFFIXES include)
    find_library(METIS_LIBRARY metis)
    if (METIS_INCLUDE_DIR AND METIS_LIBRARY)
        ads_interop_try(METIS ADS_WITH_METIS "
            #include <metis.h>
            int main( void ) {
             idx_t o[ METIS_NOPTIONS ];
             return( METIS_SetDefaultOptions( o ) == METIS_OK ? 0 : 1 );
             }" "${METIS_INCLUDE_DIR}" "${METIS_LIBRARY}")
    endif ()
endif ()

# KaHIP, by its header and its library, whose indices must be those of its
# header [see include/ADS/interop/kahip]
if ("KaHIP" IN_LIST ADS_INTEROP_WANT)
    set(ADS_KaHIP_FOUND FALSE)
    find_path(KaHIP_INCLUDE_DIR kaHIP_interface.h PATH_SUFFIXES include)
    find_library(KaHIP_LIBRARY kahip)
    if (KaHIP_INCLUDE_DIR AND KaHIP_LIBRARY)
        ads_interop_try(KaHIP ADS_WITH_KAHIP "
            #include <kaHIP_interface.h>
            int main( void ) {
             return( kahip_sizeof_idx() == int( sizeof( kahip_idx ) )
                     ? 0 : 1 );
             }" "${KaHIP_INCLUDE_DIR}" "${KaHIP_LIBRARY}")
    endif ()
endif ()

# SuiteSparse:GraphBLAS 10 or later, by its header and its library [see
# include/ADS/interop/graphblas]
if ("GraphBLAS" IN_LIST ADS_INTEROP_WANT)
    set(ADS_GraphBLAS_FOUND FALSE)
    find_path(GraphBLAS_INCLUDE_DIR GraphBLAS.h
              PATH_SUFFIXES include include/suitesparse suitesparse)
    find_library(GraphBLAS_LIBRARY graphblas)
    if (GraphBLAS_INCLUDE_DIR AND GraphBLAS_LIBRARY)
        ads_interop_try(GraphBLAS ADS_WITH_GRAPHBLAS "
            #include <GraphBLAS.h>
            static_assert( GxB_IMPLEMENTATION_MAJOR >= 10 );
            int main( void ) {
             GrB_Info ( *f )( GxB_Container * ) = &GxB_Container_new;
             return( f == nullptr ? 1 : 0 );
             }" "${GraphBLAS_INCLUDE_DIR}" "${GraphBLAS_LIBRARY}")
    endif ()
endif ()

# NetworKit 11.2 or later, with fromCSR(), by its headers, those of tlx and
# its library, with OpenMP, which its headers use [see
# include/ADS/interop/networkit]
if ("NetworKit" IN_LIST ADS_INTEROP_WANT)
    set(ADS_NetworKit_FOUND FALSE)
    find_path(NetworKit_INCLUDE_DIR networkit/graph/Graph.hpp
              PATH_SUFFIXES include)
    find_path(tlx_INCLUDE_DIR tlx/version.hpp PATH_SUFFIXES include)
    find_library(NetworKit_LIBRARY networkit)
    find_package(OpenMP QUIET COMPONENTS CXX)
    if (NetworKit_INCLUDE_DIR AND tlx_INCLUDE_DIR AND NetworKit_LIBRARY AND
        TARGET OpenMP::OpenMP_CXX)
        ads_interop_try(NetworKit ADS_WITH_NETWORKIT "
            #include <networkit/graph/Graph.hpp>
            #include <vector>
            int main( void ) {
             std::vector< NetworKit::index > p{ 0 , 1 , 1 } , i{ 1 };
             std::vector< double > x;
             const NetworKit::Graph G =
               NetworKit::Graph::fromCSR( p , i , x , false , false );
             return( int( G.numberOfEdges() ) - 1 );
             }" "${NetworKit_INCLUDE_DIR};${tlx_INCLUDE_DIR}"
             "${NetworKit_LIBRARY};OpenMP::OpenMP_CXX")
    endif ()
endif ()
