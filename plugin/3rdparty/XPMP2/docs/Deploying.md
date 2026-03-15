Deploying XPMP2-based Plugins
==

FMOD Sound Support (XP11/Windows)
--
If you built XPMP2 and your plugin with [FMOD Sound Support](Sound.html),
and you want to allow running your plugin with X-Plane 11 under Windows,
then you should ship the `fmod.dll`, too, as
```
win_x64/fmod.dll
```
next to your plugin's Windows version. Find `api/core/lib/x64/fmod.dll` in the Windows version
of the FMOD Core API that you had [downloaded from FMOD](https://www.fmod.com/download#fmodengine).

The reason is that X-Plane updated the FMOD versions included in X-Plane between XP11 and XP12.
While XPMP2 is built in a way that it can run with either version,
XP11 ships a DLL by the name of `fmod64.dll` while XPMP2 expects it by the name `fmod.dll` as XP12 provides it.

Resources
--

XPMP2 needs a couple additional files to work. In your calls to
`XPMPMultiplayerInit` and `XPMPLoadCSLPackage` you provide the locations
to the folders holding these files.

You should ship all the files provided in the `Resources` folder:

- `Doc8643.txt` is a list of ICAO aircraft type designators taken from
  the ICAO web site. This list is required for matching rules related
  to the aircraft type, ie. if no direct match is found in the available
  models or via the `related.txt` lists. ICAO provides monthly updates,
  so this file will update here from time to time, too.
- `MapIcons.png` contains the aircraft icons displayed in the additional
  map layer. Without this file that map layer cannot be created.
- `related.txt` defines "similar looking" aircraft types, so that an
  A320 model could be used if no exact match for the A319 at hand is found.
  Without that file this "related" matching cannot take place.
- `relOp.txt` similarly defines "similar looking" airlines,
  often subsidaries of mother airlines, which share the same livery.
- `Obj8DataRefs.txt` is only required for the
  ["Copying .OBJ Files" functionality](CopyingObjFiles.html):
  It defines dataRef replacements.
  You don't need to ship this file if you do not enable that functionality
  in your plugin.
- `Contrail` folder with 3 files, `Contrail.obj/.png/.pss`;
  this is needed for drawing contrails. If missing, contrails aren't
  available. The folder is not needed if you
  [disable contrails](Contrails.html#disable).

These files have all to be installed in the same folder.
It is good practice to install these files in a folder named `Resources` in
the plugin's folder. Your plugin provides XPMP2 with the folder location
in the `resourceDir` parameter of the `XPMPMultiplayerInit` call
(see [XPMPMultiplayer.h](html/XPMPMultiplayer_8h.html) for more details).

You do not need to and shall not ship the XPMP2 Remote Client with your plugin.
This plugin is maintained and provided centrally. Please refer your users to
- the [download location](https://forums.x-plane.org/index.php?/files/file/67797-xpmp2-remote-client/) and
- the [documentation location](https://twinfan.gitbook.io/livetraffic/setup/installation/xpmp2-remote-client).

CSL Models
--

CSL Models are not shipped with XPMP2. They are available as separate downloads.
Recommended sources are:

- [Bluebell](https://forums.x-plane.org/index.php?/files/file/37041-bluebell-obj8-csl-packages/)
- [X-CSL](https://csl.x-air.ru/?lang_id=43)
- Individual models from X-Plane.org's
[download section "XSB CSL Kits"](https://forums.x-plane.org/index.php?/files/category/12-xsb-csl-kits/)

You may want to refer to
[LiveTraffic's detailed CSL model installation instructions](https://twinfan.gitbook.io/livetraffic/setup/installation/step-by-step#bluebell-csl-package-by-oktalist).

While it is recommendable to have the models installed somewhere under
the plugin's folder, the CSL packages' location is a matter of convention.
Your plugin provides the folder location of CSL packages in one or more
calls to `XPMPLoadCSLPackage`.

Additional `.obj` files next to the existing ones might be generated
during runtime by XPMP2 for replacing dataRefs and textures. A plugin can
control this behaviour via configuration settings.
[See here for details.](CopyingObjFiles.html)
