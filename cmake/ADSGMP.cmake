# --------------------------------------------------------------------------- #
#    ADSGMP.cmake                                                             #
#                                                                             #
#    GMP for the backend of BigInteger [see BigInteger.h]: the target         #
#    ADS::GMP, with the headers and the library of GMP found by find_path()   #
#    and find_library() (GMP_INCLUDE_DIR and GMP_LIBRARY in the cache point   #
#    to another GMP), and ADS_GMP_FOUND. CMakeLists.txt and ADSConfig.cmake   #
#    include it.                                                              #
# --------------------------------------------------------------------------- #

find_path(GMP_INCLUDE_DIR gmp.h)
find_library(GMP_LIBRARY gmp)
if (GMP_INCLUDE_DIR AND GMP_LIBRARY)
    set(ADS_GMP_FOUND TRUE)
    if (NOT TARGET ADS::GMP)
        add_library(ADS::GMP UNKNOWN IMPORTED)
        set_target_properties(ADS::GMP PROPERTIES
                IMPORTED_LOCATION "${GMP_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}")
    endif ()
else ()
    set(ADS_GMP_FOUND FALSE)
endif ()

# --------------------------------------------------------------------------- #
