# Shared dataRefs providing Textual Aircraft / Flight Information

Textual aircraft and flight information is provided via
[shared dataRefs](https://developer.x-plane.com/sdk/XPLMDataAccess/#XPLMShareData)
of type `xplmType_Data` as suggested by FSTramp.
This information is offered to 3rd party plugins,
which want to show (information of) AI planes like camera or map plugins.

[DRE](https://developer.x-plane.com/tools/datarefeditor/) and
[DRT](https://forums.x-plane.org/index.php?/forums/topic/82960-datareftool-is-an-improved-datarefeditor-open-source-better-search-change-detection/)
are informed about these dataRef names as soon as the first aircraft
is created. So you can use either to have a peek.

## Information offered by XPMP2-based Plugin

The names of the shared dataRefs follow the naming conventions known
from pre-XP11.50 times when there were just 19 multiplayer planes
support with similarly named dataRefs for position and configuration information.
For each plane there is a set of dataRefs, each dataRef contains
up to 40 byte data to be interpreted as a zero-terminated C string.

XPMP2 creates and maintains up to 63 sets of these dataRefs, starting at
`sim/multiplayer/position/plane1_tailnum` and ending at
`sim/multiplayer/position/plane63_apt_to`.

If you want to pubish information in your XPMP2-based plugin via these channels,
then fill the appropriate field in the `XPMP2::Aircraft::acInfoTexts` structure.

`sim/multiplayer/position/...`     | `acInfoTexts.` | Meaning
-----------------------------------|----------------|--------------------------------------
`plane#_tailnum`                   | `tailNum`      | Tail number, registration
`plane#_ICAO`                      | `icaoAcType`   | ICAO Aircraft type
`plane#_manufacturer`              | `manufacturer` | Aircraft manufacturer, human readable
`plane#_model`                     | `model`        | Aircraft model, human readable
`plane#_ICAOairline`               | `icaoAirline`  | ICAO airline/operator code
`plane#_airline`                   | `airline`      | Airline/operator
`plane#_flightnum`                 | `flightNum`    | Flight number
`plane#_apt_from`                  | `aptFrom`      | Origin airport
`plane#_apt_to`                    | `aptTo`        | Destination airport
`plane#_cslModel`                  | n/a            | CSL Model name *)

*) `_cslModel` differs from the others in that it is _not_ related to live flight
information but to the way the plane is rendered by the XPMP2-based plugin.
It returns the CSL model used as returned by `XPMP2::Aircraft::GetModelName()`,
that is: last folder name plus model id.

## Usage by 3rd-Party Plugins

A plugin that wants to use this data

- must _once_ fetch references to all the up to `63 * 9 = 567` datarefs by
  - calling
    [`XPLMShareData`](https://developer.x-plane.com/sdk/XPLMDataAccess/#XPLMShareData),
  - _optionally_ thereby registering a notification callback, and
  - calling
    [`XPLMFindDataRef`](https://developer.x-plane.com/sdk/XPLMDataAccess/#XPLMFindDataRef);
- can then query current information via
  [`XPLMGetDatab`](https://developer.x-plane.com/sdk/XPLMDataAccess/#XPLMGetDatab) as usual.
  Provide at least a 40 character buffer to be on the safe side.

## Additional links

- [Download page for "FSTrampFree"](https://forums.x-plane.org/index.php?/search/&q=FSTrampFree&type=downloads_file&search_and_or=and&search_in=titles&sortby=relevancy)
  where the above shared dataRefs were originally suggested for implementation
- [LiveTraffic issue #144](https://github.com/TwinFan/LiveTraffic/issues/144),
  with which the functionality was originally added into the previous
  TwinFan branch of "libxplanemp"
