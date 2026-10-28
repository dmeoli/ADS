# --------------------------------------------------------------------------- #
#    ADSBLAS.cmake                                                            #
#                                                                             #
#    A CBLAS for the backend of gemm() [see DenseMatrix.h]: the target        #
#    ADS::BLAS with the header cblas.h and the library found by find_path()   #
#    and find_library() (CBLAS_INCLUDE_DIR and CBLAS_LIBRARY in the cache     #
#    point to another one, ADS_BLAS_HINTS are directories to look in first;   #
#    OpenBLAS first, then a library cblas, then one blas), or Accelerate on   #
#    macOS, with ADS_BLAS_ACCELERATE; ADS_BLAS_FOUND, true when they are      #
#    found and, where C++ is enabled, a program that calls cblas_dgemm() and  #
#    cblas_sgemm() compiles and links with them; and the flags of ads.pc,     #
#    ADS_BLAS_PC_CFLAGS and ADS_BLAS_PC_LIBS. CMakeLists.txt and              #
#    ADSConfig.cmake include it.                                              #
# --------------------------------------------------------------------------- #

set(ADS_BLAS_FOUND FALSE)
set(_ads_blas_defs "")
if (APPLE AND NOT CBLAS_LIBRARY)
    find_library(ADS_ACCELERATE Accelerate)
    if (ADS_ACCELERATE)
        set(CBLAS_LIBRARY "${ADS_ACCELERATE}")
        set(CBLAS_INCLUDE_DIR "")
        set(_ads_blas_defs ADS_BLAS_ACCELERATE)
        set(_ads_blas_header "vecLib/cblas.h")
        set(ADS_BLAS_FOUND TRUE)
    endif ()
endif ()
if (NOT ADS_BLAS_FOUND)
    find_path(CBLAS_INCLUDE_DIR cblas.h HINTS ${ADS_BLAS_HINTS}
              PATH_SUFFIXES openblas)
    find_library(CBLAS_LIBRARY NAMES openblas cblas blas
                 HINTS ${ADS_BLAS_HINTS})
    set(_ads_blas_header "cblas.h")
    if (CBLAS_INCLUDE_DIR AND CBLAS_LIBRARY)
        set(ADS_BLAS_FOUND TRUE)
    endif ()
endif ()
if (ADS_BLAS_FOUND AND CMAKE_CXX_COMPILER_LOADED)
    include(CheckCXXSourceCompiles)
    include(CMakePushCheckState)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_INCLUDES "${CBLAS_INCLUDE_DIR}")
    set(CMAKE_REQUIRED_LIBRARIES "${CBLAS_LIBRARY}")
    set(CMAKE_REQUIRED_QUIET ON)
    check_cxx_source_compiles("
        #include <${_ads_blas_header}>
        int main( void ) {
         double a = 2 , c = 0;
         float f = 2 , g = 0;
         cblas_dgemm( CblasRowMajor , CblasNoTrans , CblasNoTrans , 1 , 1 ,
                      1 , 1.0 , &a , 1 , &a , 1 , 0.0 , &c , 1 );
         cblas_sgemm( CblasRowMajor , CblasNoTrans , CblasNoTrans , 1 , 1 ,
                      1 , 1.0f , &f , 1 , &f , 1 , 0.0f , &g , 1 );
         return( int( c + g ) - 8 );
         }" ADS_BLAS_LINKS)
    cmake_pop_check_state()
    set(ADS_BLAS_FOUND ${ADS_BLAS_LINKS})
endif ()
if (ADS_BLAS_FOUND)
    if (NOT TARGET ADS::BLAS)
        add_library(ADS::BLAS INTERFACE IMPORTED)
        set_target_properties(ADS::BLAS PROPERTIES
                INTERFACE_LINK_LIBRARIES "${CBLAS_LIBRARY}"
                INTERFACE_COMPILE_DEFINITIONS "${_ads_blas_defs}")
        if (CBLAS_INCLUDE_DIR)
            set_target_properties(ADS::BLAS PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${CBLAS_INCLUDE_DIR}")
        endif ()
    endif ()
    # the flags of ads.pc: the library by its directory and name, or the
    # framework
    if (_ads_blas_defs)
        set(ADS_BLAS_PC_CFLAGS "-DADS_BLAS_ACCELERATE")
        set(ADS_BLAS_PC_LIBS " -framework Accelerate")
    else ()
        set(ADS_BLAS_PC_CFLAGS "-I${CBLAS_INCLUDE_DIR}")
        get_filename_component(_ads_blas_dir "${CBLAS_LIBRARY}" DIRECTORY)
        get_filename_component(_ads_blas_name "${CBLAS_LIBRARY}" NAME_WE)
        string(REGEX REPLACE "^lib" "" _ads_blas_name "${_ads_blas_name}")
        set(ADS_BLAS_PC_LIBS " -L${_ads_blas_dir} -l${_ads_blas_name}")
    endif ()
endif ()

# --------------------------------------------------------------------------- #
