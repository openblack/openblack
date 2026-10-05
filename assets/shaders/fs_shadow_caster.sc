$input v_position, v_texcoord0, v_normal, v_color0, v_haze

#include <bgfx_shader.sh>

// Writes the coverage of a shadow caster's silhouette. The game rasterises the caster's triangles whatever their
// texture, so a translucent texture still casts a solid shadow.

void main()
{
	gl_FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
