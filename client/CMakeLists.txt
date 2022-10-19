cmake_minimum_required(VERSION 3.19)
project(xPilot LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../externals/boost)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
endif()

if(WIN32)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
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
    set(GUI_TYPE WIN32)
endif (WIN32)

# Set some Apple MacOS Specific settings
if (APPLE)
    set(GUI_TYPE MACOSX_BUNDLE)
endif ()

configure_file(src/common/build_config.cpp.in ${CMAKE_BINARY_DIR}/generated/build_config.cpp)
include_directories(${CMAKE_BINARY_DIR}/generated)
configure_file(xpilot.rc.in ${CMAKE_SOURCE_DIR}/xpilot.rc @ONLY)

file(GLOB_RECURSE xpilot_SRC CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.h
    ${CMAKE_BINARY_DIR}/generated/build_config.cpp)

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

add_executable(${PROJECT_NAME} ${GUI_TYPE} ${xpilot_SRC} ${afv_HEADERS} ${afv_SRC} ${qrc_SOURCES} xpilot.rc)

qt6_import_qml_plugins(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/afv-native/include")
source_group("AFV Headers" FILES ${afv_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/afv-native/src")
source_group("AFV Sources" FILES ${afv_SRC})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/afv-native/extern/cpp-jwt/include
    ${CMAKE_SOURCE_DIR}/afv-native/extern/simpleSource
    ${CMAKE_SOURCE_DIR}/afv-native/extern
    PUBLIC
    ${CMAKE_SOURCE_DIR}/afv-native/include)

add_subdirectory(${CMAKE_SOURCE_DIR}/../externals/msgpack-c ${CMAKE_CURRENT_BINARY_DIR}/msgpack-c)
add_subdirectory(${CMAKE_SOURCE_DIR}/../externals/json ${CMAKE_CURRENT_BINARY_DIR}/json)
add_subdirectory(${CMAKE_SOURCE_DIR}/../externals/qtpromise ${CMAKE_CURRENT_BINARY_DIR}/qtpromise)

if (WIN32)

    find_library(LIB_VATSIM_AUTH vatsimauth ${CMAKE_SOURCE_DIR}/../externals/windows)
    find_library(LIB_OPUS opus ${CMAKE_SOURCE_DIR}/../externals/windows)
    find_library(LIB_SPEEXDSP speexdsp ${CMAKE_SOURCE_DIR}/../externals/windows)

    #nng
    include_directories(${CMAKE_SOURCE_DIR}/../externals/windows/nng/include)
    find_library(LIB_NNG nng ${CMAKE_SOURCE_DIR}/../externals/windows/nng)

    #openssl
    include_directories(${CMAKE_SOURCE_DIR}/../externals/windows/openssl/include)
    find_library(LIB_CRYPTO libcrypto ${CMAKE_SOURCE_DIR}/../externals/windows/openssl)
    find_library(LIB_SSL libssl ${CMAKE_SOURCE_DIR}/../externals/windows/openssl)

    #libevent
    include_directories(${CMAKE_SOURCE_DIR}/../externals/windows/libevent/include)
    find_library(LIB_EVENT event ${CMAKE_SOURCE_DIR}/../externals/windows/libevent)

    #libcurl
    include_directories(${CMAKE_SOURCE_DIR}/../externals/windows/libcurl/include)
    find_library(LIB_CURL libcurl ${CMAKE_SOURCE_DIR}/../externals/windows/libcurl)

    #libzip
    include_directories(${CMAKE_SOURCE_DIR}/../externals/windows/libzip/include)
    find_library(LIB_ZIP zip ${CMAKE_SOURCE_DIR}/../externals/windows/libzip)

elseif(APPLE)

    find_library(LIB_VATSIM_AUTH vatsimauth PATHS ${CMAKE_SOURCE_DIR}/../externals/macos NO_DEFAULT_PATH )
    if(NOT LIB_VATSIM_AUTH)
        message(FATAL_ERROR "vatsimauth library not found")
    endif()
    message(STATUS "vatsimauth: ${LIB_VATSIM_AUTH}")

    find_library(LIB_OPUS opus PATHS ${CMAKE_SOURCE_DIR}/../externals/macos NO_DEFAULT_PATH )
    if(NOT LIB_OPUS)
        message(FATAL_ERROR "opus library not found")
    endif()
    message(STATUS "opus: ${LIB_OPUS}")

    find_library(LIB_SPEEXDSP speexdsp PATHS ${CMAKE_SOURCE_DIR}/../externals/macos NO_DEFAULT_PATH )
    if(NOT LIB_SPEEXDSP)
        message(FATAL_ERROR "speexdsp library not found")
    endif()
    message(STATUS "speexdsp: ${LIB_SPEEXDSP}")

    #nng
    include_directories(${CMAKE_SOURCE_DIR}/../externals/macos/nng/include)
    find_library(LIB_NNG nng PATHS ${CMAKE_SOURCE_DIR}/../externals/macos/nng NO_DEFAULT_PATH )
    if(NOT LIB_NNG)
        message(FATAL_ERROR "nng library not found")
    endif()
    message(STATUS "nng: ${LIB_NNG}")

    #openssl
    include_directories(${CMAKE_SOURCE_DIR}/../externals/macos/openssl/include)
    find_library(LIB_CRYPTO crypto PATHS ${CMAKE_SOURCE_DIR}/../externals/macos/openssl NO_DEFAULT_PATH )
    if(NOT LIB_CRYPTO)
        message(FATAL_ERROR "crypto library not found")
    endif()
    message(STATUS "crypto: ${LIB_CRYPTO}")

    find_library(LIB_SSL ssl PATHS ${CMAKE_SOURCE_DIR}/../externals/macos/openssl NO_DEFAULT_PATH)
    if(NOT LIB_SSL)
        message(FATAL_ERROR "ssl library not found")
    endif()
    message(STATUS "ssl: ${LIB_SSL}")

    #libevent
    include_directories(${CMAKE_SOURCE_DIR}/../externals/macos/libevent/include)
    find_library(LIB_EVENT event PATHS ${CMAKE_SOURCE_DIR}/../externals/macos/libevent NO_DEFAULT_PATH )
    if(NOT LIB_EVENT)
        message(FATAL_ERROR "event library not found")
    endif()
    message(STATUS "event: ${LIB_EVENT}")

    #libcurl
    include_directories(${CMAKE_SOURCE_DIR}/../externals/macos/libcurl/include)
    find_library(LIB_CURL curl PATHS ${CMAKE_SOURCE_DIR}/../externals/macos/libcurl NO_DEFAULT_PATH)
    if(NOT LIB_CURL)
        message(FATAL_ERROR "curl library not found")
    endif()
    message(STATUS "curl: ${LIB_CURL}")

    #libzip
    include_directories(${CMAKE_SOURCE_DIR}/../externals/macos/libzip/include)
    find_library(LIB_ZIP zip PATHS ${CMAKE_SOURCE_DIR}/../externals/macos/libzip NO_DEFAULT_PATH)
    if(NOT LIB_ZIP)
        message(FATAL_ERROR "zip library not found")
    endif()
    message(STATUS "zip: ${LIB_ZIP}")

    find_library(COREFOUNDATION CoreFoundation)
    find_library(COREAUDIO CoreAudio)
    find_library(AUDIOTOOLBOX AudioToolbox)

    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")

    add_custom_target(osx_bundle_dirs
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/bin/xPilot.app/Contents/Resources
        COMMAND cp ${CMAKE_SOURCE_DIR}/icon.icns ${CMAKE_CURRENT_BINARY_DIR}/bin/xPilot.app/Contents/Resources/icon.icns)

    add_dependencies(${PROJECT_NAME} osx_bundle_dirs)

else()

    find_library(LIB_VATSIM_AUTH vatsimauth ${CMAKE_SOURCE_DIR}/../externals/linux)
    find_library(LIB_OPUS opus ${CMAKE_SOURCE_DIR}/../externals/linux)
    find_library(LIB_SPEEXDSP speexdsp ${CMAKE_SOURCE_DIR}/../externals/linux)

    #nng
    include_directories(${CMAKE_SOURCE_DIR}/../externals/linux/nng/include)
    find_library(LIB_NNG nng ${CMAKE_SOURCE_DIR}/../externals/linux/nng)

    #openssl
    include_directories(${CMAKE_SOURCE_DIR}/../externals/linux/openssl/include)
    find_library(LIB_CRYPTO crypto ${CMAKE_SOURCE_DIR}/../externals/linux/openssl)
    find_library(LIB_SSL ssl ${CMAKE_SOURCE_DIR}/../externals/linux/openssl)

    #libevent
    include_directories(${CMAKE_SOURCE_DIR}/../externals/linux/libevent/include)
    find_library(LIB_EVENT event ${CMAKE_SOURCE_DIR}/../externals/linux/libevent)

    #libcurl
    include_directories(${CMAKE_SOURCE_DIR}/../externals/linux/libcurl/include)
    find_library(LIB_CURL curl ${CMAKE_SOURCE_DIR}/../externals/linux/libcurl)

    #libzip
    include_directories(${CMAKE_SOURCE_DIR}/../externals/linux/libzip/include)
    find_library(LIB_ZIP zip ${CMAKE_SOURCE_DIR}/../externals/linux/libzip)

endif ()

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
        ${COREFOUNDATION}
        ${COREAUDIO}
        ${AUDIOTOOLBOX})
endif()

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Quick
    Qt${QT_MAJOR_VERSION}::Network
    Qt${QT_MAJOR_VERSION}::Svg
    PUBLIC
    ${LIB_OPUS}
    ${LIB_VATSIM_AUTH}
    ${LIB_EVENT}
    ${LIB_CURL}
    ${LIB_CRYPTO}
    ${LIB_SSL}
    ${LIB_SPEEXDSP}
    ${LIB_VATSIM_AUTH}
    ${LIB_NNG}
    ${LIB_ZIP}
    qtpromise
    msgpackc-cxx
    nlohmann_json
)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

if(MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib Wldap32.lib Iphlpapi.lib crypt32.lib)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _USE_MATH_DEFINES)
    add_definitions(-DCURL_STATICLIB -DNNG_STATIC_LIB)
endif()
