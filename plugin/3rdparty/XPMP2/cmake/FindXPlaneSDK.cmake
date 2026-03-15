# FindXPlaneSDK.cmake
#
# Searches fo the X-Plane SDK
# Define XPlaneSDK_DIR upfront if you know where it is
# By default, only looks for header directories and returns them in XPlaneSDK_INCLUDE_DIRS.
# Supports COMPONENTS "XPLM" and "XPWidgets" if interested in library targets to link to
# Keep in mind that no libraries are provided nor needed for Linux builds.

include(FindPackageHandleStandardArgs)

# If a path is given, use it
if(XPlaneSDK_DIR)
    set(_xplane_sdk_possible_paths "${XPlaneSDK_DIR}")
else()
    # Otherwise define common search paths for SDK
    if(WIN32)
        set(_xplane_sdk_possible_paths
            "C:/XPlaneSDK"
            "C:/Program Files/XPlaneSDK"
        )
    elseif(APPLE)
        set(_xplane_sdk_possible_paths
            "/Library/Developer/XPlaneSDK"
            "$ENV{HOME}/XPlaneSDK"
        )
    elseif(UNIX)
        set(_xplane_sdk_possible_paths
            "/usr/local/XPlaneSDK"
            "/opt/XPlaneSDK"
        )
    endif()
endif()

# Search for the SDK root directory
find_path(XPlaneSDK_ROOT_DIR
    NAMES "CHeaders/XPLM/XPLMDefs.h"
    PATHS ${_xplane_sdk_possible_paths}
    DOC "Directory containing the X-Plane SDK"
)

if (XPlaneSDK_ROOT_DIR)
    # --- Header Directories ---
    if(IS_DIRECTORY "${XPlaneSDK_ROOT_DIR}/CHeaders/XPLM")
        SET(XPlaneSDK_XPLM_INCLUDE_DIR "${XPlaneSDK_ROOT_DIR}/CHeaders/XPLM")
        LIST(APPEND XPlaneSDK_INCLUDE_DIRS ${XPlaneSDK_XPLM_INCLUDE_DIR})
    endif()
    if(IS_DIRECTORY "${XPlaneSDK_ROOT_DIR}/CHeaders/Widgets")
        SET(XPlaneSDK_XPWidgets_INCLUDE_DIR "${XPlaneSDK_ROOT_DIR}/CHeaders/Widgets")
        LIST(APPEND XPlaneSDK_INCLUDE_DIRS ${XPlaneSDK_XPWidgets_INCLUDE_DIR})
    endif()
    if(IS_DIRECTORY "${XPlaneSDK_ROOT_DIR}/CHeaders/Wrappers")
        SET(XPlaneSDK_Wrappers_INCLUDE_DIR "${XPlaneSDK_ROOT_DIR}/CHeaders/Wrappers")
        LIST(APPEND XPlaneSDK_INCLUDE_DIRS ${XPlaneSDK_Wrappers_INCLUDE_DIR})
    endif()

    # --- Library ---
    # Define common subdirectories for libraries
    if(WIN32)
        set(_xplane_lib_path "${XPlaneSDK_ROOT_DIR}/Libraries/Win")
    elseif(APPLE)
        set(_xplane_lib_path "${XPlaneSDK_ROOT_DIR}/Libraries/Mac")
    elseif(UNIX)
        set(_xplane_lib_path "${XPlaneSDK_ROOT_DIR}/Libraries/Lin")
    endif()

    # Try finding the components, expected are XPLM and/or XPWidgets
    foreach (comp IN LISTS XPlaneSDK_FIND_COMPONENTS)
        # Find the actual library
        find_library(XPlaneSDK_${comp}_LIBRARY NAMES "${comp}_64" "${comp}" PATHS "${_xplane_lib_path}")
        # Create an import target for the XPLM component
        if(XPlaneSDK_${comp}_LIBRARY AND NOT TARGET XPlaneSDK::${comp})
            add_library(XPlaneSDK::${comp} UNKNOWN IMPORTED)
            set_target_properties(XPlaneSDK::${comp} PROPERTIES
                IMPORTED_LOCATION ${XPlaneSDK_${comp}_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES "${XPlaneSDK_${comp}_INCLUDE_DIR}"
            )
            set(XPlaneSDK_${comp}_FOUND TRUE)
            message ("XPlaneSDK_${comp}_LIBRARY = ${XPlaneSDK_${comp}_LIBRARY}")
        endif()
    endforeach()
endif()

# Provide results back to caller
find_package_handle_standard_args(XPlaneSDK
    REQUIRED_VARS XPlaneSDK_ROOT_DIR XPlaneSDK_XPLM_INCLUDE_DIR
    REASON_FAILURE_MESSAGE "X-Plane SDK not found in ${_xplane_sdk_possible_paths}. Specify location by defining XPlaneSDK_ROOT_DIR."
    HANDLE_COMPONENTS
)