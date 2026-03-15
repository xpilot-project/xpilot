# Building XPMP2 and the Sample Plugin

The [XPMP2 Library](https://github.com/TwinFan/XPMP2) and
the [XPMP2-Sample plugin]/https://github.com/TwinFan/XPMP2-Sample)
are now two separate GitHub repositories.

It is recommended that you include `XPMP2` as a submodule in your project.

The `XPMP2-Sample` repo is configured as a _Public Template_,
so you can immediately [base your own project on it](https://docs.github.com/en/repositories/creating-and-managing-repositories/creating-a-repository-from-a-template).
Please note that `XPMP2-Sample` includes `XPMP2` as a submodule,
so you need to checkout those, too, e.g. on the command line by doing
```bash
git clone --recurse-submodules https://github.com/TwinFan/XPMP2-Sample
```

## Build options

Both repositories share many similarities when it comes to building.
There are four options to build from sources:

Options            | Windows            | MacOS (universal)   | Linux
-------------------|--------------------|---------------------|-------------------
**CMake**          | VS 2022 / `NMAKE`  | XCode 12 / `ninja`  | Focal and Bionic / `ninja`
**Github Actions** | Visual Studio 2022 | XCode 12            | Focal
**IDE**            | Visual Studio 2022 | XCode 12            | -
**Docker**         | Mingw64            | clang, SDK 12       | Focal and Bionic

## Including XPMP2 directly in GitHub project and CMake builds

You can avoid separate builds and instead include `XPMP2` directly into your project.
Recommended steps are:

1. Include XPMP2 as a Github Submodule into your Github project, using something like
    ```bash
    git submodule add --name XPMP2 'https://github.com/TwinFan/XPMP2' lib/XPMP2
    ```
    To update your local version with changes from the XPMP2 repository run something like
    ```bash
    git submodule update --remote
    ```
2. In your `CMakeLists.txt` file include XPMP2 using something like the following in appropriate places:
    1. If needing FMOD sound support first define
        ```cmake
        set (INCLUDE_FMOD_SOUND 1)              
        add_compile_definitions(INCLUDE_FMOD_SOUND=1)
        include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/XPMP2/lib/fmod/logo")
        target_sources(${CMAKE_PROJECT_NAME} lib/XPMP2/lib/fmod/logo/FMOD_Logo.cpp)
        ```
    2. Including building XPMP2 as follows:
        ```cmake
        include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/XPMP2/inc")
        add_subdirectory(lib/XPMP2)
        add_dependencies(${CMAKE_PROJECT_NAME} XPMP2)
        target_link_libraries(${CMAKE_PROJECT_NAME} XPMP2)
        ```

## Using CMake

### MacOS, Linux

Given a proper local setup with a suitable compiler, CMake, and Ninja installed,
you can just locally build the sources from the `CMakeList.txt` file,
e.g. like this:

```bash
mkdir build
cd build
cmake -G Ninja ..
ninja
```

This is precicely how the Mac and Linux builds are done in Github Actions.

### Windows

For Windows, there is a build script available that encapsulates the
Visual Studio environment setup and then runs `CMAKE` and `NMAKE`.

```bash
.github\actions\build-win\build-win.cmd "C:\Program Files\Microsoft Visual Studio\2022\Community" build-win RelWithDebInfo
```

Run `build-win.cmd` without parameters to learn about options.
To learn what the path to your Visual Studio Build Tools folder is
open an "x64 Native Tools COmmand Prompt for VS...", an option
you'll find in the "Visual Studio..." folder of your Windows Start menu.
The folder the prompt opens in is the one to pass to `build-win.cmd`.

#### DLL

The Windows version can also be built as a DLL.

> **WARNING**
>
> Shipping XPMP2 as a DLL is generally not recommended.
> Over time, different versions of the `XPMP2.#.#.#.dll` could get
> into conflict with each other.
>
> To reduce the risk of version conflict, the DLL is generated with a version number
> in its name. Still, version conflicts could only be avoided if only
> the DLLs provided here in the
> [Github Releases](https://github.com/TwinFan/XPMP2/releases) are distributed.
> Even just recompiling the DLL and import library with a slightly different
> compiler could cause a different signature and a version conflict.
>
> The DLL version has not been extensively tested and should not be used
> for shipping public versions of a plugin without such extensive tests.
> 
> All public functions, classes, and structure are then defined as
> `__declspec(dllexport)` and the warnings
> [C4251](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4251?view=msvc-170)
> and [C4275](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170) are suppressed.
> The cases listed in the above warning documentation may not apply to XPMP2
> and it may just work, but I have not done intensive analysis on the matter.
> I have, however, briefly ran LiveTraffic with a DLL version just to prove
> it all basically works.

To build the DLL version of XPMP2, pass one more parameter
`-D"XPMP2_BUILD_SHARED_LIBS:boolean=TRUE"` to `build-win.cmd`
so that it looks like

```shell
.github\actions\build-win\build-win.cmd "C:\Program Files\Microsoft Visual Studio\2022\Community" build-win RelWithDebInfo -D"XPMP2_BUILD_SHARED_LIBS:boolean=TRUE"
```

You'll find the actual `XPMP2-#.#.#.dll`, the import library `XPMP2-#.#.#.lib`,
and the debug information `XPMP2-#.#.#.pdb` all in the provided build directory,
`build-win` in the above example.

To use the DLL version from within your plugin

1. Define the compiler macro `XPMP2_DLLIMPORT` before including any
   XPMP2 header so that all the public XPMP2 functions, classes, and structures
   are defined with `__declspec(dllimport)`,
2. link to `XPMP2-#.#.#.lib`, which serves as the DLL import library,
3. ship `XPMP2-#.#.#.dll` alongside your plugin in the same directory.

If you import the XPMP2 project as part of your CMake build process,
as explained above in "Including XPMP2 directly...", then define something
like this before the `add_subdirectory` call to enable building and linking
the DLL version with your plugin:

```cmake
# Using XPMP2 as DLL
if (WIN32)
    # Tell XPMP2 to build the DLL
    set(XPMP2_BUILD_SHARED_LIBS 1)
    # Tell my project to import functions from DLL
    target_compile_definitions(${CMAKE_PROJECT_NAME}
        PRIVATE XPMP2_DLLIMPORT
    )
endif()
```

The resulting `XPMP2.#.#.#.dll` will be found in the `XPMP2` build folder,
not in your plugin's build output.

## Using an IDE

### Windows

- Install [Visual Studio](https://visualstudio.microsoft.com/vs/community/)
- Open the root folder of the repo using "File > Open > Folder",
  [see here for VS's "Open Folder" functionality](https://docs.microsoft.com/en-us/cpp/build/open-folder-projects-cpp?view=vs-2019))
- VS use the CMake setup to configure building the binaries.
- Build from within Visual Studio.

Results are in `build-win`.

### MacOS

My development environment is Mac OS, so expect the XCode environment to be
maintained best. Open `XPMP2.xcodeproj` resp. `XPMP2-Sample.xcodeproj` in XCode.

In the `XPMP2-Sample` XCode Build Settings you may want to change the definition
of the User-Defined build macro `XPLANE11_ROOT`: Put in the path to your
X-Plane 11 installation's root path, then the XPMP2 binaries will be
installed right there in the appropriate `Resources/plugins` sub-folder
structure.

## Using Gitub Actions

The GitHub workflow `.github/workflows/build.yml` builds the plugin in GitHubs CD/CI environment.
`build.yml` calls a number of custom composite actions available in `.github/actions`,
each coming with its own `README.md`.

The workflow builds Linux, MacOS, and Windows plugin binaries and provides them as artifacts,
so you can download the result from the _Actions_ tab on GitHub.

### Apple: Sign and Notiarize

The Apple build of the `XPMP2-Sample` plugin can be signed and notarized,
provided that the following _Repository Secrets_ are defined in the repository's settings
(Settings > Secrets > Actions):
- `MACOS_CERTIFICATE`: Base64-encoded .p12 certificate as
  [explained here](https://localazy.com/blog/how-to-automatically-sign-macos-apps-using-github-actions#lets-get-started)
- `MACOS_CERT_PWD`: The password for the above .p12 certificate file export
- `NOTARIZATION_USERNAME`: Apple ID for notarization service (parameter `--apple-id` to `notarytool`)
- `NOTARIZATION_TEAM`: Team ID for notarization service (parameter `--team-id` to `notarytool`)
- `NOTARIZATION_PASSWORD`: [App-specific password](https://support.apple.com/en-gb/HT204397) for notarization service (parameter `--password` to `notarytool`)

## Using Docker

> **NOTE:** The Docker environments are a bit outdated
> and no longer actively maintained. Use caution.

Docker environments based on Ubuntu 20.04 and 18.04 are used,
which can build all 3 platforms, Linux even in two flavors.

- Install [Docker Desktop](https://www.docker.com/products/docker-desktop) and start it.
- `cd` to the project's `docker` folder, and enter `make` for all
  XPMP2 library targets on all platforms.

The following `make` targets are available:

- `lin` builds Linux on Ubuntu 20.04
- `lin-bionic` builds Linux on Ubuntu 18.04
- `mac-arm` builds MacOS for Apple Silicon using `clang` as cross compiler
- `mac-x86` builds MacOS for x86 using `clang` as cross compiler
- `mac` builds `mac-arm` and `mac-x86` and then combines the results into a univeral binary
- `win` builds Windows using a Mingw64 cross compiler setup
- `bash_focal` starts a bash prompt in the Ubuntu 20.04 docker container
- `bash_bionic` starts a bash prompt in the Ubuntu 18.04 docker container
- `doc` builds the documentation based on Doxygen, will probably only work on a Mac with Doxygen provided in `Applications/Doxygen.app`
- `clean` removes all `build-<platform>` folders

Find results in the respective `build-<platform>` folder, the `XPMP2` library right there,
the Sample plugins in their proper `<platform>_x64` subfolder.

For more details and background information on the provided Docker environments
see the `docker/README.md`.

