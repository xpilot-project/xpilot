# XPMP2 - Set up to be used in the provided docker environment to build lin and mac
#         Set up to be used in a Visual Studio environment to build win (File > Open > Folder, then VS recognized the CMAKE configuration)
#
# Targets XPMP2-Remote and XPMP2-Sample are EXCLUDE_FROM_ALL.
# If you want to build them you have to explicitely state them on the command line, like
#   ninja XPMP2-Sample XPMP2-Remote
#
# If you want to build with FMOD sound support,
# 1. Make sure to understand the FMOD license and attribution requirements:
#    https://www.fmod.com/licensing
#    https://www.fmod.com/attribution
# 2. Define INCLUDE_FMOD_SOUND cache entry, e.g. using `cmake -G Ninja -D INCLUDE_FMOD_SOUND=1 ..`

cmake_minimum_required(VERSION 3.16)

# Mac: Need to tell early on that we want a cross platform build
if(DEFINED ENV{platform})
    message ("-- Platform is $ENV{platform}")
    if($ENV{platform} STREQUAL "mac-x86")
        message ("   Building cross-platform for mac/x86_64")
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Archs to build")
    elseif($ENV{platform} STREQUAL "mac-arm")
        message ("   Building cross-platform for mac/arm64")
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Archs to build")
    elseif($ENV{platform} STREQUAL "mac")
        message ("   Building cross-platform for both mac/x86_64 and mac/arm64")
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Archs to build")
    endif()
else()
    # No 'platform' defined could mean running from command line, assume we build universal image in one go via XCode
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Archs to build")
endif()

option(XPMP2_BUILD_SHARED_LIBS "Build XPMP2 as a shared libraries" OFF)

project(XPMP2
        VERSION 3.6.0
        DESCRIPTION "Multiplayer library for X-Plane 11 and 12")

# Source file list
set(SOURCE_FILES
    inc/XPCAircraft.h
    inc/XPMPAircraft.h
    inc/XPMPMultiplayer.h
    inc/XPMPRemote.h
    inc/XPMPPlaneRenderer.h
    inc/XPMPExport.h
    src/2D.h
    src/2D.cpp
    src/AIMultiplayer.h
    src/AIMultiplayer.cpp
    src/Aircraft.h
    src/Aircraft.cpp
    src/Contrail.cpp
    src/CSLCopy.cpp
    src/CSLModels.h
    src/CSLModels.cpp
    src/Map.h
    src/Map.cpp
    src/Network.h
    src/Network.cpp
    src/RelatedDoc8643.h
    src/RelatedDoc8643.cpp
    src/Remote.h
    src/Remote.cpp
    src/Sound.h
    src/Sound.cpp
    src/Utilities.h
    src/Utilities.cpp
    src/XPMP2.h
    src/XPMPMultiplayer.cpp)

# Support for finding the X-Plane SDK
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(NOT XPlaneSDK_DIR)
    set(XPlaneSDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDK")
endif()

if(XPMP2_BUILD_SHARED_LIBS)
    if (NOT WIN32)
        message (FATAL_ERROR "Building a shared library is only supported for Windows!")
    endif()
    find_package(XPlaneSDK REQUIRED XPLM)                           # For a dynamic library need to link to XPLM
    add_library(XPMP2 SHARED ${SOURCE_FILES} "XPMP2.rc") 
    set_target_properties(XPMP2 PROPERTIES                          # Add version number to DLL name to reduce risk of version conflicts
        OUTPUT_NAME "XPMP2-${PROJECT_VERSION}"
        VERSION ${PROJECT_VERSION}
    )   
    target_compile_definitions(XPMP2 PUBLIC XPMP2_DLLEXPORT)        # Make sure symbols get exported to DLL, see XPMPExport.h
    target_link_libraries(XPMP2 PRIVATE wsock32 ws2_32 iphlpapi XPlaneSDK::XPLM)
    # Write the dynamic output file name into a file, so that Github actions can pick it up for uplading the artifact
    file(GENERATE OUTPUT "XPMP2_FileName.txt" CONTENT $<TARGET_FILE_BASE_NAME:XPMP2>)
else()
    find_package(XPlaneSDK REQUIRED)                                # For a static library we don't need any SDK lib to link
    add_library(XPMP2 STATIC ${SOURCE_FILES})
endif()

# Provide compile macros from the above project version definition
target_compile_definitions(XPMP2 PRIVATE
    XPMP2_VERSION="${PROJECT_VERSION}"
    XPMP2_VER_MAJOR=${PROJECT_VERSION_MAJOR}
    XPMP2_VER_MINOR=${PROJECT_VERSION_MINOR}
    XPMP2_VER_PATCH=${PROJECT_VERSION_PATCH}
)

message ("== Building: ${PROJECT_NAME} ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} ==")
message ("Compiler Info:")
message ("CMAKE_CXX_COMPILER_ID         = ${CMAKE_CXX_COMPILER_ID}")
message ("CMAKE_CXX_COMPILER_VERSION    = ${CMAKE_CXX_COMPILER_VERSION}")
message ("CMAKE_CXX_COMPILER            = ${CMAKE_CXX_COMPILER}")
message ("WIN32 / MSVC / MINGW          = ${WIN32} / ${MSVC} / ${MINGW}")
message ("UNIX / APPLE                  = ${UNIX} / ${APPLE}")
if (APPLE)
    message ("OSX_SDK_PATH                  = $ENV{OSX_SDK_PATH}")
    message ("CMAKE_OSX_ARCHITECTURES       = ${CMAKE_OSX_ARCHITECTURES}")
endif()
message ("X-Plane SDK Headers           = ${XPlaneSDK_INCLUDE_DIRS}")
message ("Building Shared Lib           = ${XPMP2_BUILD_SHARED_LIBS}")
message ("Build Type                    = ${CMAKE_BUILD_TYPE}")

################################################################################
# Target Systems
################################################################################

# Windows: Target Windows 7.0 and later
if (WIN32)
    target_compile_definitions(XPMP2 PRIVATE _WIN32_WINNT=0x0601)
    if (NOT DEFINED ENV{platform})
        set(ENV{platform} "win")
    endif()
elseif(APPLE)
    # MacOS 10.15 is minimum system requirement for X-Plane 12
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
    target_compile_options(XPMP2 PRIVATE -mmacosx-version-min=10.15)
    target_link_options(XPMP2 PRIVATE  -mmacosx-version-min=10.15)
endif()

################################################################################
# C++ Standard required
################################################################################

set_property(TARGET XPMP2 PROPERTY CXX_STANDARD 17)

################################################################################
# Compile Options
################################################################################

# Enable all X-Plane SDK APIs up to the newest version.
target_compile_definitions(XPMP2 PUBLIC XPLM200=1 XPLM210=1 XPLM300=1 XPLM301=1 XPLM303=1 XPLM400=1)

# Define platform macros.
target_compile_definitions(XPMP2 PUBLIC APL=$<BOOL:${APPLE}> IBM=$<BOOL:${WIN32}> LIN=$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${APPLE}>>>)

# Enable stricter warnings and then disable some we are not interested in.
# For XPMP2 compile, we don't need to be warned about our self-defined depreciations
if (MSVC)
    target_compile_options(XPMP2 PRIVATE /DWIN32 /D_WINDOWS /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS /GS- /Gd /Gy- /Zc:inline /Zc:__cplusplus /W4 /EHsc /fp:fast /diagnostics:caret /GL-)
    target_compile_options(XPMP2 PRIVATE /wd4996 /wd4068)
else()
    target_compile_options(XPMP2 PRIVATE -Wall -Wshadow -Wextra -Wno-deprecated-declarations)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 9.0 AND NOT APPLE)
        target_compile_options(XPMP2 PRIVATE -Wno-stringop-truncation)
    endif()

    # Force-enable exception support. This is most likely redundant, although for C
    # code the default is the opposite. Since we are mixing C++ and C libraries,
    # safer to set it on?
    target_compile_options(XPMP2 PRIVATE -fexceptions)

    # Makes symbols non-exported by default.
    target_compile_options(XPMP2 PRIVATE -fvisibility=hidden)
endif()

# Debug vs Release build
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(XPMP2 PRIVATE DEBUG=1)
    if (MSVC)
        # Multi-threaded-debug (/MDd), No optimizations (/Od), debug info into the lib (/Z7), and some generated run-time checks (/RTC1)
        target_compile_options(XPMP2 PRIVATE /MDd /Od /RTC1 /Z7)
    else()
        # No optimizations (-O0), debug info into the lib (-g), position-independent code (-fPIC)
        target_compile_options(XPMP2 PRIVATE -O0 -g -fPIC)
    endif()
else()
    target_compile_definitions(XPMP2 PRIVATE NDEBUG=1)
    if(MSVC)
        # Multi-threaded (/MD), full optimization (/O2), debug info into separate -pdb file
        target_compile_options(XPMP2 PRIVATE /MD /O2 /Zi)
    else()
        # Full optimizations (-O3), no debug info, position-independent code (-fPIC)
        target_compile_options(XPMP2 PRIVATE -O3 -fPIC)
    endif()
endif()

# Mingw Threads (if not already defined by an outer target)
if (MINGW AND NOT TARGET mingw_stdthreads)
    option(MINGW_STDTHREADS_GENERATE_STDHEADERS "" ON)
    add_subdirectory(lib/mingw-std-threads)
endif()

################################################################################
# XPMP2 Library
################################################################################

# Define pre-compiled header
target_precompile_headers(XPMP2 PRIVATE src/XPMP2.h)

# Header include directories
target_include_directories(XPMP2
	PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/fmod/inc
		${XPlaneSDK_INCLUDE_DIRS}
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Some files are known to do float comparison, but are good with that
if (NOT MSVC)
    set_source_files_properties(
        src/Remote.cpp
    PROPERTIES
        COMPILE_FLAGS -Wno-float-equal)
endif()

# Include MingW threads
if (MINGW)
    target_link_libraries(XPMP2 mingw_stdthreads)
endif()

# FMOD Library support if requested
# (The actual FMOD lib is not included here, that needs to be done by the plugin target)
if(INCLUDE_FMOD_SOUND)
    # Compile options
    target_compile_definitions(XPMP2 PRIVATE INCLUDE_FMOD_SOUND=1)
    target_sources(XPMP2 PRIVATE
        src/SoundFMOD.h
        src/SoundFMOD.cpp
    )
    # In case of the DLL we need to link with FMOD, too
    if(XPMP2_BUILD_SHARED_LIBS)
        find_library(FMOD_LIBRARY
            NAMES fmod_vc.lib
            PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/fmod
        )
        if (NOT FMOD_LIBRARY)
            message (FATAL_ERROR "Did not find fmod_vc.lib in ${CMAKE_CURRENT_SOURCE_DIR}/lib/fmod")
        endif()
        message ("FMOD_LIBRARY                  = ${FMOD_LIBRARY}")
        target_link_libraries(XPMP2 PRIVATE ${FMOD_LIBRARY})
    endif()
endif()

# make it a framework
if(APPLE)
    list(APPEND XPMP2_PUB_HEADERS
        ../inc/XPCAircraft.h
        ../inc/XPMPAircraft.h
        ../inc/XPMPMultiplayer.h
        ../inc/XPMPPlaneRenderer.h
        ../inc/XPMPRemote.h
        ../inc/XPMPExport.h
    )

    set_target_properties(XPMP2 PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION ${PROJECT_VERSION}
        MACOSX_FRAMEWORK_IDENTIFIER com.twinfan.XPMP2
        MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        VERSION ${PROJECT_VERSION}.0
        SOVERSION ${PROJECT_VERSION}.0
        PUBLIC_HEADER "${XPMP2_PUB_HEADERS}"
    )
endif()
