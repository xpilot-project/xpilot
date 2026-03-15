Wake Turbulence Support
==

X-Plane 12 started to
[support wake turbulence](https://developer.x-plane.com/2022/02/wake-turbulence/)
also for TCAS targets.
While X-Plane provides a fixed default (of a comparibly small jet) in case
no additional information is available, its true capacbilities only come
to light when fed with details about the actual plane's dimensions and mass.

Keep in mind that XPMP2 does _not_ build upon AI aircraft, so from X-Plane's
point of view XPMP2's planes are just some random instanced objects. X-Plane does
not actually place wakes behind instanced objects! Instead, X-Plane refers
to the [TCAS target information](https://developer.x-plane.com/article/overriding-tcas-and-providing-traffic-information/)
to calculate wakes. [Some dataRefs](https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/)
have been added there for additional information to calculate matching wakes,
specifically:
- Wing span and area
- Mass
- Lift
- Angle of Attack (AoA)

Defaults
--

As a trade-off between complexity (knowing exact
wing dimensions and weight/lift of any plane) and results (strength of the wake)
XPMP2 applies defaults to the aircraft dimensions based on the
Wake Turbulence Category (WTC) listed in the `Doc8643` data,
which is looked up based on the `icaoType`.
If the `icaoType` isn't found then a default WTC of `M` is assumed.

So even if you don't provide any overrides in your plugin implementation,
there's are reasonable factors in place to provide about matching wakes:

WTC | Span [m]| Area [m^2] | Weight [kg] | Reference
----|---------|------------|-------------|----------------------------
L   | 11.00   |   16.2     |    1,038    | [C172](http://www.flugzeuginfo.net/acdata_php/acdata_cessna172_en.php)
L/M | 17.65   |   28.8     |    6,301    | [B350](http://www.flugzeuginfo.net/acdata_php/acdata_beech350_en.php)
M   | 34.09   |  122.6     |   74,500    | [A320](http://www.flugzeuginfo.net/acdata_php/acdata_a320_en.php)
H   | 64.40   |  541.2     |  367,129    | [B744](http://www.flugzeuginfo.net/acdata_php/acdata_7474_en.php)
J   | 79.80   |  845.0     |  510,560    | [A388](http://www.flugzeuginfo.net/acdata_php/acdata_a380_en.php)

with `Weight = Empty Weight + 80% * (MTOW - Empty Weight)`

Default **Lift** is Weight multiplied by `9.81`, ie. standard gravity.
No g load is taken into account.

Default **AoA** is the plane's pitch, which should be fairly accurate.

Overriding Values in Your Plugin
--

A plugin using XPMP2 can opt to provide its own values for even more
precice results. Search `XPMP2Aircraft.h` for "Wake support" to find the group of functions to support wake tubulence definition.

`WakeApplyDefaults()` sets wing span, area, and weight as per above defaults.

The various `Get/Set` functions get/set wing span, area, weight individually.

As angle of attack and lift may change very dynamically they are implemented
as `virtual` functions that you can override in your own aircraft class:

- `virtual float XPMP2::Aircraft::GetAoA() const`
- `virtual float GetLift() const`

There's one specific case a plugin may want to consider, which is the
transition between ground and airborne operation:
There is no wake behind a taxiing wake because there is no lift produced.
Wake gradually appears during the take-off roll and
fades during rollout after landing.

If you want to simulate this you should override `XPMP2::Aircraft::GetLift()`.
See LiveTraffic's `LTAircraft::GetLift()` for an example in its
[`LTAircraft.cpp` file](https://github.com/TwinFan/LiveTraffic/blob/master/Src/LTAircraft.cpp),
which take information as input that XPMP2 doesn't have but only the
plugin that controls the aircraft in motion.