`xsb_aircraft.txt` File format
==

The `xsb_aircraft.txt` file describes the content of one CSL model package.
XPMP2 reads the file to learn about available CSL models and which ICAO aircraft
type designators, airlines, and special liveries they match.

The [original file format](https://github.com/kuroneko/libxplanemp/wiki/LegacyCSL#aircraft-directory-text-file-format)
supported more commands necessary to support older formats like `.acf` or
OBJ7. XPMP2 only supported OBJ8 models and processes only the commands
listed here. Others are ignored. They may raise warnings in `Log.txt` but
otherwise do no harm.

Example
--

The following is taken from the beginning of Bluebell's `xsb_aircraft.txt`
file in its `BB_Airbus` folder:

```
EXPORT_NAME __Bluebell_Airbus

OBJ8_AIRCRAFT A306_AAW
OBJ8 SOLID YES __Bluebell_Airbus/A306/A306_AAW.obj
VERT_OFFSET 4.6
AIRLINE A306 AAW

OBJ8_AIRCRAFT A306_AHK
OBJ8 SOLID YES __Bluebell_Airbus/A306/A306_AHK.obj
VERT_OFFSET 4.6
AIRLINE A306 AHK
```

Structure
--

A `xsb_aircraft.txt` file starts defining one (or even more) package
name(s) using the `EXPORT_NAME` command. Packages are identified by these
names, which are then used as a root name to define the `.obj` file locations.

Then follow one or typically many aircraft definitions. Each aircraft definition
starts with an `OBJ8_AIRCRAFT` command defining a package-unique id.
XPMP2 expects the _combination_ of `EXPORT_NAME` and `OBJ8_AIRCRAFT` name
to be unique and ignores any duplicates (with a warning written to `Log.txt`).

Each aircraft definition then includes:
- One or more `OBJ8` commands to defines which `.obj` file(s) to load for the
  CSL model.
  The OBJ8 file format is
  [defined by Laminar](https://developer.x-plane.com/article/obj8-file-format-specification/).
- Optionally one `VERT_OFFSET` command to define the vertical offset that
  needs to be applied to make the model sitting right on its wheel when
  placed on solid ground.
  If missing, then XPMP2 will read the `.obj` and find the lowest feature of
  the model and use this as reference.
- One or more matching definitions using any of the commands `ICAO`, `AIRLINE`,
  `LIVERY`, or `MATCHES`. With XPMP2, all four commands are synonyms and
  handle 1 to 3 parameters:
  - ICAO aircraft type designator
  - ICAO airline operator code
  - special livery text

Commands
--

### `EXPORT_NAME`

```
EXPORT_NAME <package_name>
```

Defines a package name for the content of the `xsb_aircraft.txt` file.
By this package name its content can be referenced, both from within
this file but also from other files.

Multiple `EXPORT_NAME`s can be defined in one single `xsb_aircraft.txt` file.
They all then refer to the same content.

Everything after the `EXPORT_NAME` command until the end of the line is considered
the `<package_name>`.

The `<package_name>` is used in the `OBJ8` command as the root to the
necessary `.obj` files.

### `OBJ8_AIRCRAFT`

```
OBJ8_AIRCRAFT <model_id>
```

Starts the definition of a CSL model.

XPMP2 uses the `<package_name>` plus the `<model_id>`
to uniquely refer to a specific CSL model. This combination must be unique.

Everything after the `OBJ8_AIRCRAFT` command until the end of the line is considered
the `<model_id>`.

### `OBJ8`

```
OBJ8 SOLID YES <package_name>[/<relativePathTo>]/<file.obj> [<texture.ext> [<texture_lit.ext>]]
```

Specifies an `.obj` file to load and optionally differing textures to use.

One `OBJ8_AIRCRAFT` can consist of multiple `.obj` files, though this is not recommended.
If multiple `OBJ8` lines are given then _all_ objects will be loaded and
displayed when the CSL model is needed, assuming that their local coordinate
reference is the same, ie. they will all be placed at the same location.
(And only _if_ all `.obj` files are found and loaded will the model be rendered.)

The location is relative to the given `<package_name>` (which can be and often is
in the current `xsb_aircraft.txt` file, but could also be defined in another one):
- If a `<relativePathTo>` is given it needs to start in the same directory
  where the `<package_name>`'s `xsb_aircraft.txt` file is located;
- if no `<relativePathTo>` is given then the `<file.obj>` is expected to be
  in the same directory as the `<package_name>`'s `xsb_aircraft.txt` file.

Historically, other parameters than `SOLID YES` were supported,
but the distinction is no longer needed. In fact, XPMP2 just ignores the
2nd and 3rd parameter altogether.

The 4th and 5th parameters are optional. They define a different texture
(livery) to use than originally specified in `<file.obj>`. To be able to use this
differing texture, XPMP2 creates a copy of `<file.obj>`, namely
`<file>.<texture>.xpmp2.obj`, in which the object's `TEXTURE` resp.
`TEXTURE_LIT` commands refer to `<texture.ext>` resp. `<texture_lit.ext>`.
[See here for details on copying `.obj` files.](CopyingObjFiles.html)

### `VERT_OFFSET`

```
VERT_OFFSET <float_num>
```

Defines the vertical offset for correct placement of the model on the ground
with gears down.

If the `VERT_OFFSET` command is missing, then XPMP2 will read the `.obj` file(s)
searching for the lowest feature defined in it and will use that point as
the reference for the vertical offset.

### `OFFSET`

```
OFFSET <unknown> <unknown> <vertical_offset>
```

The `OFFSET` command appears in PilotEdge packages.
The meaning of the first two parameters remains a mystery.

The 3rd parameter has the same meaning as `VERT_OFFSET`, see above.

### `MATCHES` (`ICAO`, `AIRLINE`, `LIVERY`)

```
MATCHES <acType> [<operator> [<livery]]
```

The 3 commands `ICAO`, `AIRLINE`, `LIVERY` are synonyms for `MATCHES`,
provided for backward compatibility.
In XPMP2, all four commands share the same syntax and semantic.

Defines matching parameters for XPMP2 matching algorithm.

- `<acType>` (mandatory) defines the aircraft type, typically given as
  [ICAO aircraft type designator](https://www.icao.int/publications/DOC8643/Pages/Search.aspx).
  It is not mandatory to be an ICAO designator, but many plugins using XPMP2
  will expect it to be one. Notable exceptions are ground vehicles, which are
  not defined by ICAO. "ZZZC" is an often used `<acType>` code for ground
  vehicles.
- `<operator>` (optional) defines the aircraft's operator, often an airline.
  Different operators will fly different liveries, which in turn are defined
  as textures in the referenced `.obj` file
  (see [its `TEXTURE` command](https://developer.x-plane.com/article/obj8-file-format-specification/#TEXTURE_lttex_file_namegt)). Most plugins will expect this `<operator>`
  code to be one of the
  [ICAO-defined operator codes](https://en.wikipedia.org/wiki/List_of_airline_codes).

  You can use a single dash `-` if you don't want to define an operator,
  but need to place a value to be able to define a livery with the 3rd parameter:
- `<livery>` (optional, can only be defined together with `<operator>`)
  distinguishes different liveries of one operator, e.g. special anniversary
  or event liveries.
  There is no univerally defined way what this code shall look like,
  so it is up to the plugin to define what matches.
  For example, LiveTraffic feeds the registration (tail number) to match
  against the `<livery>` code. That makes it possible to list the
  specific aircraft that are using some special livery in several
  `MATCHES` lines.

Several `MATCHES` lines can be defined per model. The model will then match
for all the provided codes.

`<acType>` must be the same in all `MATCHES` lines
of one aircraft definition. Differing `<acType>` values will raise a warning
but will otherwise be just ignored.
