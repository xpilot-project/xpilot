cmake_minimum_required(VERSION 3.19)
project(xPilot LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/boost)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
endif()

if(WIN32)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT ")
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

list(FILTER afv_SRC EXCLUDE REGEX ".*SoundIOAudioDevice\\.(h|cpp)$")

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

add_subdirectory(3rdparty/qtpromise)
add_subdirectory(3rdparty/msgpack-c)
add_subdirectory(3rdparty/json)
add_subdirectory(afv-native/extern/portaudio)

if (WIN32)
    find_library(LIB_VATSIM_AUTH vatsimauth ${CMAKE_SOURCE_DIR}/externals/windows)
    find_library(LIB_OPUS opus ${CMAKE_SOURCE_DIR}/externals/windows)
    find_library(LIB_SPEEXDSP speexdsp ${CMAKE_SOURCE_DIR}/externals/windows)

    #nng
    include_directories(${CMAKE_SOURCE_DIR}/externals/windows/nng/include)
    find_library(LIB_NNG nng ${CMAKE_SOURCE_DIR}/externals/windows/nng/lib)

    #openssl
    include_directories(${CMAKE_SOURCE_DIR}/externals/windows/openssl/include)
    find_library(LIB_CRYPTO libcrypto ${CMAKE_SOURCE_DIR}/externals/windows/openssl/lib)
    find_library(LIB_SSL libssl ${CMAKE_SOURCE_DIR}/externals/windows/openssl/lib)

    #libevent
    include_directories(${CMAKE_SOURCE_DIR}/externals/windows/libevent/include)
    find_library(LIB_EVENT event ${CMAKE_SOURCE_DIR}/externals/windows/libevent/lib)

    #libcurl
    include_directories(${CMAKE_SOURCE_DIR}/externals/windows/libcurl/include)
    find_library(LIB_CURL libcurl ${CMAKE_SOURCE_DIR}/externals/windows/libcurl/lib)

    #libzip
    include_directories(${CMAKE_SOURCE_DIR}/externals/windows/libzip/include)
    find_library(LIB_ZIP zip ${CMAKE_SOURCE_DIR}/externals/windows/libzip/lib)
endif ()

if (UNIX)
    set(VATSIM_AUTH_LIBRARY ${CMAKE_SOURCE_DIR}/externals/linux/vatsimauth.a)
    set(OPUS_LIBRARY ${CMAKE_SOURCE_DIR}/externals/linux/libopus.a)
    set(SPEEX_LIBRARY ${CMAKE_SOURCE_DIR}/externals/linux/libspeexdsp.a)
    find_library(ALSA_LIBRARY NAMES asound)
endif ()

if (APPLE)
    find_library(LIB_VATSIM_AUTH vatsimauth ${CMAKE_SOURCE_DIR}/externals/macos)
    find_library(LIB_OPUS opus ${CMAKE_SOURCE_DIR}/externals/macos)
    find_library(LIB_SPEEXDSP speexdsp ${CMAKE_SOURCE_DIR}/externals/macos)

    #nng
    include_directories(${CMAKE_SOURCE_DIR}/externals/macos/nng/include)
    find_library(LIB_NNG nng ${CMAKE_SOURCE_DIR}/externals/macos/nng)

    #openssl
    include_directories(${CMAKE_SOURCE_DIR}/externals/macos/openssl/include)
    find_library(LIB_CRYPTO crypto ${CMAKE_SOURCE_DIR}/externals/macos/openssl/lib)
    find_library(LIB_SSL ssl ${CMAKE_SOURCE_DIR}/externals/macos/openssl/lib)

    #libevent
    include_directories(${CMAKE_SOURCE_DIR}/externals/macos/libevent/include)
    find_library(LIB_EVENT event ${CMAKE_SOURCE_DIR}/externals/macos/libevent/lib)

    #libcurl
    include_directories(${CMAKE_SOURCE_DIR}/externals/macos/libcurl/include)
    find_library(LIB_CURL curl ${CMAKE_SOURCE_DIR}/externals/macos/libcurl/lib)

    #libzip
    include_directories(${CMAKE_SOURCE_DIR}/externals/macos/libzip/include)
    find_library(LIB_ZIP zip ${CMAKE_SOURCE_DIR}/externals/macos/libzip/lib)

    find_library(COREFOUNDATION CoreFoundation)
    find_library(COREAUDIO CoreAudio)
    find_library(AUDIOTOOLBOX AudioToolbox)

    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")

    add_custom_target(osx_bundle_dirs
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/bin/xPilot.app/Contents/Resources
        COMMAND cp ${CMAKE_SOURCE_DIR}/xpilot.icns ${CMAKE_CURRENT_BINARY_DIR}/bin/xPilot.app/Contents/Resources/xpilot.icns)

    add_dependencies(${PROJECT_NAME} osx_bundle_dirs)
endif ()

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
        ${COREFOUNDATION}
        ${COREAUDIO}
        ${AUDIOTOOLBOX})
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
        ${ALSA_LIBRARY})
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
    PortAudio
)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

if(MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib Wldap32.lib Iphlpapi.lib crypt32.lib)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _USE_MATH_DEFINES)
    add_definitions(-DCURL_STATICLIB -DNNG_STATIC_LIB)
endif()
