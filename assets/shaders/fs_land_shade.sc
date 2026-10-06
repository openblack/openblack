$input v_texcoord0

#include <bgfx_shader.sh>

// A cloud's shadow, or a light's image of brightness
SAMPLER2D(s_texture, 0);
// x: 0 for a cloud's shadow, 1 for a light. y: the shadow's alpha, or the light's strength, of 255. z: the light's image
// size in texels
uniform vec4 u_landShade;
// xy: the weights, of 255, the light's image is blended with along x and z
uniform vec4 u_landStamp;

// a / 255 rounded down, for a whole number a
float DivideBy255(float a)
{
	return floor((a + 0.5f) / 255.0f);
}

// A texel of the light's image, its rows along x and its columns along z, brought down to the land light's warm levels
float LightTexel(vec2 rowColumn)
{
	float value = floor(texture2DLod(s_texture, (rowColumn.yx + 0.5f) / u_landShade.z, 0.0f).r * 255.0f + 0.5f);
	return min(47.0f, floor(value * 0.18823529f + 0.5f));
}

void main()
{
	if (u_landShade.x < 0.5f)
	{
		// A shadow darkens the land by its alpha towards the shadow's own darkness, never below 48; the land keeps the
		// darkest shadow over it
		float shadow = floor(texture2D(s_texture, v_texcoord0.xy).r * 255.0f + 0.5f);
		float value = max(48.0f, 255.0f - DivideBy255((255.0f - shadow) * u_landShade.y));
		gl_FragColor = vec4(value / 255.0f, 0.0f, 0.0f, 0.0f);
	}
	else
	{
		// The cell's place in the image: the four texels from it blended along z and then along x by the weights, in
		// whole steps, then by the light's strength. The land keeps the brightest light over it.
		vec2 cell = floor(v_texcoord0.xy);
		float b = LightTexel(cell);
		float a = LightTexel(cell + vec2(0.0f, 1.0f));
		float d = LightTexel(cell + vec2(1.0f, 0.0f));
		float c = LightTexel(cell + vec2(1.0f, 1.0f));
		float first = b + floor((a - b) * u_landStamp.y / 256.0f);
		float second = d + floor((c - d) * u_landStamp.y / 256.0f);
		float light = first + floor((second - first) * u_landStamp.x / 256.0f);
		gl_FragColor = vec4(0.0f, 0.0f, 0.0f, DivideBy255(light * u_landShade.y) / 255.0f);
	}
}
