Model Matching
==

"Model Matching" describes how XPMP2 picks one of the installed CSL models
to _match_ with what your plugins would like to draw into the simulator's
sky. Users will have installed a varying amount of CSL models from which
XPMP2 can choose.

Matching takes place
- upon creation of a new plane, preferably by instantiating a new object
  of your aircraft class derived from [`XPMP2::Aircraft`](html/classXPMP2_1_1Aircraft.html),
- when specifically instructed, e.g. by calling `XPMP2::Aircraft::ChangeModel`
  or `XPMP2::Aircraft::ReMatchModel`.

Input
--

Matching is based on 3 input parameters:
- The ICAO **aircraft type designator** of the plane,
- the ICAO **operator code** to identify the airline,
- a text to identify a **special livery**.

All parameters are optional. Pass an empty string if you can't provide details.

Internally, two more parameters are added:
- A parameter named **related group**,
  derived from the ICAO aircraft type designator:
  There are many similar looking aircraft models in the world...just think of
  A319, A320, A321. The `related.txt` files combines all similar looking models
  into one line, which make up such a group. Each aircraft type is allowed to
  appear at maximum once in the entire file.
  The idea is to use a A320 model if a necessary A319 model is not available.
- A parameter named **related operator group**,
  derived from the ICAO airline code:
  There are airlines identified by differing airline codes (and call signs),
  which still share the same liveries or even actual planes,
  like subsidaries of mother airlines. The idea is that EUK (Aer Lingus UK)
  can use the same liveries as EIN (Air Lingus Ireland) if
  no EUK-specific livery is available.

Aircraft / airline / livery combinations available for the user are listed in the
`xsb_aircraft.txt` files in the user's CSL model folders.
XPMP2 reads all of these files into a cache when your plugin calls
`XPMPLoadCSLPackage`, i.e. typically at startup.

Procedure
--

Model Matching performs 12 comparisons between the parameters you pass
during aircraft creation and the available models. They are rated
according to the following priority from broad to fine-grained attributes:

1. "has rotor" - this avoids picking a winged model for a helicopter
2. ICAO aircraft class type - landplane, seaplane, helicopter, ...
3. Wake Turbulence Category - a good rough general size indicator
4. Number of engines
5. Engine type - jet, turbo, piston
6. Related group - see above, groups similar-looking models like A319, A320, A321
7. ICAO airline / operator and related group together match
   (this means that a "related" model with matching livery is preferred over an
   exactly matching aircraft with wrong livery)
8. Related operator group and related group together match
   (this means that a "related" model with a matching group livery is preferred over an
   exactly matching aircraft with wrong livery)
9.  ICAO aircraft type designator - like A320, A388, B738, B741, C172, MD82, ..
10. Related operator group - see above, groups similar-looking airlines like EIN for EUK
11. ICAO airline / operator - like AAL, AFR, BAW, DLH, SWA, WJA, ...
12. Special Livery

The better a potential CSL model matches with the passed-in parameters
the more of the above attributes match, the higher is the "match quality".
Higher priority attributes count more than lower ones: technically it is a bit mask
with the number 1 attribute at the most significant bit.

The model with the best match quality is picked. If there are several models
ending up with the same match quality, then XPMP2 takes a random pick.
This often happens if there are several models available for the expected
ICAO aircraft type designator, but none of them has the correct livery
for the requested operator.

There is no "default" livery any longer as there was in `libxplanemp`.
The downside is that it is a bit harder to identify non-matching liveries:
Asking twice with the same match parameters can end in different models
being picked.

Logging
--

If you are interested in how models are picked then you can activate
logging of model matching: In your configuration callback return
`1` for key `XPMP_CFG_ITM_MODELMATCHING` and
`0` or `1` for key `XPMP_CFG_ITM_LOGLEVEL`.

With model matching logging activated, XPMP2 writes info into `Log.txt` like this:

```
.../CSLFindMatch: MATCH INPUT: Type=BE36 (WTC=L,Class=L1P,Related=128), Airline=N13, Livery=N136HP
.../CSLFindMatch: MATCH FOUND: Type=BE20 (WTC=L,Class=L2T,Related=128), Airline=SLG, Livery= / Quality = 56 -> model BB_GA/BE20_CGSAE

.../CSLFindMatch: MATCH INPUT: Type=B737 (WTC=M,Class=L2J,Related=56), Airline=SWA, Livery=N462WN
.../CSLFindMatch: MATCH FOUND: Type=B737 (WTC=M,Class=L2J,Related=56), Airline=SWA, Livery= / Quality = 2 -> model BB_Boeing/B737_SWA

.../CSLFindMatch: MATCH INPUT: Type=DHC6 (WTC=L,Class=L2T,Related=108), Airline=CVU, Livery=N190GC
.../CSLFindMatch: MATCH FOUND: Type=BE20 (WTC=L,Class=L2T,Related=128), Airline=JEI, Livery= / Quality = 16 -> model BB_GA/BE20_DIKOB

.../CSLFindMatch: MATCH INPUT: Type=H500 (WTC=L,Class=H1T,Related=177), Airline=N91, Livery=N911WY
.../CSLFindMatch: MATCH FOUND: Type=B06 (WTC=L,Class=H1T,Related=177), Airline=PAT, Livery= / Quality = 8 -> model BB_Heli/B06_US3_31A
```

Note that "quality" is reported inverse here in `Log.txt` (as it is used in
XPMP2's code): Lower numbers are better matches.

For example: The first match (Quality = 56) is a comparably bad one...
just look at the aircraft class: Wanted is a "L1P" aircraft (one piston engine),
the best we found is an L2T (two turbo engines).
