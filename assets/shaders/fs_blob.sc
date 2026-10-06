$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

// The blob's picture: its opacity in the top four bits of each texel
SAMPLER2D(s_diffuse, 0);

// A texel's opacity, in the sixteen steps the game keeps of it
float Opacity(vec2 texel)
{
	const float size = 32.0f;
	float value = texture2DLod(s_diffuse, (texel + 0.5f) / size, 0.0f).r;
	return floor(value * 255.0f / 16.0f) / 15.0f;
}

// A dark shadow, blended over the land by the picture's opacity, fading out towards its far end
void main()
{
	const float size = 32.0f;
	vec2 p = v_texcoord0.xy * size - 0.5f;
	vec2 corner = floor(p);
	vec2 t = p - corner;
	float top = mix(Opacity(corner), Opacity(corner + vec2(1.0f, 0.0f)), t.x);
	float bottom = mix(Opacity(corner + vec2(0.0f, 1.0f)), Opacity(corner + vec2(1.0f, 1.0f)), t.x);
	gl_FragColor = vec4(0.0f, 0.0f, 0.0f, mix(top, bottom, t.y) * v_color0.a);
}
