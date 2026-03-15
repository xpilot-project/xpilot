How to Use XPMP2
==

There is a separate Template Github repository named
[XPMP2-Sample](https://github.com/TwinFan/XPMP2-Sample),
which demonsrates using the XPMP2 library,
and which may serve you as a basis for your own plugin.

## Sample Plugin XPMP2-Sample ##

[XPMP2-Sample](https://github.com/TwinFan/XPMP2-Sample) is a complete
plugin including build projects and CMake setup. It displays 3 aircraft flying circles
in front of the user's plane. Each of the 3 aircraft is using a different technology:
the now recommended way of subclassing `XPMP2::Aircraft`, the legacy way
of subclassing `XPCAircraft` (as used by LiveTraffic v1.x), and by calling
standard C functions.

Instructions how to check out and build XPMP2-Sample are found
in [Building XPMP2 and the Sample Plugin](Building.html).

For the sample plugin to work you need to follow instruction in
[Deploying XPMP2-based Plugins](Deploying.html) for

- a `Resources` folder under the plugin's folder holding the 3 files from
  the `Resources` folder provided here with XPMP2,
- CSL models installed in folders under that `Resources` folder.
  - The sample plugin tries to find matches for "B06/TXB", "DH8A/BER", and
    "A321", all available in the
    [Bluebell](https://forums.x-plane.org/index.php?/files/file/37041-bluebell-obj8-csl-packages/)
    packages. But matching will find _anything_ if you provide at least one model.

Its source code `XPMP2-Sample.cpp` includes a lot of comments explaining
what is happening. Read them!

Expected folder structure of the installation:
```
<X-Plane>/Resources/plugins/XPMP2-Sample/
  lin_x64/XPMP2-Sample.xpl
  mac_x64/XPMP2-Sample.xpl
  Resources/
      CSL/                 <-- install models here
      Doc8643.txt
      MapIcons.png
      Obj8DataRefs.txt
      related.txt
      relOp.txt
  win_x64/XPMP2-Sample.xpl
```

## Binaries and Header Files ##

You don't need to build the library yourself if you don't want.
Archives with headers and release/debug builds are available in the
[Release section](https://github.com/TwinFan/XPMP2/releases)
here on GitHub.

## Anatomy of Your Plugin using XPMP2 ##

Generically you should probably use XPMP2 as follows:

- `XPluginEnable`: Initialize XPMP2 using
  - `XPMPMultiplayerInit`,
  - `XPMPLoadCSLPackage` once or multiple times, and
  - `XPMPMultiplayerEnable`.
- During runtime, e.g. in flight loop callbacks,
  - Create new aircraft by creating new objects of _your_ aircraft class,
    which derives from [`XPMP2::Aircraft`](html/classXPMP2_1_1Aircraft.html)
  - Remove aircraft by deleting that object
- `XPluginDisable`:
  - Remove all your remaining aircraft, then call
  - `XPMPMultiplayerDisable` and
  - `XPMPMultiplayerCleanup`

## Subclass `XPMP2::Aircraft` ##

New plugin implementations are strongly advised to directly sub-class
from the new class `XPMP2::Aircraft`, which is the actual aircraft representation.

See the `SampleAircraft` class defined in `XPMP2-Sample/XPMP2-Sample.cpp`
for an example implementation.

In your class, override the abstract function `UpdatePosition()` and
- provide current position and orientation
  - via a call to `SetLocation()`, or
  - by writing directly into `drawInfo`;
- provide plane configuration details
  - calling the many `Get`/`Set` member functions as needed, or
  - writing directly into the `v` array
    using the elements of `enum DR_VALS` as indexes.

This way minimises the number of copy operations needed. `drawInfo` and the `v` array
are _directly_ passed on to the `XPLMInstanceSetPosition` call.

Other values like `label`, `aiPrio`, or `acInfoTexts` can also be updated by your
`UpdatePosition()` implementation and are used when drawing labels
or providing information externally like via AI/multiplayer dataRefs.
