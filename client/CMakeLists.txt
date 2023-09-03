cmake_minimum_required(VERSION 3.25)
project(xPilot LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(MSGPACK_USE_BOOST OFF CACHE INTERNAL "" FORCE)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
endif()

if(WIN32)
    set(CMAKE_CXX_EXTENSIONS OFF)

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MTd")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    endif()
endif()

# Configure Qt
set(QT_MAJOR_VERSION 6)

if(DEFINED ENV{Qt${QT_MAJOR_VERSION}_HOME})
    message(STATUS "Looking for Qt in: " $ENV{Qt${QT_MAJOR_VERSION}_HOME})
else()
    message(STATUS "Qt${QT_MAJOR_VERSION}_HOME environment variable not set. Checking default paths.")
endif()

if("${QT_MAJOR_VERSION}" STREQUAL "6")
    set(CMAKE_PREFIX_PATH $ENV{Qt${QT_MAJOR_VERSION}_HOME})
endif()

find_package(Qt${QT_MAJOR_VERSION} COMPONENTS Core Quick Network Svg REQUIRED PATHS $ENV{Qt${QT_MAJOR_VERSION}_HOME})

# Read config variables
file(READ ${CMAKE_SOURCE_DIR}/../xpilot.json XPILOT_JSON)

string(JSON VERSION_URL GET ${XPILOT_JSON} version_url)
string(JSON IS_BETA_VERSION GET ${XPILOT_JSON} is_beta)
string(JSON BETA_NUMBER GET ${XPILOT_JSON} beta_number)
string(JSON VERSION_MAJOR GET ${XPILOT_JSON} version major)
string(JSON VERSION_MINOR GET ${XPILOT_JSON} version minor)
string(JSON VERSION_PATCH GET ${XPILOT_JSON} version patch)

if (IS_BETA_VERSION)
    set(IS_BETA_VERSION true)
else()
    set(IS_BETA_VERSION false)
endif()

if (NOT VATSIM_CLIENT_ID)
    set(VATSIM_CLIENT_ID 0)
endif ()

if (NOT VATSIM_CLIENT_KEY)
    set(VATSIM_CLIENT_KEY "")
endif ()

if (NOT CONFIG_ENCRYPTION_KEY)
    set(CONFIG_ENCRYPTION_KEY 0)
endif ()

# Set some Win32 Specific Settings
if (WIN32)
    set(gui_TYPE WIN32)
endif (WIN32)

# Set some Apple MacOS Specific settings
if (APPLE)
    set(gui_TYPE MACOSX_BUNDLE)
endif ()

configure_file(src/common/build_config.cpp.in ${CMAKE_BINARY_DIR}/generated/build_config.cpp)
include_directories(${CMAKE_BINARY_DIR}/generated)
configure_file(xpilot.rc.in ${CMAKE_SOURCE_DIR}/xpilot.rc @ONLY)

include_directories(${Qt6Quick_PRIVATE_INCLUDE_DIRS})

file(GLOB_RECURSE xpilot_SRC CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.h
    ${CMAKE_BINARY_DIR}/generated/build_config.cpp
)

file(
    GLOB_RECURSE afv_HEADERS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/afv-native/include/*.h*"
)

file(
    GLOB_RECURSE afv_SRC
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/afv-native/src/*"
    "${CMAKE_SOURCE_DIR}/afv-native/extern/compressor/*"
)

qt_add_resources(qrc_SOURCES qml.qrc)

if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set(xpilot_ICON ${CMAKE_CURRENT_SOURCE_DIR}/icon.icns)
    set_source_files_properties(${xpilot_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

add_executable(${PROJECT_NAME} ${gui_TYPE} ${xpilot_ICON} ${xpilot_SRC} ${afv_HEADERS} ${afv_SRC} ${qrc_SOURCES} xpilot.rc)

qt6_import_qml_plugins(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/afv-native/include")
source_group("AFV Headers" FILES ${afv_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/afv-native/src")
source_group("AFV Sources" FILES ${afv_SRC})

# Include the /src/ directory as a system include directory
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/afv-native/extern/cpp-jwt/include
    ${CMAKE_SOURCE_DIR}/afv-native/extern/simpleSource
    ${CMAKE_SOURCE_DIR}/afv-native/extern
    PUBLIC
    ${CMAKE_SOURCE_DIR}/afv-native/include)

add_subdirectory(${CMAKE_SOURCE_DIR}/../dependencies/msgpack-c ${CMAKE_CURRENT_BINARY_DIR}/msgpack-c)
add_subdirectory(${CMAKE_SOURCE_DIR}/../dependencies/json ${CMAKE_CURRENT_BINARY_DIR}/json)
add_subdirectory(${CMAKE_SOURCE_DIR}/../dependencies/qtpromise ${CMAKE_CURRENT_BINARY_DIR}/qtpromise)
add_subdirectory(${CMAKE_SOURCE_DIR}/../dependencies/vatsim-auth ${CMAKE_CURRENT_BINARY_DIR}/vatsim-auth)

# Check the system name and set the platform variable
set(MY_PLATFORM "")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(MY_PLATFORM "windows")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(MY_PLATFORM "linux")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(MY_PLATFORM "macos")
endif()

# Set library search paths for each platform
set(LIB_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}/../dependencies/platform/${MY_PLATFORM}
    ${CMAKE_SOURCE_DIR}/../dependencies/platform/${MY_PLATFORM}/nng
    ${CMAKE_SOURCE_DIR}/../dependencies/platform/${MY_PLATFORM}/openssl
    ${CMAKE_SOURCE_DIR}/../dependencies/platform/${MY_PLATFORM}/libevent
    ${CMAKE_SOURCE_DIR}/../dependencies/platform/${MY_PLATFORM}/libcurl
    ${CMAKE_SOURCE_DIR}/../dependencies/platform/${MY_PLATFORM}/libzip
)

# Library finding function
function(find_and_set_library VAR_NAME LIB_NAME)
    find_library(${VAR_NAME} ${LIB_NAME} PATHS ${LIB_SEARCH_PATHS} NO_DEFAULT_PATH)
    if(NOT ${VAR_NAME})
        message(FATAL_ERROR "${LIB_NAME} library not found")
    endif()
    message(STATUS "${LIB_NAME}: ${${VAR_NAME}}")
endfunction()

# Find libraries
find_and_set_library(LIB_OPUS opus)
find_and_set_library(LIB_SPEEXDSP speexdsp)
find_and_set_library(LIB_NNG nng)
find_and_set_library(LIB_CRYPTO crypto)
find_and_set_library(LIB_SSL ssl)
find_and_set_library(LIB_EVENT event)
find_and_set_library(LIB_CURL curl)
find_and_set_library(LIB_ZIP zip)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../dependencies/platform/${MY_PLATFORM}/nng/include
    ${CMAKE_SOURCE_DIR}/../dependencies/platform/${MY_PLATFORM}/openssl/include
    ${CMAKE_SOURCE_DIR}/../dependencies/platform/${MY_PLATFORM}/libevent/include
    ${CMAKE_SOURCE_DIR}/../dependencies/platform/${MY_PLATFORM}/libcurl/include
    ${CMAKE_SOURCE_DIR}/../dependencies/platform/${MY_PLATFORM}/libzip/include
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
        ${COREFOUNDATION}
        ${COREAUDIO}
        ${AUDIOTOOLBOX}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Quick
    Qt${QT_MAJOR_VERSION}::Network
    Qt${QT_MAJOR_VERSION}::Svg
    Qt${QT_MAJOR_VERSION}::qmlfolderlistmodelplugin
    ${LIB_OPUS}
    ${LIB_EVENT}
    ${LIB_CURL}
    ${LIB_SSL}
    ${LIB_CRYPTO}
    ${LIB_SPEEXDSP}
    ${LIB_VATSIM_AUTH}
    ${LIB_NNG}
    ${LIB_ZIP}
    msgpack-cxx
    vatsim-auth
    qtpromise
    nlohmann_json
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
)

add_definitions(-DCURL_STATICLIB -DNNG_STATIC_LIB)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        _USE_MATH_DEFINES
    )
endif()
