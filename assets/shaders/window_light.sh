#ifndef WINDOW_LIGHT_SH
#define WINDOW_LIGHT_SH

// The light in a house's windows at night while someone is home: a grey that comes up through the late evening and
// goes down before dawn, flickering a little, with each house a little off the hour by where it stands.

// x: 1 for a window submesh. y: the visual hour. z: 1 at night
uniform vec4 u_window;

// The windows' grey, 0 to 255, or -1 while they are dark
float WindowGrey(vec3 origin)
{
	// Each house is a fraction of an hour off by where it stands
	float f = abs(origin.x + origin.z) * 0.1f + origin.y;
	float t = (f - (f < 0.0f ? ceil(f) : floor(f))) + u_window.y;
	float intensity = 0.0f;
	if (t > 20.5f)
	{
		intensity = floor((t - 20.5f) * 8192.0f);
	}
	else if (t < 3.0f)
	{
		intensity = floor((3.0f - t) * 8192.0f);
	}
	if (intensity <= 0.0f)
	{
		return -1.0f;
	}
	// It flickers through eight levels as the hour moves on
	float step = mod(floor(t * 1000.0f), 8.0f);
	float flicker = step < 0.5f ? 0.0f : step < 1.5f ? 7.0f : step < 2.5f ? 3.0f : step < 3.5f ? 5.0f :
	                step < 4.5f ? 4.0f : step < 5.5f ? 2.0f : step < 6.5f ? 6.0f : 1.0f;
	float grey = 224.0f + flicker * 4.0f;
	// Below full, it fades in whole steps
	return intensity < 256.0f ? floor(grey * intensity / 256.0f) : grey;
}

#endif // WINDOW_LIGHT_SH
