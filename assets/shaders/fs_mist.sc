$input v_texcoord0, v_color0, v_haze

#include <bgfx_shader.sh>

// The smoke texture's frames and their alpha
SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_alpha, 1);

// The texture times the mist's colour and its alpha times the mist's, blended over what is behind it, with the distance
// haze added. Only the texture's full size is read: its smaller sizes would bleed the frames into each other.
void main()
{
	vec3 texel = texture2DLod(s_diffuse, v_texcoord0.xy, 0.0f).rgb;
	float alpha = texture2DLod(s_alpha, v_texcoord0.xy, 0.0f).r;
	gl_FragColor = vec4(min(texel * v_color0.rgb + v_haze.rgb, vec3_splat(1.0f)), alpha * v_color0.a);
}
