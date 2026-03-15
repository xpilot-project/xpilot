dataRefs available to CSL Models
==

XPMP2 provides the following dataRef values for each CSL model rendered.
That means, a CSL model can use it for its animations.

Actual values are taken from the `XPMP2::Aircraft::v` array. Compare the indexes
listed in the second comlumn as defined in the `enum DR_VALS`
([`XPMPAircraft.h`](html/XPMPAircraft_8h.html)).

It is recommended to use the various getter/setter member functions in XPMP2::Aircraft
to get/set values. Some of them perform additional functionality like
synchronising RPM and RAD/s values.

Controls
--

dataRef                                     | Getter/Setter             | index into `v`                 | Meaning
------------------------------------------- | ------------------------- | ------------------------------ | ---------------------------------
`libxplanemp/controls/gear_ratio`           | `Get/SetGearRatio`        | `V_CONTROLS_GEAR_RATIO`        | Gear deployment ratio, `0..1`
`libxplanemp/controls/nws_ratio`            | `Get/SetNoseWheelAngle`   | `V_CONTROLS_NWS_RATIO`         | Nose wheel angle, `-45..+45`
`libxplanemp/controls/flap_ratio`           | `Get/SetFlapRatio`        | `V_CONTROLS_FLAP_RATIO`        | Flaps deployment ratio, `0..1`
`libxplanemp/controls/spoiler_ratio`        | `Get/SetSpoilerRatio`     | `V_CONTROLS_SPOILER_RATIO`     | Spoiler deployment ratio, `0..1`
`libxplanemp/controls/speed_brake_ratio`    | `Get/SetSpeedbrakeRatio`  | `V_CONTROLS_SPEED_BRAKE_RATIO` | Speed brakes deployment ratio, `0..1`
`libxplanemp/controls/slat_ratio`           | `Get/SetSlatRatio`        | `V_CONTROLS_SLAT_RATIO`        | Slats deployment ratio, `0..1`
`libxplanemp/controls/wing_sweep_ratio`     | `Get/SetWingSweepRatio`   | `V_CONTROLS_WING_SWEEP_RATIO`  | Wing sweep ratio, `0..1`
`libxplanemp/controls/thrust_ratio`         | `Get/SetThrustRatio`      | `V_CONTROLS_THRUST_RATIO`      | Thrust ratio, `0..1`
`libxplanemp/controls/yoke_pitch_ratio`     | `Get/SetYokePitchRatio`   | `V_CONTROLS_YOKE_PITCH_RATIO`  | Yoke pitch ratio, `-1..0..1`
`libxplanemp/controls/yoke_heading_ratio`   | `Get/SetYokeHeadingRatio` | `V_CONTROLS_YOKE_HEADING_RATIO`| Yoke heading ratio, `-1..0..1`
`libxplanemp/controls/yoke_roll_ratio`      | `Get/SetYokeRollRatio`    | `V_CONTROLS_YOKE_ROLL_RATIO`   | Yoke roll ratio, `-1..0..1`
`libxplanemp/controls/thrust_revers`        | `Get/SetThrustReversRatio`| `V_CONTROLS_THRUST_REVERS`     | Thrust reversers ratio, `0..1`
`libxplanemp/misc/touch_down`               | `Get/SetTouchDown`        | `V_MISC_TOUCH_DOWN`            | Moment of touch down, `0` or `1`

Lights
--

dataRef                                  | Getter/Setter                | index into `v`               | Meaning
---------------------------------------- | ---------------------------- | ---------------------------- | ---------------------------------
`libxplanemp/controls/taxi_lites_on`     | `Get/SetLightsTaxi`          | `V_CONTROLS_TAXI_LITES_ON`   | Taxi lights on? `0` or `1`
`libxplanemp/controls/landing_lites_on`  | `Get/SetLightsLanding`       | `V_CONTROLS_LANDING_LITES_ON`| Landing lights on? `0` or `1`
`libxplanemp/controls/beacon_lites_on`   | `Get/SetLightsBeacon`        | `V_CONTROLS_BEACON_LITES_ON` | Beacon lights on? `0` or `1`
`libxplanemp/controls/strobe_lites_on`   | `Get/SetLightsStrobe`        | `V_CONTROLS_STROBE_LITES_ON` | Strobe lights on? `0` or `1`
`libxplanemp/controls/nav_lites_on`      | `Get/SetLightsNav`           | `V_CONTROLS_NAV_LITES_ON`    | Navigation lights on? `0` or `1`

Gear
--

dataRef                                         | Getter/Setter             | index into `v`                        | Meaning
----------------------------------------------- | --------------------------| ------------------------------------- | ---------------------------------
`libxplanemp/controls/gear_ratio`               | `Get/SetGearRatio`        | `V_CONTROLS_GEAR_RATIO`               | Gear deployment ratio, `0..1`
`libxplanemp/controls/nws_ratio`                | `Get/SetNoseWheelAngle`   | `V_CONTROLS_NWS_RATIO`                | Nose wheel angle, `-45..+45`
`libxplanemp/gear/nose_gear_deflection_mtr` | `Get/SetNoseGearDeflection`   | `V_GEAR_NOSE_GEAR_DEFLECTION_MTR` | Vertical nose gear deflection, `0..1`
`libxplanemp/gear/tire_vertical_deflection_mtr` | `Get/SetTireDeflection`   | `V_GEAR_TIRE_VERTICAL_DEFLECTION_MTR` | Vertical (main) gear deflection, `0..1`
`libxplanemp/gear/tire_rotation_angle_deg`      | `Get/SetTireRotAngle`     | `V_GEAR_TIRE_ROTATION_ANGLE_DEG`      | Tire rotation angle, `0..359` degrees
`libxplanemp/gear/tire_rotation_speed_rpm`      | `Get/SetTireRotRpm`       | `V_GEAR_TIRE_ROTATION_SPEED_RPM`      | Tire rotation speed in revolutions per minute
`libxplanemp/gear/tire_rotation_speed_rad_sec`  | `Get/SetTireRotRad`       | `V_GEAR_TIRE_ROTATION_SPEED_RAD_SEC`  | Tire rotation speed in radians per second (`= _RPM * PI/30`)

Engines and Props
--

dataRef                                             | Getter/Setter             | index into `v`                            | Meaning
--------------------------------------------------- | --------------------------| ----------------------------------------- | ---------------------------------
`libxplanemp/engines/engine_rotation_angle_deg`     | `Get/SetEngineRotAngle`   | `V_ENGINES_ENGINE_ROTATION_ANGLE_DEG`     | All engines rotation angle, `0..359` degrees
`libxplanemp/engines/engine_rotation_angle_deg1`    | `Get/SetEngineRotAngle`   | `V_ENGINES_ENGINE_ROTATION_ANGLE_DEG1`    | Engine 1 rotation angle, `0..359` degrees
`libxplanemp/engines/engine_rotation_angle_deg2`    | `Get/SetEngineRotAngle`   | `V_ENGINES_ENGINE_ROTATION_ANGLE_DEG2`    | Engine 2 rotation angle, `0..359` degrees
`libxplanemp/engines/engine_rotation_angle_deg3`    | `Get/SetEngineRotAngle`   | `V_ENGINES_ENGINE_ROTATION_ANGLE_DEG3`    | Engine 3 rotation angle, `0..359` degrees
`libxplanemp/engines/engine_rotation_angle_deg4`    | `Get/SetEngineRotAngle`   | `V_ENGINES_ENGINE_ROTATION_ANGLE_DEG4`    | Engine 4 rotation angle, `0..359` degrees
`libxplanemp/engines/engine_rotation_speed_rpm`     | `Get/SetEngineRotRpm`     | `V_ENGINES_ENGINE_ROTATION_SPEED_RPM`     | All engines rotation speed in revolutions per minute
`libxplanemp/engines/engine_rotation_speed_rpm1`    | `Get/SetEngineRotRpm`     | `V_ENGINES_ENGINE_ROTATION_SPEED_RPM1`    | Engine 1 rotation speed in revolutions per minute
`libxplanemp/engines/engine_rotation_speed_rpm2`    | `Get/SetEngineRotRpm`     | `V_ENGINES_ENGINE_ROTATION_SPEED_RPM2`    | Engine 2 rotation speed in revolutions per minute
`libxplanemp/engines/engine_rotation_speed_rpm3`    | `Get/SetEngineRotRpm`     | `V_ENGINES_ENGINE_ROTATION_SPEED_RPM3`    | Engine 3 rotation speed in revolutions per minute
`libxplanemp/engines/engine_rotation_speed_rpm4`    | `Get/SetEngineRotRpm`     | `V_ENGINES_ENGINE_ROTATION_SPEED_RPM4`    | Engine 4 rotation speed in revolutions per minute
`libxplanemp/engines/engine_rotation_speed_rad_sec` | `Get/SetEngineRotRad`     | `V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC` | All engines rotation speed in radians per second (`= _RPM * PI/30`)
`libxplanemp/engines/engine_rotation_speed_rad_sec1`| `Get/SetEngineRotRad`     | `V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC1`| Engine 1 rotation speed in radians per second (`= _RPM * PI/30`)
`libxplanemp/engines/engine_rotation_speed_rad_sec2`| `Get/SetEngineRotRad`     | `V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC2`| Engine 2 rotation speed in radians per second (`= _RPM * PI/30`)
`libxplanemp/engines/engine_rotation_speed_rad_sec3`| `Get/SetEngineRotRad`     | `V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC3`| Engine 3 rotation speed in radians per second (`= _RPM * PI/30`)
`libxplanemp/engines/engine_rotation_speed_rad_sec4`| `Get/SetEngineRotRad`     | `V_ENGINES_ENGINE_ROTATION_SPEED_RAD_SEC4`| Engine 4 rotation speed in radians per second (`= _RPM * PI/30`)
`libxplanemp/engines/prop_rotation_angle_deg`       | `Get/SetPropRotAngle`     | `V_ENGINES_PROP_ROTATION_ANGLE_DEG`       | Propellor or rotor rotation angle, `0..359` degrees
`libxplanemp/engines/prop_rotation_speed_rpm`       | `Get/SetPropRotRpm`       | `V_ENGINES_PROP_ROTATION_SPEED_RPM`       | Propellor or rotor rotation speed in revolutions per minute
`libxplanemp/engines/prop_rotation_speed_rad_sec`   | `Get/SetPropRotRad`       | `V_ENGINES_PROP_ROTATION_SPEED_RAD_SEC`   | Propellor or rotor rotation speed in radians per second (`= _RPM * PI/30`)
`libxplanemp/engines/thrust_reverser_deploy_ratio`  | `Get/SetReversDeployRatio`| `V_ENGINES_THRUST_REVERSER_DEPLOY_RATIO`  | Thrust reversers deployment ration, `0..1`
