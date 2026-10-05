$input v_position, v_texcoord0, v_normal, v_color0, v_haze

#include <bgfx_shader.sh>

// The game draws the main room mirrored through its floor before the room. Where nothing of the room covers it,
// as in the pool under its water, the reflection is what shows. The reflection pass is drawn from the mirrored camera
// with the same projection, so it lines up on the screen.
SAMPLER2D(s_reflection, 4);

void main()
{
	gl_FragColor = vec4(texture2D(s_reflection, gl_FragCoord.xy * u_viewTexel.xy).rgb, 1.0f);
}
