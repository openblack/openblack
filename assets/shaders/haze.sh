#ifndef HAZE_SH
#define HAZE_SH

// The game's distance haze, from the land's light table. Colours here are 0 to 255.

uniform vec4 u_haze;       // x: near distance, y: far distance, z: k, w: 1 when the haze is on
uniform vec4 u_hazeColour; // rgb: the haze's colour

// How far into the haze a depth is, 0 to 1; 0 with the haze off
float HazeT(float depth)
{
	if (u_haze.w <= 0.0f)
	{
		return 0.0f;
	}
	return (min(max(depth, u_haze.x), u_haze.y) - u_haze.x) / (u_haze.y - u_haze.x);
}

// What a colour is scaled by, of 256: down to k at full haze, in whole steps
float HazeFactor(float t)
{
	return 256.0f - floor((256.0f - u_haze.z) * t);
}

// A colour scaled by the factor, (c f) >> 8 per channel
vec3 HazeDiffuse(vec3 colour, float factor)
{
	return factor < 256.0f ? floor(colour * factor / 256.0f) : colour;
}

// The haze colour added at t, rounded to the nearest with halves to even
vec3 HazeColour(float t)
{
	vec3 x = u_hazeColour.rgb * t;
	vec3 r = floor(x + 0.5f);
	return r - step(vec3_splat(0.5f), mod(r, vec3_splat(2.0f))) * step(vec3_splat(0.5f), r - x);
}

#endif // HAZE_SH
