# Sample toolchain file for building for Windows from an Ubuntu Linux system.
#
# Expected environment variables and example values:
# MINGW_TOOLCHAIN_PREFIX = "x86_64-w64-mingw32"
# OSX_SDK_PATH = "/usr/x86_64-w64-mingw32"

set(CMAKE_SYSTEM_NAME                       Windows)
set(CMAKE_INSTALL_PREFIX                    $ENV{MINGW_PATH})
set(CMAKE_FIND_ROOT_PATH                    $ENV{MINGW_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM       NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY       BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE       ONLY)
set(CMAKE_C_COMPILER                        $ENV{MINGW_TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER                      $ENV{MINGW_TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER                       $ENV{MINGW_TOOLCHAIN_PREFIX}-windres)
