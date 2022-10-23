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

if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set(XPILOT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/icon.icns)
    set_source_files_properties(${XPILOT_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

add_executable(${PROJECT_NAME} ${GUI_TYPE} ${XPILOT_ICON} ${xpilot_SRC} ${afv_HEADERS} ${afv_SRC} ${qrc_SOURCES} xpilot.rc)

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

    #vatsimauth
    find_library(LIB_VATSIM_AUTH vatsimauth PATHS ${CMAKE_SOURCE_DIR}/../externals/windows NO_DEFAULT_PATH)
    if(NOT LIB_VATSIM_AUTH)
        message(FATAL_ERROR "vatsimauth library not found")
    endif()
    message(STATUS "vatsimauth: ${LIB_VATSIM_AUTH}")

    #opus
    find_library(LIB_OPUS opus PATHS ${CMAKE_SOURCE_DIR}/../externals/windows NO_DEFAULT_PATH )
    if(NOT LIB_OPUS)
        message(FATAL_ERROR "opus library not found")
    endif()
    message(STATUS "opus: ${LIB_OPUS}")

    #speexdsp
    find_library(LIB_SPEEXDSP speexdsp PATHS ${CMAKE_SOURCE_DIR}/../externals/windows NO_DEFAULT_PATH )
    if(NOT LIB_SPEEXDSP)
        message(FATAL_ERROR "speexdsp library not found")
    endif()
    message(STATUS "speexdsp: ${LIB_SPEEXDSP}")

    #nng
    include_directories(${CMAKE_SOURCE_DIR}/../externals/windows/nng/include)
    find_library(LIB_NNG nng PATHS ${CMAKE_SOURCE_DIR}/../externals/windows/nng NO_DEFAULT_PATH )
    if(NOT LIB_NNG)
        message(FATAL_ERROR "nng library not found")
    endif()
    message(STATUS "nng: ${LIB_NNG}")

    #openssl
    include_directories(${CMAKE_SOURCE_DIR}/../externals/windows/openssl/include)
    find_library(LIB_CRYPTO libcrypto PATHS ${CMAKE_SOURCE_DIR}/../externals/windows/openssl NO_DEFAULT_PATH )
    if(NOT LIB_CRYPTO)
        message(FATAL_ERROR "libcrypto library not found")
    endif()
    message(STATUS "libcrypto: ${LIB_CRYPTO}")

    find_library(LIB_SSL libssl PATHS ${CMAKE_SOURCE_DIR}/../externals/windows/openssl NO_DEFAULT_PATH)
    if(NOT LIB_SSL)
        message(FATAL_ERROR "libssl library not found")
    endif()
    message(STATUS "libssl: ${LIB_SSL}")

    #libevent
    include_directories(${CMAKE_SOURCE_DIR}/../externals/windows/libevent/include)
    find_library(LIB_EVENT event PATHS ${CMAKE_SOURCE_DIR}/../externals/windows/libevent NO_DEFAULT_PATH )
    if(NOT LIB_EVENT)
        message(FATAL_ERROR "event library not found")
    endif()
    message(STATUS "event: ${LIB_EVENT}")

    #libcurl
    include_directories(${CMAKE_SOURCE_DIR}/../externals/windows/libcurl/include)
    find_library(LIB_CURL libcurl PATHS ${CMAKE_SOURCE_DIR}/../externals/windows/libcurl NO_DEFAULT_PATH)
    if(NOT LIB_CURL)
        message(FATAL_ERROR "libcurl library not found")
    endif()
    message(STATUS "libcurl: ${LIB_CURL}")

    #libzip
    include_directories(${CMAKE_SOURCE_DIR}/../externals/windows/libzip/include)
    find_library(LIB_ZIP zip PATHS ${CMAKE_SOURCE_DIR}/../externals/windows/libzip NO_DEFAULT_PATH)
    if(NOT LIB_ZIP)
        message(FATAL_ERROR "zip library not found")
    endif()
    message(STATUS "zip: ${LIB_ZIP}")

elseif(APPLE)

    #vatsimauth
    find_library(LIB_VATSIM_AUTH vatsimauth PATHS ${CMAKE_SOURCE_DIR}/../externals/macos NO_DEFAULT_PATH )
    if(NOT LIB_VATSIM_AUTH)
        message(FATAL_ERROR "vatsimauth library not found")
    endif()
    message(STATUS "vatsimauth: ${LIB_VATSIM_AUTH}")

    #opus
    find_library(LIB_OPUS opus PATHS ${CMAKE_SOURCE_DIR}/../externals/macos NO_DEFAULT_PATH )
    if(NOT LIB_OPUS)
        message(FATAL_ERROR "opus library not found")
    endif()
    message(STATUS "opus: ${LIB_OPUS}")

    #speexdsp
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
else()

      #vatsimauth
      find_library(LIB_VATSIM_AUTH vatsimauth PATHS ${CMAKE_SOURCE_DIR}/../externals/linux NO_DEFAULT_PATH)
      if(NOT LIB_VATSIM_AUTH)
          message(FATAL_ERROR "vatsimauth library not found")
      endif()
      message(STATUS "vatsimauth: ${LIB_VATSIM_AUTH}")
  
      #opus
      find_library(LIB_OPUS opus PATHS ${CMAKE_SOURCE_DIR}/../externals/linux NO_DEFAULT_PATH )
      if(NOT LIB_OPUS)
          message(FATAL_ERROR "opus library not found")
      endif()
      message(STATUS "opus: ${LIB_OPUS}")
  
      #speexdsp
      find_library(LIB_SPEEXDSP speexdsp PATHS ${CMAKE_SOURCE_DIR}/../externals/linux NO_DEFAULT_PATH )
      if(NOT LIB_SPEEXDSP)
          message(FATAL_ERROR "speexdsp library not found")
      endif()
      message(STATUS "speexdsp: ${LIB_SPEEXDSP}")
  
      #nng
      include_directories(${CMAKE_SOURCE_DIR}/../externals/linux/nng/include)
      find_library(LIB_NNG nng PATHS ${CMAKE_SOURCE_DIR}/../externals/linux/nng NO_DEFAULT_PATH )
      if(NOT LIB_NNG)
          message(FATAL_ERROR "nng library not found")
      endif()
      message(STATUS "nng: ${LIB_NNG}")
  
      #openssl
      include_directories(${CMAKE_SOURCE_DIR}/../externals/linux/openssl/include)
      find_library(LIB_CRYPTO crypto PATHS ${CMAKE_SOURCE_DIR}/../externals/linux/openssl NO_DEFAULT_PATH )
      if(NOT LIB_CRYPTO)
          message(FATAL_ERROR "crypto library not found")
      endif()
      message(STATUS "crypto: ${LIB_CRYPTO}")
  
      find_library(LIB_SSL ssl PATHS ${CMAKE_SOURCE_DIR}/../externals/linux/openssl NO_DEFAULT_PATH)
      if(NOT LIB_SSL)
          message(FATAL_ERROR "ssl library not found")
      endif()
      message(STATUS "ssl: ${LIB_SSL}")
  
      #libevent
      include_directories(${CMAKE_SOURCE_DIR}/../externals/linux/libevent/include)
      find_library(LIB_EVENT event PATHS ${CMAKE_SOURCE_DIR}/../externals/linux/libevent NO_DEFAULT_PATH )
      if(NOT LIB_EVENT)
          message(FATAL_ERROR "event library not found")
      endif()
      message(STATUS "event: ${LIB_EVENT}")
  
      #libcurl
      include_directories(${CMAKE_SOURCE_DIR}/../externals/linux/libcurl/include)
      find_library(LIB_CURL curl PATHS ${CMAKE_SOURCE_DIR}/../externals/linux/libcurl NO_DEFAULT_PATH)
      if(NOT LIB_CURL)
          message(FATAL_ERROR "curl library not found")
      endif()
      message(STATUS "curl: ${LIB_CURL}")
  
      #libzip
      include_directories(${CMAKE_SOURCE_DIR}/../externals/linux/libzip/include)
      find_library(LIB_ZIP zip PATHS ${CMAKE_SOURCE_DIR}/../externals/linux/libzip NO_DEFAULT_PATH)
      if(NOT LIB_ZIP)
          message(FATAL_ERROR "zip library not found")
      endif()
      message(STATUS "zip: ${LIB_ZIP}")

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
    Qt${QT_MAJOR_VERSION}::qmlfolderlistmodelplugin
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
add_definitions(-DCURL_STATICLIB -DNNG_STATIC_LIB)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _USE_MATH_DEFINES)
endif()
