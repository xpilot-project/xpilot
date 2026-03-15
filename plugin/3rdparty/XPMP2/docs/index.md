# XPMP2 - Developer's documentation

These pages, available as [GitHub pages](https://twinfan.github.io/XPMP2/),
document both usage and internals of the XPMP2 library.

Content and availability status:

- [X] [Requirements](#requirements)
- [ ] Feature Details
  - [X] [Model Matching](Matching.html)
  - [X] [TCAS Target and AI/multiplayer support](TCAS.html)
  - [ ] Additional map layer
  - [X] [Wake Turbulence Support](Wake.html)
  - [X] [Sound Support](Sound.html), by X-Plane 12 or via FMOD sound library
  - [X] [Contrails](Contrails.html)
  - [X] [Shared dataRefs](SharedDataRefs.html) providing textual aircraft / flight information to interested 3rd party plugins
  - [X] [XPMP2 Remote](Remote.html) functionality
- [ ] Coding, Building, Deployment
  - [X] [Building](Building.html) XPMP2, and the Sample plugin
  - [X] Brief ["HowTo" guide](HowTo.html)
  - [X] Doxgen-generated [code documentation](html/index.html)
  - [ ] Provide some general "how it works" background, can base on kuronekos wiki (but shorter) and on some of [my additions](https://github.com/TwinFan/libxplanemp/wiki#changes-to-multiplayeraitcas-handling)
  - [X] [What to ship](Deploying.html) so that it works
- [X] CSL Packages
  - [X] [File format definition](XSBAircraftFormat.html) for `xsb_aircraft.txt`
  - [X] [CSL model dataRefs supported by XPMP2](CSLdataRefs.html)
  - [X] [Copying `.obj` files on load for replacing animation dataRefs and textures](CopyingObjFiles.html)

## Requirements

- XPMP2 implements [instancing](https://developer.x-plane.com/sdk/XPLMInstance/),
  so it **requires X-Plane 11.10** or later
- CSL models in **OBJ8 format** (ie. older OBJ7 models are no longer supported)
- Potentially an FMOD license if built with FMOD sound support, see
  [Sound Support](Sound.html)

## Coding, Building, Deployment

These aspects are relevant for developers using XPMP2 in their own plugin:

### Building XPMP2

XPMP2 can be included into your projects as GitHub submodule, and into a CMake build plan via `add_subdirectory`.
XCode projects, Visual Studio solutions, and a Docker environment for
Linux and Mac OS builds are provided. The details are
[documented here](Building.html).

### How to Use XPMP2

This part will probably need more attention, but find some [first
information here](HowTo.html) and study the working sample plugin
in the `XPMP2-Sample` folder.

### API and Code Documentation

All header (and code) files are documented using
[Doxygen](http://www.doxygen.nl/)-style comments.
The generated doxygen files are checked in, too, so that the are available
online:

- [Main Page](html/index.html)
- [XPMPMultiplayer.h](html/XPMPMultiplayer_8h.html) -
  Initialisation and general control functions
- [XPMPAircraft.h](html/XPMPAircraft_8h.html) -
  Defines the main class
  [XPMP2::Aircraft](html/classXPMP2_1_1Aircraft.html),
  which represents an aircraft. **Subclass this class in your plugin!**

### Backward Compatibility

If you are familiar with the original `libxplanemp` and are using it already
in your plugins, then you will find
[these information on backward compatibility](BackwardsCompatibility.md)
useful, which explain how you can replace `libxplanemp` with XPMP2
with limited effort.

### Deploying Your Plugin

Apart from the binaries that you build you will need to ship the files provided
here in the 'Resources' folder. Also, users will need to install CSL models.
Find [more details here](Deploying.html).

## CSL Models and Packages

These aspects are relevant for CSL model developers and package providers:

### `xsb_aircraft.txt` File Format

The `xsb_aircraft.txt` file defines the content of one CSL model packages.
It´s format is [define here](XSBAircraftFormat.html).

### dataRefs available to CSL Models

To drive CSL models'
[ANIMations](https://developer.x-plane.com/article/obj8-file-format-specification/#ANIMATION_COMMANDS),
a long list of dataRefs is provided by XPMP2.
See its [documentation here](CSLdataRefs.html).

### Copying `.obj` files on load

CSL packages come in different flavours. Popular ones for general use are
the Bluebell and the X-CSL packages. Both come with different history.
For XPMP2 to use all their features it needs to change their `.obj` files.
Performing these changes is built into XPMP2.
[See here for details.](CopyingObjFiles.html)

## Links to outside locations

### TCAS Override

The [TCAS Override approach](https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/)
explains how TCAS information is provided, the classic multiplayer dataRefs are maintained
and how 3rd party plugins can access this information. XPMP2 publishes data
via the `sim/cockpit2/tcas/targets` dataRefs.

### Wake Turbulence

X-Plane's approach to Wake Turbulence support for TCAS targets is described in
[this blog post](https://developer.x-plane.com/2022/02/wake-turbulence/)
featuring LiveTraffic screenshots and
[this article on the dataRef details](https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/).

### Original libxplanemp

- [TwinFan's libxplanemp fork](https://github.com/TwinFan/libxplanemp) on GitHub
  - [wiki explaining differences to the kuroneko fork](https://github.com/TwinFan/libxplanemp/wiki)
- [kuroneko's fork](https://github.com/kuroneko/libxplanemp) on GitHub, a long-time standard and basis of other important forks
  - [kuroneko's wiki](https://github.com/kuroneko/libxplanemp/wiki) including notes on CSL development
