cmake_minimum_required(VERSION 3.16)
project(xpilot LANGUAGES C CXX VERSION 2.0.0)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

if(APPLE)
    add_compile_options(-mmacosx-version-min=10.15)
    add_link_options(-mmacosx-version-min=10.15)
endif()

set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED 17)
set_property(GLOBAL PROPERTY CXX_STANDARD 17)

if(UNIX AND NOT APPLE)
    # fix rpath on linux
    SET(CMAKE_SKIP_BUILD_RPATH FALSE)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    SET(CMAKE_INSTALL_RPATH "\$ORIGIN")
endif()

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

if(MSVC)
	if(NOT DEFINED CONAN_LINK_RUNTIME AND DEFINED CONAN_SETTINGS_COMPILER_RUNTIME)
		message(WARNING "Forcing MSVC runtime based on conan profile")
		# this means Conan is not running the build, so we need to fix our linker flags ourself.... although we can probably cheat...
		set(CONAN_LINK_RUNTIME "/${CONAN_SETTINGS_COMPILER_RUNTIME}")
		conan_set_vs_runtime()
	endif()
endif(MSVC)

if (WIN32)
    # Compile as multithreaded plugin on Windows
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
endif ()

include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/externals/Font")
include_directories("${CMAKE_SOURCE_DIR}/externals/gmath")
include_directories("${CMAKE_SOURCE_DIR}/externals/imgui")
include_directories("${CMAKE_SOURCE_DIR}/externals/imgui-stdlib")
include_directories("${CMAKE_SOURCE_DIR}/externals/ImgWindow")
include_directories("${CMAKE_SOURCE_DIR}/externals/XPMP2")
include_directories("${CMAKE_SOURCE_DIR}/externals/SDK/CHeaders/XPLM")
include_directories("${CMAKE_SOURCE_DIR}/externals/fmod/include")

set(Header_Files
  include/AircraftManager.h
  include/AudioEngine.h
  include/Config.h
  include/Constants.h
  include/DataRefAccess.h
  include/FrameRateMonitor.h
  include/NearbyATCWindow.h
  include/NetworkAircraft.h
  include/NetworkAircraftConfig.h
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

source_group("Header Files" FILES ${Header_Files})

set(Source_Files
  src/AircraftManager.cpp
  src/AudioEngine.cpp
  src/Config.cpp
  src/DataRefAccess.cpp
  src/FrameRateMonitor.cpp
  src/NearbyATCWindow.cpp
  src/NetworkAircraft.cpp
  src/NetworkAircraftConfig.cpp
  src/NotificationPanel.cpp
  src/OwnedDataRef.cpp
  src/Plugin.cpp
  src/SettingsWindow.cpp
  src/Stopwatch.cpp
  src/TerrainProbe.cpp
  src/TextMessageConsole.cpp
  src/XPilot.cpp
  externals/imgui/imgui.cpp
  externals/imgui/imgui_draw.cpp
  externals/imgui/imgui_tables.cpp
  externals/imgui/imgui_widgets.cpp
  externals/imgui-stdlib/imgui_stdlib.cpp
  externals/ImgWindow/XPImgWindow.cpp
  externals/ImgWindow/ImgFontAtlas.cpp
  externals/ImgWindow/ImgWindow.cpp)

source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES  ${Header_Files} ${Source_Files})

add_library(${PROJECT_NAME} MODULE ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/externals)

if (APPLE)
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/externals/XPMP2/macos")
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/externals/sdk/Libraries/Mac")
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/externals/fmod/mac")
elseif (UNIX)
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/externals/XPMP2/lin")
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/externals/fmod/lin")
elseif (WIN32)
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/externals/XPMP2/win")
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/externals/sdk/Libraries/Win")
	list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/externals/fmod/win")
endif()

# Link the XPMP2 library
find_library(XPMP2_LIBRARY XPMP2 REQUIRED)
message ("XPMP2_LIBRARY = ${XPMP2_LIBRARY}")
target_link_libraries(${PROJECT_NAME} ${XPMP2_LIBRARY})

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

target_link_libraries(
  ${PROJECT_NAME}
	CONAN_PKG::nlohmann_json
	CONAN_PKG::zeromq
	CONAN_PKG::cppzmq
	${XPMP2_LIBRARY})

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
include_directories( ${OpenGL_INCLUDE_DIRS} )
target_link_libraries( ${PROJECT_NAME} ${OpenGL_LIBRARIES} )

# Enable all X-Plane SDK APIs up to the newest version.
add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1 -DXPLM303=1)

# Define platform macros.
add_definitions(-DAPL=$<BOOL:${APPLE}> -DIBM=$<BOOL:${WIN32}> -DLIN=$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${APPLE}>>>)

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