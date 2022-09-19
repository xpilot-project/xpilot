cmake_minimum_required(VERSION 3.19)
project(xPilot LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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

# Configure Conan

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

if(MSVC)
    if(NOT DEFINED CONAN_LINK_RUNTIME AND DEFINED CONAN_SETTINGS_COMPILER_RUNTIME)
        message(WARNING "Forcing MSVC runtime based on conan profile")
        # this means Conan is not running the build, so we need to fix our linker flags ourself.... although we can probably cheat...
        set(CONAN_LINK_RUNTIME "/${CONAN_SETTINGS_COMPILER_RUNTIME}")
        conan_set_vs_runtime()
    endif()
endif(MSVC)

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

if(NOT SENTRY_DSN)
    set(SENTRY_DSN "")
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

if (WIN32)
    set(VATSIM_AUTH_LIBRARY ${CMAKE_SOURCE_DIR}/externals/windows/vatsimauth.lib)
    set(OPUS_LIBRARY ${CMAKE_SOURCE_DIR}/externals/windows/opus.lib)
    set(PORTAUDIO_LIBRARY ${CMAKE_SOURCE_DIR}/externals/windows/portaudio.lib)
    set(SPEEX_LIBRARY ${CMAKE_SOURCE_DIR}/externals/windows/speexdsp.lib)
    set(ALSA_LIBRARY "")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CONAN_BIN_DIRS_SENTRY-CRASHPAD}/crashpad_handler.exe"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif ()

if (UNIX)
    set(VATSIM_AUTH_LIBRARY ${CMAKE_SOURCE_DIR}/externals/linux/vatsimauth.a)
    set(OPUS_LIBRARY ${CMAKE_SOURCE_DIR}/externals/linux/libopus.a)
    set(PORTAUDIO_LIBRARY ${CMAKE_SOURCE_DIR}/externals/linux/libportaudio.a)
    set(SPEEX_LIBRARY ${CMAKE_SOURCE_DIR}/externals/linux/libspeexdsp.a)

    find_library(ALSA_LIBRARY NAMES asound)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CONAN_BIN_DIRS_SENTRY-CRASHPAD}/crashpad_handler"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif ()

if (APPLE)
    set(VATSIM_AUTH_LIBRARY ${CMAKE_SOURCE_DIR}/externals/macos/vatsimauth.a)
    set(OPUS_LIBRARY ${CMAKE_SOURCE_DIR}/externals/macos/libopus.a)
    set(PORTAUDIO_LIBRARY ${CMAKE_SOURCE_DIR}/externals/macos/libportaudio.a)
    set(SPEEX_LIBRARY ${CMAKE_SOURCE_DIR}/externals/macos/libspeexdsp.a)
    set(ALSA_LIBRARY "")

    find_library(COREFOUNDATION CoreFoundation)
    find_library(COREAUDIO CoreAudio)
    find_library(AUDIOTOOLBOX AudioToolbox)

    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")

    add_custom_target(osx_bundle_dirs
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/bin/xPilot.app/Contents/Resources
        COMMAND cp ${CMAKE_SOURCE_DIR}/xpilot.icns ${CMAKE_CURRENT_BINARY_DIR}/bin/xPilot.app/Contents/Resources/xpilot.icns)

    add_dependencies(${PROJECT_NAME} osx_bundle_dirs)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CONAN_BIN_DIRS_SENTRY-CRASHPAD}/crashpad_handler"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif ()

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Quick
    Qt${QT_MAJOR_VERSION}::Network
    Qt${QT_MAJOR_VERSION}::Svg
    PUBLIC
    CONAN_PKG::openssl
    CONAN_PKG::msgpack-cxx
    CONAN_PKG::nlohmann_json
    CONAN_PKG::libcurl
    CONAN_PKG::libevent
    CONAN_PKG::sentry-native
    CONAN_PKG::sentry-crashpad
    CONAN_PKG::libzip
    CONAN_PKG::nng
    CONAN_PKG::boost
    ${OPUS_LIBRARY}
    ${PORTAUDIO_LIBRARY}
    ${SPEEX_LIBRARY}
    ${VATSIM_AUTH_LIBRARY}
    ${COREFOUNDATION}
    ${COREAUDIO}
    ${AUDIOTOOLBOX}
    ${ALSA_LIBRARY}
    qtpromise
)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

if(MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _USE_MATH_DEFINES)
endif()
