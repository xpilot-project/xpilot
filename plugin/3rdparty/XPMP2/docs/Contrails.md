## Contrails

XPMP2 can add contrails to planes. This is achieved by instantiating
one additional object per contrail at the same position as the actual plane.
This object `Resources/Contrail/Contrail.obj` has no tris,
but only holds a reference to a particle system file, `Contrail.pss`.
The contrail is then created through X-Plane's
[particle system](https://developer.x-plane.com/article/x-plane-11-particle-system/).

### Disable

If you don't want contrails at all, you need to return `0` for the two config items
`XPMP_CFG_ITM_CONTR_MIN_ALT` and `XPMP_CFG_ITM_CONTR_MAX_ALT`.
In that case you don't even need to provide the `Resources/Contrail` folder
with your deployment.

### Deployment

If you want to make use of contrails, you need to ship the
`Resources/Contrail` folder with all its 3 files with your plugin.

### Default

By default, ie. without any provision in code, a single contrail will be
generated for jet aircraft flying in an altitude between 25,000 and 45,000 ft.

### Configuration of Automatic Contrails

XPMP2 asks for four contrail-related configuration items, with which
you can fine-tune how XPMP2 automatically generates contrails.
This should be sufficient for most usages, even complex plugins like LiveTraffic
only provide pass-through configuration options for these four parameters,
but no self-development implementation.

Config item | Default | Description
------------|---------|-------------------
`XPMP_CFG_ITM_CONTR_MIN_ALT` | 25000 | Minimum altitude in feet for automatic contrail generation
`XPMP_CFG_ITM_CONTR_MAX_ALT` | 45000 | Maximum altitude in feet for automatic contrail generation; set both min and max altitude to `0` to switch off automatic contrails
`XPMP_CFG_ITM_CONTR_LIFE` | 25 | Default maximum lifetime of contrail puffs in seconds, determines contrail length
`XPMP_CFG_ITM_CONTR_MULTI` | 0 | Boolean: Shall multiple contrails be auto-created (one per engine), or just one?

Keep in mind that contrails, using the particle system, may have the potential
to affect performance. That's why by default just a single contrail is generated.

### More Control

If you need more control and want to direct yourself when how many contrails
are shown, you should switch off the auto-generation by returning `0` to
both `XPMP_CFG_ITM_CONTR_MIN_ALT` and `XPMP_CFG_ITM_CONTR_MAX_ALT`,
and then call either of the following two member functions of `XPMP2::Aircraft`:

- `ContrailTrigger()` tells XPMP2 to check the plane's engine type,
  and if it is jet then a contrail (or even multiple, depending on
  config `XPMP_CFG_ITM_CONTR_MULTI`) are created. This is the function
  internally used by the auto-generation mechanism once a plane is in the
  right altitude.
- `ContrailRequest (unsigned num, unsigned dist_m = 0, unsigned lifeTime = 0)`
  allows even more control by specifying the number of contrails, its spacing,
  and the life time.

Last but not least, there's `ContrailRemove()` to remove contrails.
