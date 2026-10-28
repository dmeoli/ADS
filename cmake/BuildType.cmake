# --------------------------------------------------------------------------- #
#    BuildType.cmake                                                          #
#                                                                             #
#    Sets the build type to Release when none was given, and only when ADS    #
#    is the main project: as a subproject it leaves the build type to the     #
#    project that takes it. Release also in a clone, since the tests of the   #
#    exercises take many times longer without the optimizations.              #
# --------------------------------------------------------------------------- #
set(default_build_type "Release")

if (PROJECT_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE
        AND NOT CMAKE_CONFIGURATION_TYPES)

    message(STATUS "Setting build type to '${default_build_type}'")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
        STRING "Choose the type of build." FORCE)

    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

# --------------------------------------------------------------------------- #
