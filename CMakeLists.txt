cmake_minimum_required(VERSION 3.14)
project(xPilot LANGUAGES CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick REQUIRED)

AUX_SOURCE_DIRECTORY("${CMAKE_SOURCE_DIR}/protobuf" PROTOBUF_SRC)

set(PROJECT_SOURCES
    src/main.cpp
    src/interprocess.cpp
    src/interprocess.h
    src/radiostack.h
    src/appconfig.h
    src/nearbyatc.h
    src/base64.hpp
    qml.qrc
    ${PROTOBUF_SRC}
)

# Set some Win32 Specific Settings
IF(WIN32)
SET(GUI_TYPE WIN32)
ENDIF(WIN32)
# Set some Apple MacOS Specific settings
IF (APPLE)
SET(GUI_TYPE MACOSX_BUNDLE)
ENDIF (APPLE)

add_executable(xPilot
  ${GUI_TYPE}
  ${PROJECT_SOURCES}
  ${HEADERS}
  xpilot.rc
)

target_compile_definitions(xPilot
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(xPilot
  PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Quick)

find_package(Protobuf REQUIRED)
if(Protobuf_FOUND)
        message("Found Protobuf library")
        message(${Protobuf_INCLUDE_DIRS})
        include_directories(${Protobuf_INCLUDE_DIRS})
        target_link_libraries(xPilot PRIVATE ${Protobuf_LIBRARIES})
endif(Protobuf_FOUND)
