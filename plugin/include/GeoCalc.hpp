#ifndef GeoCalc_h
#define GeoCalc_h

#include <cmath>

const double EARTH_RADIUS_NM = 3437.670013352;
const double PI_OVER_ONE_EIGHTY = 0.0174532925;
const double ONE_EIGHTY_OVER_PI = 57.2957795;
const int FEET_PER_NM = 6076;
const double METERS_PER_FOOT = 0.3048;
const double METERS_PER_NM = FEET_PER_NM * METERS_PER_FOOT;
const double NM_PER_DEG = 60.0;
const double FEET_PER_DEG = NM_PER_DEG * FEET_PER_NM;
const double METERS_PER_DEG = FEET_PER_DEG * METERS_PER_FOOT;
const double DEG_PER_NM = 1.0 / NM_PER_DEG;

static double DegreesToRadians(double degrees)
{
	return degrees * PI_OVER_ONE_EIGHTY;
}

static double DegreesToRadians(int degrees)
{
	return DegreesToRadians((double)degrees);
}

static double RadiansToDegrees(double radians)
{
	return radians * ONE_EIGHTY_OVER_PI;
}

static double NauticalMilesPerDegreeLon(double lat)
{
	return PI_OVER_ONE_EIGHTY * EARTH_RADIUS_NM * cos(DegreesToRadians(lat));
}

static double LongitudeScalingFactor(double lat)
{
	return NauticalMilesPerDegreeLon(lat) / NM_PER_DEG;
}

static double FeetToDegrees(int feet)
{
	return FeetToDegrees((double)feet);
}

static double FeetToDegrees(double feet)
{
	return feet / FEET_PER_NM / NM_PER_DEG;
}

static double MetersToDegrees(double meters)
{
	return meters / METERS_PER_NM / NM_PER_DEG;
}

static double FeetToNauticalMiles(int feet)
{
	return FeetToNauticalMiles((double)feet);
}

static double FeetToNauticalMiles(double feet)
{
	return feet / FEET_PER_NM;
}

static double DegreesToFeet(double degrees)
{
	return degrees * FEET_PER_DEG;
}

static double DegreesToMeters(double degrees)
{
	return degrees * METERS_PER_DEG;
}

static double DegreesToNauticalMiles(double degrees)
{
	return degrees * NM_PER_DEG;
}

static double NauticalMilesToDegrees(double nm)
{
	return nm / NM_PER_DEG;
}

static double NormalizeHeading(double heading)
{
	if (heading <= 0.0)
	{
		heading += 360.0;
	}
	else if (heading > 360.0)
	{
		heading -= 360.0;
	}
	return heading;
}

#endif // !GeoCalc_h
