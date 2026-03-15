# Toolchain for Visual Studio builds via command line/NMAKE

set(CMAKE_EXE_LINKER_FLAGS                  "/machine:x64 /INCREMENTAL:NO /LTCG"        CACHE STRING        "Linker Flags"                  FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO   "/machine:x64 /INCREMENTAL:NO /LTCG /DEBUG" CACHE STRING        "Linker Flags"                  FORCE)
