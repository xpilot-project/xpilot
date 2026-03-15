Backwards Compatibilty
==

Despite its new approach, XPMP2 shall be your drop-in replacement for the
original library: The original header files are still provided with the same name.
All original public functions are still there. The original
[`XPCAircraft` class](html/classXPCAircraft.html) is still there,
now derived from [`XPMP2::Aircraft`](html/classXPMP2_1_1Aircraft.html).

Changes
--

A few changes are there, though, for clarity and to be future-proof.
Some few require simple code changes:
- All enumerations are now proper `enum` definitions, i.e.
  `enum typeName {...}` instead of `typedef int typeName`.
- Type `XPMPPlaneID` is now just an `unsigned`, no longer a pointer type,
  so it can be used directly as `modeS_id` in the new
  [TCAS override](https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/)
  approach. Initialisation with `NULL` or `nullptr` will not compile,
  initialise with `0` instead.
- A number of functions and the class `XPCAircraft` are explicitly marked `[[deprecated]]`,
   which will raise a few warnings, if your compiler is configured to show them.
   Just a gentle reminder to update your plugin at some point in time...
- `XPMPMultiplayerInitLegacyData` will in turn call `XPMPMultiplayerInit`, and
   `XPMPLoadCSLPackage`. The correct future-proof way of initialising the library is to call
   `XPMPMultiplayerInit` and then do one or more calls to `XPMPLoadCSLPackage`.
- Parameters of `XPMPMultiplayerInit(LegacyData)` have been reshaped and
  will require a code change. These parameters are needed now
  (see documentation of header file [XPMPMultiplayer.h](html/XPMPMultiplayer_8h.html)):
  - your plugin's name (replaces use of `XPMP_CLIENT_NAME` and
    `XPMP_CLIENT_LONGNAME` precompile macros),
  - the path to the resources directory (see [here for details](Deploying.html)),
  - (optionally) a preferences callback function,
  - (optionally) the default aircraft type code,
  - (optionally) a short name / acronym used for logging into `Log.txt`.
- It is no longer necessary to define the compile-time macros `XPMP_CLIENT_NAME`
  and `XPMP_CLIENT_LONGNAME`. Instead, you can use the new parameter
  `inPluginName` in the call to `XPMPMultiplayerInit` or the function
  `XPMPSetPluginName` to set the plugin's name from within your plugin.
  XPMP2 tries to guess the plugin's name if no name is explicitely set.
  This allows using the provided libraries directly without the need to recompile.
- `XPMPLoadCSLPackage` walks directories hierarchically up to 5 levels until it
   finds an `xsb_aircraft.txt` file. This should not affect classic usages,
   where such a path was just one level away from the `xsb_aircraft.txt` file.
   It would just also search deeper if needed.
- Available configuration parameters have change, that will be queried in calls
  to the configuration function callback that you can provide in
  `XPMPMultiplayerInit`. Please see the [header file documentation](html/XPMPMultiplayer_8h.html)
  for `XPMPIntPrefsFuncTy` for details.

Backward compatibility is tested intensively with LiveTraffic. LiveTraffic has
always used subclassing of `XPCAircraft`, so you can be very sure that similar
implementations will work fine.
There have been less tests with the direct C-style interface
using `XPMPCreatePlane()` et al., mostly using the sample plugin included
in the package.
