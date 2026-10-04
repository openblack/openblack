$input v_position, v_texcoord0, v_normal

#include <bgfx_shader.sh>

// Writes the coverage of a shadow caster's silhouette. LH3D rasterises the caster's triangles whatever their
// texture, so a translucent texture still casts a solid shadow.

void main()
{
	gl_FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
