Copying `.obj` Files for Replacing Animation dataRefs and Textures
==

CSL packages come in different flavours. Popular ones for general use are
the Bluebell and the X-CSL packages. Both come with different history.
For XPMP2 to use all their features it needs to change their `.obj` files.
Performing these changes is built into XPMP2.

- [The Bluebells](https://forums.x-plane.org/index.php?/files/file/37041-bluebell-obj8-csl-packages/)
  have not migrated newer animation dataRefs from their World Traffic
  origins to the [now available `libxplanemp` names](CSLdataRefs.html),
  which means that animations already existing in the models (like props, rotors,
  wheels, reversers) cannot be controlled by XPMP2.
- [X-CSL](https://csl.x-air.ru/?lang_id=43) avoids shipping one `.obj`
  per texture and instead uses an extended `OBJ8` command syntax in the
  `xsb_aircraft.txt` file to name the texture to use. Any kind of "dynamic
  change of the texture to use" cannot be supported with instancing any longer.
  Instancing requires one `.obj` file to be readily available for loading.

Previously, the [`CSL2XSB.py` script](https://github.com/TwinFan/CSL2XSB)
performed the necessary changes. But executing it was an additional step
too complex for some users, and required the user to have Python 3 installed.

Now, XPMP2 includes the required operation as well, which will be applied
to `.obj` files shortly before they are loaded as an instances, ie. only for
models which are actually used.

Two kinds of operations happen and a plugin using XPMP2 can control via
configuration if they shall be applied. See the `XPMPIntPrefsFuncTy`
type definition in [XPMPMultiplayer.h](html/XPMPMultiplayer_8h.html) or
the `XPMP2-Sample.cpp` code for `CBIntPrefsFunc()`.
The following configuration values play a role:

section | key                 | type | default | description
------- | ------------------- | ---- | ------- | -------------------------------------------------------------------------
models  | replace_datarefs    | int  |    0    | Replace dataRefs in OBJ8 files upon load, creating new OBJ8 files for XPMP2 (**defaults to OFF!**)
models  | replace_texture     | int  |    1    | Replace textures in OBJ8 files upon load if needed (specified on the OBJ8 line in xsb_aircraft.txt), creating new OBJ8 files

General Notes on Copied Files
--

If copying takes place, then a new `.obj` file is written into the same
folder next to the original one. The file name of the copy is as follows:
- If the file is created for a specific texture, then the texture's file name
  (without extension) is added to the file name before the extension.
- The extension will always be `.xpmp2.obj` instead of just `.obj`.

**For example**, X-CSL defines the Lufthansa A320 as follows in `A320/xsb_aircraft.txt`:
```
OBJ8_AIRCRAFT A320:DLH
OBJ8 GLASS YES A320:A320fCFMfan.obj
OBJ8 SOLID YES A320:A320fCFM.obj DLH.png A320fCFM_LIT.png
VERT_OFFSET 0.0
AIRLINE A320 DLH
```

The copying operation will create the following two new files:
- `A320fCFMfan.xpmp2.obj` as a copy for the fan model.<br>
  This file has no additional texture specifications, so all livery variations
  share just one copy. If the copy exists already it will certainly not be copied again.
- `A320fCFM.DLH.xpmp2.obj` as a copy for the fuselage, referencing the
  `DLH.png` and `A320fCFM_LIT.png` textures.

Each copied file includes a comment in line 4 stating its origin:
```
# Created by <PluginLogAcronym>/XPMP2 based on Resources/plugins/LiveTraffic/Resources/X-CSL/A320/A320fCFMfan.obj
```

Replacing dataRefs
--

This is **off by default**, because well curated packages won't need it.
To enable it return 1 when called for configuration item `XPMP_CFG_ITM_REPLDATAREFS`.

When enabled, a copy for each `.obj` file will be created just before it is
to be loaded. In each copy, dataRefs are replaced based on the definition in
`<resourceDir>/Obj8DataRefs.txt`. That file just lists dataRef names to be
replaced with the replacement dataRef like this:
```
cjs/world_traffic/engine_rotation_angle1        libxplanemp/engines/engine_rotation_angle_deg1
cjs/world_traffic/main_gear_deflection          libxplanemp/gear/tire_vertical_deflection_mtr
```
The `Obj8DataRefs.txt` file available with XPMP2 defines a number of replacements
to unlock more features in the Bluebell CSL models. The simple text format
allows you to add/change replacement definitions easily and ship your
version with your plugin.

Replacing Texture
--

This is **on by default**. You could switch it off by returning `0` when asked
for configuration item `XPMP_CFG_ITM_REPLTEXTURE`.

A copy will only be initiated if additional textures are defined as 4th
(and optionally 5th) parameter with the `OBJ8` command in `xsb_aircraft.txt`.

The copy will then state those defined textures in the `TEXTURE` resp.
`TEXTURE_LIT` commands of the `.obj` file.

For the Lufthansa A320 example above the `TEXTURE` lines will look like this:
```
TEXTURE DLH.png
TEXTURE_LIT A320fCFM_LIT.png
```
