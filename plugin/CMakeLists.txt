cmake_minimum_required(VERSION 3.16)
project(xpilot LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED 17)
set_property(GLOBAL PROPERTY CXX_STANDARD 17)

set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

set(Boost_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../externals/boost)

file(READ ${CMAKE_SOURCE_DIR}/../xpilot.json XPILOT_JSON)
string(JSON IS_BETA_VERSION GET ${XPILOT_JSON} is_beta)
string(JSON BETA_NUMBER GET ${XPILOT_JSON} beta_number)
string(JSON VERSION_MAJOR GET ${XPILOT_JSON} version major)
string(JSON VERSION_MINOR GET ${XPILOT_JSON} version minor)
string(JSON VERSION_PATCH GET ${XPILOT_JSON} version patch)

if (IS_BETA_VERSION)
    set(IS_BETA_VERSION true)
    set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-beta.${BETA_NUMBER})
else()
    set(IS_BETA_VERSION false)
    set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
endif()

configure_file(${CMAKE_SOURCE_DIR}/include/Constants.h.in ${CMAKE_SOURCE_DIR}/include/Constants.h @ONLY)

if(WIN32)
    include_directories(${CMAKE_SOURCE_DIR}/../externals/windows/nng/include)
    find_library(LIB_NNG nng ${CMAKE_SOURCE_DIR}/../externals/windows/nng)
elseif(APPLE)
    include_directories(${CMAKE_SOURCE_DIR}/../externals/macos/nng/include)
    find_library(LIB_NNG nng ${CMAKE_SOURCE_DIR}/../externals/macos/nng)
elseif(UNIX)
    include_directories(${CMAKE_SOURCE_DIR}/../externals/linux/nng/include)
    find_library(LIB_NNG nng ${CMAKE_SOURCE_DIR}/../externals/linux/nng)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/../externals/msgpack-c ${CMAKE_CURRENT_BINARY_DIR}/msgpack-c)
add_subdirectory(${CMAKE_SOURCE_DIR}/../externals/json ${CMAKE_CURRENT_BINARY_DIR}/json)
add_subdirectory(3rdparty/XPMP2)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/include/*.h)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/imgui)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/imgui-stdlib)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/imgwindow)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/font)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/gmath)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/fmod/include)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/sdk/CHeaders/XPLM)

set(INCLUDES
  include/AircraftManager.h
  include/AudioEngine.h
  include/Config.h
  include/Constants.h
  include/Dto.h
  include/DataRefAccess.h
  include/FrameRateMonitor.h
  include/NearbyATCWindow.h
  include/NetworkAircraft.h
  include/NotificationPanel.h
  include/OwnedDataRef.h
  include/Plugin.h
  include/SettingsWindow.h
  include/Stopwatch.h
  include/TerrainProbe.h
  include/TextMessageConsole.h
  include/Utilities.h
  include/XPilot.h
  include/XPilotAPI.h
  include/XplaneCommand.h)

source_group("Header Files" FILES ${INCLUDES})

set(SRC
  src/AircraftManager.cpp
  src/AudioEngine.cpp
  src/Config.cpp
  src/DataRefAccess.cpp
  src/FrameRateMonitor.cpp
  src/NearbyATCWindow.cpp
  src/NetworkAircraft.cpp
  src/NotificationPanel.cpp
  src/OwnedDataRef.cpp
  src/Plugin.cpp
  src/SettingsWindow.cpp
  src/Stopwatch.cpp
  src/TerrainProbe.cpp
  src/TextMessageConsole.cpp
  src/XPilot.cpp
  3rdparty/imgui/imgui.cpp
  3rdparty/imgui/imgui_draw.cpp
  3rdparty/imgui/imgui_tables.cpp
  3rdparty/imgui/imgui_widgets.cpp
  3rdparty/imgui-stdlib/imgui_stdlib.cpp
  3rdparty/imgwindow/XPImgWindow.cpp
  3rdparty/imgwindow/ImgFontAtlas.cpp
  3rdparty/imgwindow/ImgWindow.cpp)

source_group("Source Files" FILES ${SRC})
set(ALL_FILES ${INCLUDES} ${SRC})
add_library(${PROJECT_NAME} MODULE ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/3rdparty)

if (APPLE)
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/3rdparty/sdk/Libraries/Mac")
elseif (WIN32)
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/3rdparty/sdk/Libraries/Win")
endif()

# Link FMOD
list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/3rdparty/fmod")

if(WIN32)
    find_library(FMOD_LIBRARY NAMES fmod_vc.lib REQUIRED)
    message ("FMOD_LIBRARY = ${FMOD_LIBRARY}")
    target_link_libraries(${PROJECT_NAME} ${FMOD_LIBRARY})
elseif(APPLE)
    find_library(FMOD_LIBRARY NAMES libfmod.dylib REQUIRED)
    message ("FMOD_LIBRARY = ${FMOD_LIBRARY}")
    target_link_libraries(${PROJECT_NAME} ${FMOD_LIBRARY})
elseif(UNIX AND NOT APPLE)
    find_library(FMOD_LIBRARY NAMES libfmod.so REQUIRED)
    message ("FMOD_LIBRARY = ${FMOD_LIBRARY}")
    target_link_libraries(${PROJECT_NAME} ${FMOD_LIBRARY})
endif()

target_link_libraries(${PROJECT_NAME} 
    msgpackc-cxx
    nlohmann_json
    XPMP2
    ${LIB_NNG})

if (WIN32 OR APPLE)
    find_library(XPLM_LIBRARY NAMES XPLM XPLM_64.lib)
    target_link_libraries(${PROJECT_NAME} ${XPLM_LIBRARY})
endif()

if (WIN32)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(${PROJECT_NAME} ws2_32.lib wldap32.lib advapi32.lib crypt32.lib wsock32.lib opengl32.lib iphlpapi.lib)
    target_link_options(${PROJECT_NAME} PUBLIC /DEBUG)
elseif (APPLE)
    # Link OS X core system libraries.
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation REQUIRED)
    find_library(Cocoa_LIBRARY Cocoa REQUIRED)
    find_library(Security_LIBRARY Security REQUIRED)
    find_library(GSS_LIBRARY GSS REQUIRED)
    find_library(OpenGL_LIBRARY OpenGL REQUIRED)

    target_link_libraries(
      ${PROJECT_NAME}
      ${CORE_FOUNDATION_LIBRARY}
      ${Cocoa_LIBRARY}
      ${Security_LIBRARY}
      ${GSS_LIBRARY}
      ${OpenGL_LIBRARY})

    # Restrict set of symbols exported from the plugin to the ones required by XPLM:
    target_link_libraries(${PROJECT_NAME} "-exported_symbols_list ${CMAKE_SOURCE_DIR}/src/Xpilot.sym_mac")
elseif (UNIX)
    # Link library for dynamic loading of shared objects on UNIX systems.
    find_library(DL_LIBRARY dl REQUIRED)
    # Threads
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${DL_LIBRARY} Threads::Threads)
    # Specify additional runtime search paths for dynamically-linked libraries.
    # Restrict set of symbols exported from the plugin to the ones required by XPLM:
    target_link_libraries(${PROJECT_NAME} -Wl,--version-script -Wl,${CMAKE_SOURCE_DIR}/src/Xpilot.sym)
endif ()

# Link OpenGL
set (OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)  # apt install freeglut3-dev
include_directories(${OpenGL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenGL_LIBRARIES})

# Enable all X-Plane SDK APIs up to the newest version.
add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1 -DXPLM303=1)

# Define platform macros.
add_definitions(-DAPL=$<BOOL:${APPLE}> -DIBM=$<BOOL:${WIN32}> -DLIN=$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${APPLE}>>>)

add_definitions(-DNNG_STATIC_LIB)

if (UNIX OR APPLE)
    add_compile_options(-fexceptions -fpermissive)
    add_compile_options(-fvisibility=hidden)
    add_compile_options(-Wall -Wshadow -Wfloat-equal -Wextra)
    add_compile_options(-Wno-unused)
    add_compile_options(-O3 -fPIC)
    add_compile_options(-pthread)
elseif(WIN32)
    add_compile_options(/wo4996)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
endif()

if (MSVC)
   set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/win_x64")
   set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO	"${CMAKE_BINARY_DIR}/win_x64")
   set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/win_x64")
   set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/win_x64")
elseif (APPLE)
   set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mac_x64")
elseif (UNIX)
   set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lin_x64")
endif()

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "xPilot"
    SUFFIX ".xpl"
)