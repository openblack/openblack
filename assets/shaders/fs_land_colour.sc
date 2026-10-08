$input v_texcoord0

#include <bgfx_shader.sh>

// The land's colours as they were laid, or a stamp's image of colours
SAMPLER2D(s_texture, 0);
// x: 0 to copy the land's colours, 1 for a stamp. y: the stamp's strength, of 255. z: its image size in texels. w: 1 for
// a shade, whose image is how much light it lets through, the land's colours multiplied by it
uniform vec4 u_landColourStamp;
// xy: the weights, of 255, the stamp's image is blended with along x and z
uniform vec4 u_landColourWeights;

// A texel of the stamp's image as bytes, its rows along x and its columns along z
vec3 StampTexel(vec2 rowColumn)
{
	return floor(texture2DLod(s_texture, (rowColumn.yx + 0.5f) / u_landColourStamp.z, 0.0f).rgb * 255.0f + 0.5f);
}

void main()
{
	if (u_landColourStamp.x < 0.5f)
	{
		gl_FragColor = vec4(texture2D(s_texture, v_texcoord0.xy).rgb, 1.0f);
		return;
	}
	// The cell's place in the image: the four texels from it blended along z and then along x by the weights, in whole
	// steps, then by the stamp's strength
	vec2 cell = floor(v_texcoord0.xy);
	vec3 a = StampTexel(cell);
	vec3 b = StampTexel(cell + vec2(0.0f, 1.0f));
	vec3 c = StampTexel(cell + vec2(1.0f, 0.0f));
	vec3 d = StampTexel(cell + vec2(1.0f, 1.0f));
	vec3 first = a + floor((b - a) * u_landColourWeights.y / 256.0f);
	vec3 second = c + floor((d - c) * u_landColourWeights.y / 256.0f);
	vec3 colour = first + floor((second - first) * u_landColourWeights.x / 256.0f);
	if (u_landColourStamp.w > 0.5f)
	{
		// The shade lets through all the light where it is faint
		colour = 255.0f - floor(((255.0f - colour) * u_landColourStamp.y + 0.5f) / 255.0f);
		gl_FragColor = vec4(colour / 255.0f, 1.0f);
		return;
	}
	colour = floor((colour * u_landColourStamp.y + 0.5f) / 255.0f);
	gl_FragColor = vec4(colour / 255.0f, 0.0f);
}
